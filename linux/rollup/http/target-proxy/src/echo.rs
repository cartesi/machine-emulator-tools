/* Copyright 2021 Cartesi Pte. Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

use target_proxy::rollup;
use target_proxy::rollup::{AdvanceRequest, Notice, Report, RollupFinish, Voucher};

use getopts::Options;
use std::fs::File;
#[cfg(unix)]
use std::os::unix::io::{IntoRawFd, RawFd};

const ROLLUP_DEVICE_NAME: &str = "/dev/rollup";

fn print_usage(program: &str, opts: Options) {
    let brief = format!(
        "Usage: {} [options]\n Where options are: \n
          --vouchers=<n>   replicate input in n vouchers (default: 0)\n
          --notices=<n>    replicate input in n notices (default: 0)\n
          --reports=<n>    replicate input in n reports (default: 1)\n
          --verbose=<n>    display information of structures (default: 0)\n",
        program
    );
    print!("{}", opts.usage(&brief));
}

fn generate_test_output(
    rollup_fd: RawFd,
    advance_state: &AdvanceRequest,
    verbose: bool,
    test_data: TestData,
) -> Result<(), Box<dyn std::error::Error>> {
    // Generate some test vouchers, notices and reports
    // if requested by cmd line arguments
    if test_data.vouchers > 0 {
        //Generate test voucher
        let mut test_payload = advance_state.payload.clone();
        test_payload.push_str("-test-voucher");
        let mut voucher = Voucher {
            address: advance_state.metadata.msg_sender.clone(),
            payload: test_payload,
        };
        rollup::rollup_write_voucher(rollup_fd, test_data.notices, &mut voucher)?;
        if verbose {
            rollup::print_voucher(&voucher);
        }
    }

    if test_data.notices > 0 {
        // Generate test notice
        let mut test_payload = advance_state.payload.clone();
        test_payload.push_str("-test-notice");
        let notice = Notice {
            payload: test_payload,
        };
        rollup::rollup_write_notices(rollup_fd, test_data.notices, &notice)?;
        if verbose {
            rollup::print_notice(&notice);
        }
    }

    if test_data.reports > 0 {
        // Generate test reports
        let mut test_payload = advance_state.payload.clone();
        test_payload.push_str("-test-report");
        let report = Report {
            payload: test_payload,
        };
        rollup::rollup_write_report(rollup_fd, test_data.notices, &report)?;
        if verbose {
            rollup::print_report(&report);
        }
    }

    Ok(())
}

fn handle_rollup_requests(
    rollup_fd: RawFd,
    finish_request: &mut RollupFinish,
    verbose: bool,
    test_data: TestData,
) -> Result<(), Box<dyn std::error::Error>> {
    let next_request_type = finish_request.next_request_type as u32;
    println!("Next request type is {}", next_request_type);
    match next_request_type {
        rollup::CARTESI_ROLLUP_ADVANCE_STATE => {
            let advance_state =
                rollup::rollup_read_advance_state_request(rollup_fd, finish_request)?;
            if verbose {
                rollup::print_advance(&advance_state);
            }
            generate_test_output(rollup_fd, &advance_state, verbose, test_data)?;
        }
        rollup::CARTESI_ROLLUP_INSPECT_STATE => {}
        _ => {
            return Err(Box::new(rollup::RollupError::new(&format!(
                "Unknown request type"
            ))));
        }
    }
    Ok(())
}

struct TestData {
    vouchers: u32,
    reports: u32,
    notices: u32,
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    println!("Starting Rust echo program!");
    let args: Vec<String> = std::env::args().collect();
    let program = args[0].clone();
    // Process command line arguments
    let mut opts = Options::new();
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
    opts.optopt(
        "",
        "verbose",
        "Display information of structures (default: 0)",
        "",
    );
    opts.optflag("h", "help", "show this help message and exit");
    let matches = opts.parse(&args[1..])?;
    if matches.opt_present("h") {
        print_usage(&program, opts);
        return Ok(());
    }
    let vouchers = matches.opt_get_default("vouchers", 0)?;
    let notices = matches.opt_get_default("notices", 0)?;
    let reports = matches.opt_get_default("reports", 1)?;
    let verbose = matches.opt_get_default("verbose", 0)?;
    let verbose: bool = if verbose > 0 { true } else { false };

    // Open rollup device
    let rollup_file = match File::open(ROLLUP_DEVICE_NAME) {
        Ok(file) => file,
        Err(e) => {
            println!("Error opening rollup device {}", e.to_string());
            return Err(Box::new(e));
        }
    };
    let rollup_fd: RawFd = rollup_file.into_raw_fd();

    loop {
        let mut finish_request = rollup::RollupFinish::default();
        rollup::rollup_finish_request(rollup_fd, &mut finish_request)?;
        handle_rollup_requests(
            rollup_fd,
            &mut finish_request,
            verbose,
            TestData {
                vouchers,
                reports,
                notices,
            },
        )?;
    }
}
