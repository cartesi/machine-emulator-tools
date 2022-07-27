// Copyright (C) 2022 Cartesi Pte. Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use
// this file except in compliance with the License. You may obtain a copy of the
// License at http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

// Run tests with
// USE_ROLLUP_BINDINGS_MOCK=1 cargo test -- --show-output --test-threads=1

extern crate rollup_http_client;
extern crate rollup_http_server;

use actix_server::ServerHandle;
use async_mutex::Mutex;
use rollup_http_client::rollup::{
    Exception, Notice, Report, RollupRequest, RollupResponse, Voucher,
};
use rollup_http_server::config::Config;
use rollup_http_server::*;
use rstest::*;
use std::fs::File;
use std::future::Future;
use std::os::unix::io::{IntoRawFd, RawFd};
use std::sync::Arc;

const PORT: u16 = 10010;
const HOST: &str = "127.0.0.1";
const TEST_ROLLUP_DEVICE: &str = "rollup_driver.bin";

#[allow(dead_code)]
struct Context {
    address: String,
    server_handle: actix_server::ServerHandle,
}

impl Drop for Context {
    fn drop(&mut self) {
        // Shut down http server+
        println!("shutting down http service in drop cleanup");
    }
}

fn run_test_http_service(
    host: &str,
    port: u16,
) -> std::io::Result<Option<actix_server::ServerHandle>> {
    println!("Opening rollup device");
    // Open test rollup device
    let rollup_file = match File::create(TEST_ROLLUP_DEVICE) {
        Ok(file) => file,
        Err(e) => {
            log::error!("error opening rollup device {}", e.to_string());
            return Err(e);
        }
    };

    let rollup_fd: Arc<Mutex<RawFd>> = Arc::new(Mutex::new(rollup_file.into_raw_fd()));
    let rollup_fd = rollup_fd.clone();
    let http_config = Config {
        http_address: host.to_string(),
        http_port: port,
    };
    println!("Creating http server");
    let server = http_service::create_server(&http_config, rollup_fd)?;
    let server_handle = server.handle();
    println!("Spawning http server");
    tokio::spawn(server);
    println!("Http server spawned");
    Ok(Some(server_handle))
}

#[fixture]
async fn context_future() -> Context {
    let mut server_handle: Option<ServerHandle> = None;
    let mut count = 5;
    loop {
        match run_test_http_service(HOST, PORT) {
            Ok(handle) => {
                server_handle = handle;
                break;
            }
            Err(ex) => {
                eprint!("Error instantiating rollup http service {}", ex.to_string());
                if count > 0 {
                    // wait for the system to free port
                    std::thread::sleep(std::time::Duration::from_secs(1));
                } else {
                    break;
                }
            }
        };
        count = count - 1;
    }

    Context {
        address: format!("http://{}:{}", HOST, PORT),
        server_handle: server_handle.unwrap(),
    }
}

#[rstest]
#[tokio::test]
async fn test_server_instance_creation(
    context_future: impl Future<Output = Context>,
) -> Result<(), Box<dyn std::error::Error>> {
    let context = context_future.await;
    println!("Sleeping in the test... ");
    std::thread::sleep(std::time::Duration::from_secs(5));
    println!("End sleeping");
    println!("Shutting down http service");
    context.server_handle.stop(true).await;
    println!("Http server closed");
    Ok(())
}

#[rstest]
#[tokio::test]
async fn test_finish_request(
    context_future: impl Future<Output = Context>,
) -> Result<(), Box<dyn std::error::Error>> {
    let context = context_future.await;
    println!("Sending finish request");
    let request_response = RollupResponse::Finish(true);
    match rollup_http_client::client::send_finish_request(&context.address, &request_response).await
    {
        Ok(request) => match request {
            RollupRequest::Inspect(_inspect_request) => {
                context.server_handle.stop(true).await;
                panic!("Got unexpected request")
            }
            RollupRequest::Advance(advance_request) => {
                println!("Got new advance request: {:?}", advance_request);
                assert_eq!(advance_request.payload.len(), 42);
                assert_eq!(
                    advance_request.metadata.msg_sender,
                    "0x1111111111111111111111111111111111111111"
                );
                assert_eq!(
                    std::str::from_utf8(&hex::decode(&advance_request.payload[2..]).unwrap())
                        .unwrap(),
                    "test advance request"
                );
            }
        },
        Err(err) => {
            context.server_handle.stop(true).await;
            return Err(Box::new(err));
        }
    }
    match rollup_http_client::client::send_finish_request(&context.address, &request_response).await
    {
        Ok(request) => match request {
            RollupRequest::Inspect(inspect_request) => {
                println!("Got new inspect request: {:?}", inspect_request);
                context.server_handle.stop(true).await;
                assert_eq!(inspect_request.payload.len(), 42);
                assert_eq!(
                    std::str::from_utf8(&hex::decode(&inspect_request.payload[2..]).unwrap())
                        .unwrap(),
                    "test inspect request"
                );
            }
            RollupRequest::Advance(_advance_request) => {
                context.server_handle.stop(true).await;
                panic!("Got unexpected request")
            }
        },
        Err(err) => {
            context.server_handle.stop(true).await;
            return Err(Box::new(err));
        }
    }
    context.server_handle.stop(true).await;
    Ok(())
}

