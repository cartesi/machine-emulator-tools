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
use rollup_http_server::rollup;
use rollup_http_server::rollup::{
    RollupFinish, RollupRequest, RollupResponse,
};
use std::fs::File;
use std::io::ErrorKind;
#[cfg(unix)]
use std::os::unix::io::{IntoRawFd, RawFd};
use std::sync::Arc;
use tokio::sync::mpsc;

fn print_usage(program: &str, opts: Options) {
    let brief = format!("Usage: {} [options]\n Where options are:", program);
    print!("{}", opts.usage(&brief));
}

async fn perform_rollup_finish_request(
    rollup_fd: &Arc<Mutex<RawFd>>,
    accept: bool,
) -> std::io::Result<rollup::RollupFinish> {
    let mut finish_request = rollup::RollupFinish::default();
    let fd = rollup_fd.lock().await;
    match rollup::rollup_finish_request(*fd, &mut finish_request, accept) {
        Ok(_) => Ok(finish_request),
        Err(e) => {
            log::error!("error inserting finish request, details: {}", e.to_string());
            Err(std::io::Error::new(ErrorKind::Other, e.to_string()))
        }
    }
}

/// Read advance/inspect request from rollup device
/// and send http request to DApp REST server
async fn handle_rollup_requests(
    rollup_fd: Arc<Mutex<RawFd>>,
    mut finish_request: RollupFinish,
    request_tx: &mpsc::Sender<RollupRequest>,
) -> std::io::Result<()> {
    let next_request_type = finish_request.next_request_type as u32;
    match next_request_type {
        rollup::CARTESI_ROLLUP_ADVANCE_STATE => {
            log::debug!("handle advance state request...");
            let advance_request = {
                let fd = rollup_fd.lock().await;
                // Read advance request from rollup device
                match rollup::rollup_read_advance_state_request(*fd, &mut finish_request) {
                    Ok(r) => r,
                    Err(e) => {
                        return Err(std::io::Error::new(ErrorKind::Other, e.to_string()));
                    }
                }
            };
            if log::log_enabled!(log::Level::Info) {
                rollup::print_advance(&advance_request);
            }
            // Send newly read advance request to http service
            if let Err(e) = request_tx
                .send(RollupRequest::Advance(advance_request))
                .await
            {
                log::error!(
                    "error passing advance request to http service {}",
                    e.to_string()
                );
            }
        }
        rollup::CARTESI_ROLLUP_INSPECT_STATE => {
            log::debug!("handle inspect state request...");
            // Read inspect request from rollup device
            let inspect_request = {
                let fd = rollup_fd.lock().await;
                match rollup::rollup_read_inspect_state_request(*fd, &mut finish_request) {
                    Ok(r) => r,
                    Err(e) => {
                        return Err(std::io::Error::new(ErrorKind::Other, e.to_string()));
                    }
                }
            };
            if log::log_enabled!(log::Level::Info) {
                rollup::print_inspect(&inspect_request);
            }
            // Send newly read inspect request to http service
            if let Err(e) = request_tx
                .send(RollupRequest::Inspect(inspect_request))
                .await
            {
                log::error!(
                    "error passing inspect request to http service {}",
                    e.to_string()
                );
            }
        }
        _ => {
            return Err(std::io::Error::new(
                ErrorKind::Unsupported,
                "request type unsupported",
            ));
        }
    }
    Ok(())
}

