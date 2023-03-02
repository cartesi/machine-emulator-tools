// Copyright 2021 Cartesi Pte. Ltd.
//
// SPDX-License-Identifier: Apache-2.0
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use
// this file except in compliance with the License. You may obtain a copy of the
// License at http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

mod config;

use crate::config::{Config, TestConfig};
use rollup_http_client::rollup::{
    AdvanceRequest, Exception, InspectRequest, Notice, Report, RollupRequest, RollupRequestError,
    RollupResponse, Voucher,
};

use getopts::Options;
use std::io::ErrorKind;
use rollup_http_client::client;

fn print_usage(program: &str, opts: Options) {
    let brief = format!("Usage: {} [options]\n Where options are:", program);
    print!("{}", opts.usage(&brief));
}

pub async fn process_advance_request(
    config: &mut Config,
    request: &AdvanceRequest,
) -> Result<(), RollupRequestError> {
    log::debug!("dapp Received advance request {:?}", &request);
    // Generate test echo vouchers
    if config.test_config.vouchers > 0 {
        log::debug!("generating {} echo vouchers", config.test_config.vouchers);
        // Generate test vouchers
        for _ in 0..config.test_config.vouchers {
            let voucher_payload = request.payload.clone();
            let voucher = Voucher {
                destination: request.metadata.msg_sender.clone(),
                payload: voucher_payload,
            };

            // Send voucher to http dispatcher
            client::send_voucher(&config.rollup_http_server_address, voucher).await;
        }
    }
    // Generate test echo notices
    if config.test_config.notices > 0 {
        // Generate test notice
        log::debug!("Generating {} echo notices", config.test_config.notices);
        for _ in 0..config.test_config.notices {
            let notice_payload = request.payload.clone();
            let notice = Notice {
                payload: notice_payload,
            };
            client::send_notice(&config.rollup_http_server_address, notice).await;
        }
    }
    // Generate test echo reports
    if config.test_config.reports > 0 {
        // Generate test reports
        log::debug!("generating {} echo reports", config.test_config.reports);
        for _ in 0..config.test_config.reports {
            let report_payload = request.payload.clone();
            let report = Report {
                payload: report_payload,
            };
            client::send_report(&config.rollup_http_server_address, report).await;
        }
    }

    Ok(())
}

pub async fn process_inspect_request(
    config: &mut Config,
    request: &InspectRequest,
) -> Result<(), RollupRequestError> {
    log::debug!("Dapp received inspect request {:?}", &request);

    if config.test_config.reports > 0 {
        // Generate test reports for inspect
        log::debug!(
            "generating {} echo inspect state reports",
            config.test_config.reports
        );
        for _ in 0..config.test_config.reports {
            let report_payload = request.payload.clone();
            let report = Report {
                payload: report_payload,
            };
            client::send_report(&config.rollup_http_server_address, report).await;
        }
    }

    Ok(())
}

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    // Process command line arguments
    let args: Vec<String> = std::env::args().collect();
    let program = args[0].clone();
    let mut opts = Options::new();
    opts.optopt(
        "",
        "rollup-http-server",
        "Rollup http server address (default: 127.0.0.1:5004)",
        "",
    );
    opts.optopt(
        "",
        "vouchers",
        "Replicate input in n vouchers (default 0)",
        "",
    );
    opts.optopt(
        "",
        "notices",
        "Replicate input in n notices (default 0)",
        "",
    );
    opts.optopt(
        "",
        "reports",
        "Replicate input in n reports (default 1)",
        "",
    );
    opts.optopt("", "reject", "Reject the nth input (default: -1)", "");
    opts.optflag("", "reject-inspects", "reject all inspect requests");
    opts.optopt(
        "",
        "exception",
        "Cause an exception on the nth input (default: -1)",
        "",
    );
    opts.optflag("h", "help", "show this help message and exit");
    opts.optflag("", "verbose", "print more info about application execution");
    let matches = match opts.parse(&args[1..]) {
        Ok(m) => m,
        Err(e) => {
            eprintln!("error parsing arguments: {}", &e);
            return Err(std::io::Error::new(ErrorKind::InvalidInput, e.to_string()));
        }
    };
    if matches.opt_present("h") {
        print_usage(&program, opts);
        return Ok(());
    }

    // Set log level of application
    let mut log_level = "info";
    let verbose = matches.opt_present("verbose");
    if verbose {
        log_level = "debug";
    }
    // Set the global log level
    env_logger::Builder::from_env(env_logger::Env::default().default_filter_or(log_level))
        .format_timestamp(None)
        .init();

    // Parse echo dapp parameters
    let vouchers = matches
        .opt_get_default("vouchers", 0)
        .expect("vouchers could not be parsed");
    let notices = matches
        .opt_get_default("notices", 0)
        .expect("notices could not be parsed");
    let reports = matches
        .opt_get_default("reports", 1)
        .expect("reports could not be parsed");
    let reject = matches
        .opt_get_default("reject", -1)
        .expect("reject could not be parsed");
    let reject_inspects = matches.opt_present("reject-inspects");
    let exception = matches
        .opt_get_default("exception", -1)
        .expect("exception could not be parsed");

    // Setup configuration
    let mut config = Config::new(TestConfig {
        vouchers,
        notices,
        reports,
        reject,
        reject_inspects,
        exception,
    });


    // Parse rollup http server address
    let server_address = matches
        .opt_get_default("rollup-http-server", "127.0.0.1:5004".to_string())
        .unwrap_or_default();
    config.rollup_http_server_address = format!("http://{}", server_address);


    let mut request_response = RollupResponse::Finish(true);

    loop {
        let request = client::send_finish_request(
            &config.rollup_http_server_address,
            &request_response,
        )
        .await?;

        match request {
            RollupRequest::Inspect(inspect_request) => {
                request_response = match process_inspect_request(&mut config, &inspect_request).await {
                    Ok(_) => {
                        RollupResponse::Finish(!config.test_config.reject_inspects)
                    }
                    Err(error) => {
                        log::error!("Error processing inspect request: {}", error.to_string());
                        RollupResponse::Finish(false)
                    }
                };
            }
            RollupRequest::Advance(advance_request) => {
                request_response = match process_advance_request(&mut config, &advance_request).await {
                    Ok(_) => {
                            let accept = !(config.test_config.reject == advance_request.metadata.input_index as i32);
                            RollupResponse::Finish(accept)
                    }
                    Err(error) => {
                        log::error!("Error processing advance request: {}", error.to_string());
                        RollupResponse::Finish(false)
                    }
                };

                // Do the exception if specified by app parameter
                if config.test_config.exception == advance_request.metadata.input_index as i32 {
                    client::throw_exception(
                        &config.rollup_http_server_address,
                        Exception {
                            payload: "0x".to_string()
                                + &hex::encode(format!(
                                    "exception thrown after {}th input ",
                                    exception
                                )),
                        },
                    )
                    .await;
                };
            }
        }
    }
}