#[rstest]
#[tokio::test]
async fn test_write_voucher(
    context_future: impl Future<Output = Context>,
) -> Result<(), Box<dyn std::error::Error>> {
    let context = context_future.await;
    println!("Writing voucher");
    let test_voucher_01 = Voucher {
        address: "0x1111111111111111111111111111111111111111".to_string(),
        payload: "0x".to_string() + &hex::encode("voucher test payload 01"),
    };
    let test_voucher_02 = Voucher {
        address: "0x2222222222222222222222222222222222222222".to_string(),
        payload: "0x".to_string() + &hex::encode("voucher test payload 02"),
    };
    rollup_http_client::client::send_voucher(&context.address, test_voucher_01).await;
    println!("Writing second voucher!");
    rollup_http_client::client::send_voucher(&context.address, test_voucher_02).await;
    context.server_handle.stop(true).await;

    //Read text file with results
    let voucher1 =
        std::fs::read_to_string("test_voucher_1.txt").expect("error reading voucher 1 file");
    assert_eq!(
        voucher1,
        "index: 1, payload_size: 23, payload: voucher test payload 01"
    );
    std::fs::remove_file("test_voucher_1.txt")?;

    let voucher2 =
        std::fs::read_to_string("test_voucher_2.txt").expect("error reading voucher 2 file");
    assert_eq!(
        voucher2,
        "index: 2, payload_size: 23, payload: voucher test payload 02"
    );
    std::fs::remove_file("test_voucher_2.txt")?;
    Ok(())
}

#[rstest]
#[tokio::test]
async fn test_write_notice(
    context_future: impl Future<Output = Context>,
) -> Result<(), Box<dyn std::error::Error>> {
    let context = context_future.await;
    // Set the global log level
    println!("Writing notice");
    let test_notice = Notice {
        payload: "0x".to_string() + &hex::encode("notice test payload 01"),
    };
    rollup_http_client::client::send_notice(&context.address, test_notice).await;
    context.server_handle.stop(true).await;
    //Read text file with results
    let notice1 =
        std::fs::read_to_string("test_notice_1.txt").expect("error reading test notice file");
    assert_eq!(
        notice1,
        "index: 1, payload_size: 22, payload: notice test payload 01"
    );
    std::fs::remove_file("test_notice_1.txt")?;
    Ok(())
}

#[rstest]
#[tokio::test]
async fn test_write_report(
    context_future: impl Future<Output = Context>,
) -> Result<(), Box<dyn std::error::Error>> {
    let context = context_future.await;
    // Set the global log level
    println!("Writing report");
    let test_report = Report {
        payload: "0x".to_string() + &hex::encode("report test payload 01"),
    };
    rollup_http_client::client::send_report(&context.address, test_report).await;
    context.server_handle.stop(true).await;
    //Read text file with results
    let report1 =
        std::fs::read_to_string("test_report_1.txt").expect("error reading test report file");
    assert_eq!(
        report1,
        "index: 1, payload_size: 22, payload: report test payload 01"
    );
    std::fs::remove_file("test_report_1.txt")?;

    Ok(())
}

#[rstest]
#[tokio::test]
async fn test_exception_throw(
    context_future: impl Future<Output = Context>,
) -> Result<(), Box<dyn std::error::Error>> {
    let context = context_future.await;
    // Set the global log level
    println!("Throwing exception");
    let test_exception = Exception {
        payload: "0x".to_string() + &hex::encode("exception test payload 01"),
    };
    rollup_http_client::client::throw_exception(&context.address, test_exception).await;
    println!("Closing server after throw exception");
    context.server_handle.stop(true).await;
    println!("Server closed");
    //Read text file with results
    let exception =
        std::fs::read_to_string("test_exception_1.txt").expect("error reading test exception file");
    assert_eq!(
        exception,
        "index: 1, payload_size: 25, payload: exception test payload 01"
    );
    println!("Removing exception text file");
    std::fs::remove_file("test_exception_1.txt")?;
    Ok(())
}