/// Async loop for reading rollup device
/// and processing advance/inspect request
async fn linux_rollup_loop(
    rollup_fd: Arc<Mutex<RawFd>>,
    mut finish_rx: mpsc::Receiver<RollupResponse>,
    request_tx: mpsc::Sender<RollupRequest>,
) {
    // Loop and wait for indication of pending advance/inspect request
    // got from last performed finish request
    log::debug!("entering linux rollup device loop");
    loop {
        log::debug!("waiting for finish request...");
        tokio::select! {
            Some(response) = finish_rx.recv() =>
            {
                match response {
                    RollupResponse::Finish(accept) => {
                        log::debug!(
                            "request finished, writing to driver result `{}` ...",
                            accept
                        );
                        // Write finish request, read indicator for next request
                        match perform_rollup_finish_request(&rollup_fd, accept).await {
                            Ok(finish_request) => {
                                // Received new request, process it
                                log::info!(
                                    "Received new request of type {}",
                                    match finish_request.next_request_type {
                                        0 => "ADVANCE",
                                        1 => "INSPECT",
                                        _ => "UNKNOWN",
                                    }
                                );
                                match handle_rollup_requests(rollup_fd.clone(), finish_request, &request_tx)
                                    .await
                                {
                                    Ok(_) => {}
                                    Err(e) => {
                                        log::error!(
                                            "error performing handle_rollup_requests: `{}`",
                                            e.to_string()
                                        );
                                    }
                                }
                            }
                            Err(e) => {
                                log::error!(
                                    "error performing initial finish request: `{}`",
                                    e.to_string()
                                );
                            }
                        }
                    }
                }
            }
        }
    }
}

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    let args: Vec<String> = std::env::args().collect();
    let program = args[0].clone();
    // Process command line arguments
    let mut opts = Options::new();
    opts.optflag("h", "help", "show this help message and exit");
    opts.optopt(
        "",
        "address",
        "Address to listen (default: 127.0.0.1:5004)",
        "",
    );
    opts.optopt("", "dapp", "Dapp address (default: 127.0.0.1:5003)", "");
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
    if matches.opt_present("verbose") {
        log_level = "debug";
    }
    // Set the global log level
    env_logger::Builder::from_env(env_logger::Env::default().default_filter_or(log_level)).init();

    log::info!("starting http dispatcher service...");

    // Create config
    let mut http_config = Config::new();
    {
        // Parse addresses and ports
        let address_matches = matches
            .opt_get_default("address", "127.0.0.1:5004".to_string())
            .unwrap_or_default();
        let mut address = address_matches.split(':');
        http_config.http_address = address.next().expect("address is not valid").to_string();
        http_config.http_port = address
            .next()
            .expect("port is not valid")
            .to_string()
            .parse::<u16>()
            .unwrap();
    }

    // Open rollup device
    let rollup_file = match File::open(rollup::ROLLUP_DEVICE_NAME) {
        Ok(file) => file,
        Err(e) => {
            log::error!("error opening rollup device {}", e.to_string());
            return Err(e);
        }
    };

    // Linux rollup must wait for the http service to initialize
    let (start_rollup_tx, start_rollup_rx): (
        std::sync::mpsc::Sender<bool>,
        std::sync::mpsc::Receiver<bool>,
    ) = std::sync::mpsc::channel();
    let rollup_fd: Arc<Mutex<RawFd>> = Arc::new(Mutex::new(rollup_file.into_raw_fd()));
    // Channel for communicating rollup finish requests between http service and linux rollup loop
    let (finish_tx, finish_rx) = mpsc::channel::<RollupResponse>(100);
    let (request_tx, request_rx) = mpsc::channel::<RollupRequest>(100);

    {
        // Start rollup advance/inspect requests handling loop
        let rollup_fd: Arc<Mutex<RawFd>> = rollup_fd.clone();
        tokio::spawn(async move {
            start_rollup_rx.recv().unwrap();
            linux_rollup_loop(rollup_fd, finish_rx, request_tx).await;
        });
    }

    // Open http service
    tokio::select! {
        result = http_service::run(&http_config, start_rollup_tx, rollup_fd, finish_tx, request_rx) => {
            match result {
                Ok(_) => log::info!("http service terminated successfully"),
                Err(e) => log::warn!("http service terminated with error: {}", e),
            }
        }
    }
    log::info!("ending http dispatcher service!");
    Ok(())
}
