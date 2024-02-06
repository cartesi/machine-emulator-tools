// Copyright Cartesi and individual authors (see AUTHORS)
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

extern crate rollup_http_client;
extern crate rollup_http_server;

use actix_server::ServerHandle;
use async_mutex::Mutex;
use rollup_http_client::rollup::{
    Exception, Notice, Report, RollupRequest, RollupResponse, Voucher,
};
use rollup_http_server::config::Config;
use rollup_http_server::rollup::RollupFd;
use rollup_http_server::*;
use rstest::*;
use std::fs::File;
use std::future::Future;
use std::sync::Arc;
use rand::Rng;
use std::env;

const HOST: &str = "127.0.0.1";

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
    let rollup_fd: Arc<Mutex<RollupFd>> = Arc::new(Mutex::new(RollupFd::create().unwrap()));
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
    let mut port;
    loop {
        port = rand::thread_rng().gen_range(49152..65535);

        match run_test_http_service(HOST, port) {
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
        address: format!("http://{}:{}", HOST, port),
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
    env::set_var("CMT_INPUTS", "0:advance.bin,1:inspect.bin");
    env::set_var("CMT_DEBUG", "yes");

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
                assert_eq!(advance_request.payload.len(), 10);
                assert_eq!(
                    advance_request.metadata.msg_sender,
                    "0x0000000000000000000000000000000000000000"
                );
                assert_eq!(
                    &advance_request.payload[2..],
                    "deadbeef"
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
                assert_eq!(inspect_request.payload.len(), 10);
                assert_eq!(
                    &inspect_request.payload[2..],
                    "deadbeef"
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
    env::set_var("CMT_INPUTS", "0:advance.bin,1:inspect.bin");
    env::set_var("CMT_DEBUG", "yes");

    let context = context_future.await;
    println!("Writing voucher");
    let test_voucher_01 = Voucher {
        destination: "0x1111111111111111111111111111111111111111".to_string(),
        data: "0x".to_string() + &hex::encode("voucher test payload 02"),
        payload: "0x".to_string() + &hex::encode("voucher test payload 01"),
    };
    let test_voucher_02 = Voucher {
        destination: "0x2222222222222222222222222222222222222222".to_string(),
        data: "0x".to_string() + &hex::encode("voucher test payload 02"),
        payload: "0x".to_string() + &hex::encode("voucher test payload 02"),
    };
    rollup_http_client::client::send_voucher(&context.address, test_voucher_01).await;

    let voucher1 =
         std::fs::read("none.output-0.bin").expect("error reading voucher 1 file");
        assert_eq!(
         voucher1,
         [35, 122, 129, 111, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 0, 0, 0, 0, 0, 0, 0, 0, 0, 118, 111, 117, 99, 104, 101, 114, 32, 116, 101, 115, 116, 32, 112, 97, 121, 108, 111, 97, 100, 32, 48, 49, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 96, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 23, 118, 111, 117, 99, 104, 101, 114, 32, 116, 101, 115, 116, 32, 112, 97, 121, 108, 111, 97, 100, 32, 48, 49, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    );
    std::fs::remove_file("none.output-0.bin")?;

    println!("Writing second voucher!");

    rollup_http_client::client::send_voucher(&context.address, test_voucher_02).await;
    context.server_handle.stop(true).await;

    //Read text file with results

    let voucher2 =
        std::fs::read("none.output-0.bin").expect("error reading voucher 2 file");
         assert_eq!(
         voucher2,
         [35, 122, 129, 111, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 0, 0, 0, 0, 0, 0, 0, 0, 0, 118, 111, 117, 99, 104, 101, 114, 32, 116, 101, 115, 116, 32, 112, 97, 121, 108, 111, 97, 100, 32, 48, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 96, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 23, 118, 111, 117, 99, 104, 101, 114, 32, 116, 101, 115, 116, 32, 112, 97, 121, 108, 111, 97, 100, 32, 48, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    );

    std::fs::remove_file("none.output-0.bin")?;
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
        std::fs::read("none.output-0.bin").expect("error reading test notice file");
    //assert_eq!(
    //    notice1,
    //    "index: 1, payload_size: 22, payload: notice test payload 01"
    //);
    std::fs::remove_file("none.output-0.bin")?;
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
        std::fs::read_to_string("none.report-0.bin").expect("error reading test report file");
    assert_eq!(
        report1,
        "report test payload 01"
    );
    std::fs::remove_file("none.report-0.bin")?;

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
        std::fs::read("none.exception-0.bin").expect("error reading test exception file");
    assert_eq!(
        exception,
        vec![0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    );
    println!("Removing exception text file");
    std::fs::remove_file("none.exception-0.bin")?;
    Ok(())
}
