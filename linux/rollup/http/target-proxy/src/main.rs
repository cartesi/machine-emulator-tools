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

mod config;
mod http_service;

use crate::config::Config;
use async_mutex::Mutex;
use getopts::Options;
use std::fs::File;
use std::io::ErrorKind;
#[cfg(unix)]
use std::os::unix::io::{IntoRawFd, RawFd};
use std::sync::Arc;
use target_proxy::rollup;
use target_proxy::rollup::RollupFinish;

const ROLLUP_DEVICE_NAME: &str = "/dev/rollup";

fn print_usage(program: &str, opts: Options) {
    let brief = format!(
        "Usage: {} [options]\n Where options are: \n
          --address=<n>  address/port to open the service (default: 127.0.0.1:5001)\n",
        program
    );
    print!("{}", opts.usage(&brief));
}

fn handle_rollup_requests(
    rollup_fd: RawFd,
    finish_request: &mut RollupFinish,
) -> std::io::Result<()> {
    let next_request_type = finish_request.next_request_type as u32;
    log::info!("Next request type is {}", next_request_type);
    match next_request_type {
        rollup::CARTESI_ROLLUP_ADVANCE_STATE => {
            let advance_state =
                match rollup::rollup_read_advance_state_request(rollup_fd, finish_request) {
                    Ok(r) => r,
                    Err(e) => {
                        return Err(std::io::Error::new(ErrorKind::Other, e.to_string()));
                    }
                };
            if log::log_enabled!(log::Level::Info) {
                rollup::print_advance(&advance_state);
            }
        }
        rollup::CARTESI_ROLLUP_INSPECT_STATE => {}
        _ => {
            return Err(std::io::Error::new(
                ErrorKind::Unsupported,
                "Request type unsupported",
            ));
        }
    }
    Ok(())
}

async fn linux_rollup_loop(rollup_fd: Arc<Mutex<RawFd>>) -> () {
    loop {
        let mut finish_request = rollup::RollupFinish::default();
        let fd = rollup_fd.lock().await;
        match rollup::rollup_finish_request(*fd, &mut finish_request) {
            Ok(_) => {}
            Err(e) => {
                log::error!("Error performing rollup_finish_request {}", e.to_string());
            }
        }
        match handle_rollup_requests(*fd, &mut finish_request) {
            Ok(_) => {}
            Err(e) => {
                log::error!("Error performing handle_rollup_requests {}", e.to_string());
            }
        }
        drop(fd);
    }
}

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    // Set the default log level to info
    env_logger::Builder::from_env(env_logger::Env::default().default_filter_or("info")).init();
    log::info!("Starting Rust target-proxy!");

    let args: Vec<String> = std::env::args().collect();
    let program = args[0].clone();
    // Process command line arguments
    let mut opts = Options::new();
    opts.optflag("h", "help", "show this help message and exit");
    opts.optopt(
        "",
        "address",
        "Address to listen (default: 127.0.0.1:5001)",
        "",
    );
    opts.optopt("p", "port", "Port to listen (default: 5001)", "");
    let matches = match opts.parse(&args[1..]) {
        Ok(m) => m,
        Err(e) => {
            log::error!("error parsing arguments: {}", &e);
            return Err(std::io::Error::new(ErrorKind::InvalidInput, e.to_string()));
        }
    };
    if matches.opt_present("h") {
        print_usage(&program, opts);
        return Ok(());
    }

    let mut http_config = Config::new();
    http_config.http_address = matches
        .opt_get_default("address", "127.0.0.1".to_string())
        .unwrap_or_default();
    http_config.http_port = matches.opt_get_default("port", 5001).unwrap_or_default();

    // Open rollup device
    let rollup_file = match File::open(ROLLUP_DEVICE_NAME) {
        Ok(file) => file,
        Err(e) => {
            log::error!("Error opening rollup device {}", e.to_string());
            return Err(e);
        }
    };

    // Linux rollup must wait for the http service to initialize
    let (start_rollup_tx, start_rollup_rx): (
        std::sync::mpsc::Sender<bool>,
        std::sync::mpsc::Receiver<bool>,
    ) = std::sync::mpsc::channel();

    let rollup_fd: Arc<Mutex<RawFd>> = Arc::new(Mutex::new(rollup_file.into_raw_fd()));

    // Start linux loop thread
    {
        let rollup_fd: Arc<Mutex<RawFd>> = rollup_fd.clone();
        std::thread::spawn(move || {
            start_rollup_rx.recv().unwrap();
            // We need async runtime to be able to use async mutexes
            let runtime = tokio::runtime::Builder::new_current_thread()
                .thread_name(&format!("Linux rollup loop"))
                .thread_stack_size(1024 * 1024)
                .build()
                .unwrap();
            runtime.block_on(async {
                linux_rollup_loop(rollup_fd).await;
            });
        });
    }

    // Open http service
    tokio::select! {
        result = http_service::run(&http_config, start_rollup_tx, rollup_fd) => {
            match result {
                Ok(_) => log::info!("http service terminated successfully"),
                Err(e) => log::warn!("http service terminated with error: {}", e),
            }
        }
    }
    log::info!("Ending Rust target-proxy!");
    Ok(())
}
