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
mod controller;
mod http_service;
mod model;

use crate::config::Config;
use crate::model::TestEchoData;
use controller::new_controller;
use getopts::Options;
use model::Model;
use std::io::ErrorKind;

/// Environment path for specifying http-dispatcher application binary
const HTTP_DISPATCHER_PATH_ENV: &str = "HTTP_DISPATCHER_PATH";
const HTTP_DISPATCHER_PATH_DEFAULT: &str = "/opt/echo/http-dispatcher";
const HTTP_INSPECT_ENDPOINT_RETRIES: usize = 10;
const HTTP_INSPECT_ENDPOINT_RETRIES_TIMEOUT: u64 = 1000; //ms

// Perform get request to this app inspect endpoint, return true
// if it is up and running
async fn probe_inspect_endpoint(config: &Config) -> bool {
    let client = hyper::Client::new();
    let uri = format!("http://{}:{}/ping", &config.http_address, &config.http_port);
    let req = hyper::Request::builder()
        .method(hyper::Method::GET)
        .uri(uri)
        .body(hyper::Body::from(""))
        .expect("inspect request");
    // Send GET request to DApp
    match client.request(req).await {
        Ok(res) => {
            if res.status().is_success() {
                true
            } else {
                log::debug!("Unable to probe inspect endpoint, responose {:?}", res);
                false
            }
        }
        Err(_e) => false,
    }
}

/// Instantiate http dispatcher in subprocess
async fn start_http_dispatcher(config: &Config, verbose: bool) {
    let http_dispatcher_bin_path = std::env::var(HTTP_DISPATCHER_PATH_ENV).unwrap_or_else(|e| {
        log::info!(
            "Error fetching `{}`: {}. Defaulting to {}",
            HTTP_DISPATCHER_PATH_ENV,
            e,
            HTTP_DISPATCHER_PATH_DEFAULT
        );
        HTTP_DISPATCHER_PATH_DEFAULT.to_string()
    });
    // Wait for Dapp http service to start because http dispatcher and
    // Dapp are mutually dependant
    log::debug!("Waiting for ping endpoint to start...");
    let mut i = 0;
    loop {
        if probe_inspect_endpoint(config).await == true {
            log::debug!("Inspect ping up and running");
            break;
        }
        if i >= HTTP_INSPECT_ENDPOINT_RETRIES {
            panic!("Timeout, Dapp ping http service unavailable");
        }
        std::thread::sleep(std::time::Duration::from_millis(
            HTTP_INSPECT_ENDPOINT_RETRIES_TIMEOUT,
        ));
        i += 1;
    }

    log::info!(
        "Dapp http endpoints online, starting http dispatcher service {}",
        http_dispatcher_bin_path
    );
    let _output = std::process::Command::new(http_dispatcher_bin_path)
        .arg(&format!(
            "--address={}",
            &format!("{}:{}", config.dispatcher_address, config.dispatcher_port)
        ))
        .arg(&format!(
            "--dapp={}",
            format_args!("{}:{}", config.http_address, config.http_port)
        ))
        .arg(if verbose { "--verbose" } else { "" })
        .spawn()
        .expect("Unable to launch http dispatcher");
}

fn print_usage(program: &str, opts: Options) {
    let brief = format!("Usage: {} [options]\n Where options are:", program);
    print!("{}", opts.usage(&brief));
}

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    // Process command line arguments
    let args: Vec<String> = std::env::args().collect();
    let program = args[0].clone();
    let mut opts = Options::new();
    opts.optopt(
        "",
        "address",
        "Address of the dapp http service (default: 127.0.0.1:5002)",
        "",
    );
    opts.optopt(
        "",
        "dispatcher",
        "Http dispatcher address (default: 127.0.0.1:5001)",
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
    opts.optflag("h", "help", "show this help message and exit");
    opts.optflag(
        "",
        "without-dispatcher",
        "start dapp wihtout http-dispatcher (for x86 execution) ",
    );
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
    env_logger::Builder::from_env(env_logger::Env::default().default_filter_or(log_level)).init();

    // Setup configuration
    let mut config = Config::new();
    config.wihtout_dispatcher = matches.opt_present("without-dispatcher");
    {
        // Parse addresses and ports
        let address_match = matches
            .opt_get_default("address", "127.0.0.1:5002".to_string())
            .unwrap_or_default();
        let mut address = address_match.split(':');
        config.http_address = address.next().expect("address is not valid").to_string();
        config.http_port = address
            .next()
            .expect("port is not valid")
            .to_string()
            .parse::<u16>()
            .unwrap();
        let dispatcher_match = matches
            .opt_get_default("dispatcher", "127.0.0.1:5001".to_string())
            .unwrap_or_default();
        let mut dispatcher = dispatcher_match.split(':');
        config.dispatcher_address = dispatcher
            .next()
            .expect("http dispatcher address is not valid")
            .to_string();
        config.dispatcher_port = dispatcher
            .next()
            .expect("http dispatcher port is not valid")
            .to_string()
            .parse::<u16>()
            .unwrap();
    }
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
    // Instantiate Dapp model that implements DApp logic
    let model = Box::new(Model::new(
        &config.dispatcher_address,
        config.dispatcher_port,
        TestEchoData {
            vouchers,
            notices,
            reports,
            reject,
        },
    ));

    if config.wihtout_dispatcher {
        log::info!("starting dapp without http dispatcher")
    }

    // Start http dispatcher in the background
    if !config.wihtout_dispatcher {
        let config = config.clone();
        tokio::spawn(async move {
            crate::start_http_dispatcher(&config, verbose).await;
        });
    }

    // Controller handles application flow
    let (channel, service) = new_controller(model, config.clone());
    // Run controller service as async service
    tokio::spawn(async {
        service.run().await;
        log::info!("http service service terminated successfully");
    });
    // Run actix http service with advance/inspect interface implementation
    match http_service::run(&config, channel).await {
        Ok(_) => log::info!("http service terminated successfully"),
        Err(e) => log::warn!("http service terminated with error: {}", e),
    }

    Ok(())
}
