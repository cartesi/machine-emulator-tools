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
    Exception, Notice, Report, RollupRequest, RollupResponse, Voucher, GIORequest,
};
use rollup_http_server::config::Config;
use rollup_http_server::rollup::RollupFd;
use rollup_http_server::*;
use rstest::*;
use std::future::Future;
use std::sync::Arc;
use rand::Rng;
use std::env;
use std::fs::File;
use std::io::Write;

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
    /*
     * cast calldata "EvmAdvance(uint256,address,address,uint256,uint256,uint256,bytes)" \
     *     0x0000000000000000000000000000000000000001 \
     *     0x0000000000000000000000000000000000000002 \
     *     0x0000000000000000000000000000000000000003 \
     *     0x0000000000000000000000000000000000000004 \
     *     0x0000000000000000000000000000000000000005 \
     *     0x0000000000000000000000000000000000000006 \
     *     0x`echo "advance-0" | xxd -p -c0`
     */
    let advance_payload_field = "advance-0\n"; // must match `cast` invocation!
    let advance_payload_data = "cc7dee1f\
                                0000000000000000000000000000000000000000000000000000000000000001\
                                0000000000000000000000000000000000000000000000000000000000000002\
                                0000000000000000000000000000000000000000000000000000000000000003\
                                0000000000000000000000000000000000000000000000000000000000000004\
                                0000000000000000000000000000000000000000000000000000000000000005\
                                0000000000000000000000000000000000000000000000000000000000000006\
                                00000000000000000000000000000000000000000000000000000000000000e0\
                                000000000000000000000000000000000000000000000000000000000000000a\
                                616476616e63652d300a00000000000000000000000000000000000000000000";

    /*
     * inspect requests are not evm encoded
     */
    let inspect_payload_data = "inspect-0";

    let advance_binary_data = hex::decode(advance_payload_data).unwrap();
    let advance_path = "advance_payload.bin";
    let mut advance_file = File::create(advance_path)?;
    advance_file.write_all(&advance_binary_data)?;

    let inspect_binary_data = inspect_payload_data.as_bytes();
    let inspect_path = "inspect_payload.bin";
    let mut inspect_file = File::create(inspect_path)?;
    inspect_file.write_all(&inspect_binary_data)?;

    env::set_var("CMT_INPUTS", format!("0:{0},1:{1}", advance_path, inspect_path));
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
                assert_eq!(
                    advance_request.metadata.msg_sender,
                    "0x0000000000000000000000000000000000000003"
                );

                let payload_bytes = hex::decode(&advance_request.payload[2..]).unwrap();
                let payload_string = String::from_utf8(payload_bytes).unwrap();
                assert_eq!(payload_string, advance_payload_field);
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

                let payload_bytes = hex::decode(&inspect_request.payload[2..]).unwrap();
                let payload_string = String::from_utf8(payload_bytes).unwrap();
                assert_eq!(payload_string, inspect_payload_data);
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

    std::fs::remove_file(advance_path)?;
    std::fs::remove_file(inspect_path)?;

    Ok(())
}

fn check_voucher_or_fail(
    original_voucher: Voucher,
    output_filename: &str,
) {
    // we try to decode the produced voucher with a third-party lib to see if it matches
    // the expected values
    let data =
         std::fs::read(output_filename).expect("error reading voucher file");
    let decoded_voucher = ethabi::decode(
        &[ethabi::ParamType::Address, ethabi::ParamType::Uint(256), ethabi::ParamType::Bytes],
        &data[4..], // skip the first 4 bytes that are the function signature
    ).ok().unwrap();

    assert_eq!(
        "0x".to_string() + &decoded_voucher[0].to_string(),
        original_voucher.destination,
    );
    assert_eq!(
        "0x".to_string() + &decoded_voucher[1].to_string(),
        original_voucher.value,
    );
    assert_eq!(
        "0x".to_string() + &decoded_voucher[2].to_string(),
        original_voucher.payload,
    );
}

#[rstest]
#[tokio::test]
async fn test_write_voucher(
    context_future: impl Future<Output = Context>,
) -> Result<(), Box<dyn std::error::Error>> {
    let context = context_future.await;
    println!("Writing voucher");
    let test_voucher_01 = Voucher {
        destination: "0x1111111111111111111111111111111111111111".to_string(),
        value: "0xdeadbeef".to_string(),
        payload: "0x".to_string() + &hex::encode("voucher test payload 01"),
    };
    let test_voucher_02 = Voucher {
        destination: "0x2222222222222222222222222222222222222222".to_string(),
        value: "0xdeadbeef".to_string(),
        payload: "0x".to_string() + &hex::encode("voucher test payload 02"),
    };
    rollup_http_client::client::send_voucher(&context.address, test_voucher_01.clone()).await;

    check_voucher_or_fail(test_voucher_01, "none.output-0.bin");
    std::fs::remove_file("none.output-0.bin")?;

    println!("Writing second voucher!");

    rollup_http_client::client::send_voucher(&context.address, test_voucher_02.clone()).await;
    context.server_handle.stop(true).await;

    check_voucher_or_fail(test_voucher_02, "none.output-1.bin");
    std::fs::remove_file("none.output-1.bin")?;

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
    rollup_http_client::client::send_notice(&context.address, test_notice.clone()).await;
    context.server_handle.stop(true).await;

    // we try to decode the produced voucher with a third-party lib to see if it matches
    // the expected values
    let data =
        std::fs::read("none.output-0.bin").expect("error reading test notice file");
    let decoded_notice = ethabi::decode(
        &[ethabi::ParamType::Bytes],
        &data[4..], // skip the first 4 bytes that are the function signature
    ).ok().unwrap();

    assert_eq!(
        "0x".to_string() + &decoded_notice[0].to_string(),
        test_notice.payload,
    );
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
async fn test_gio_request(
    context_future: impl Future<Output = Context>,
) -> Result<(), Box<dyn std::error::Error>> {
    let context = context_future.await;
    // Set the global log level
    println!("Sending gio request");
    let test_gio_request = GIORequest {
        domain: 0x100,
        payload: "0x".to_string() + &hex::encode("gio test payload 01"),
    };
    rollup_http_client::client::send_gio_request(&context.address, test_gio_request.clone()).await;
    context.server_handle.stop(true).await;

    //Read text file with results
    let gio =
        std::fs::read_to_string("none.gio-0.bin").expect("error reading test gio file");
    assert_eq!(
        gio,
        "gio test payload 01"
    );
    std::fs::remove_file("none.gio-0.bin")?;

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
    let contents = "exception test payload 01";
    let test_exception = Exception {
        payload: "0x".to_string() + &hex::encode(contents),
    };
    rollup_http_client::client::throw_exception(&context.address, test_exception).await;
    println!("Closing server after throw exception");
    context.server_handle.stop(true).await;
    println!("Server closed");
    //Read text file with results
    let exception =
        std::fs::read("none.exception-0.bin").expect("error reading test exception file");
    assert_eq!(
        String::from_utf8(exception).unwrap(),
        contents,
    );
    println!("Removing exception text file");
    std::fs::remove_file("none.exception-0.bin")?;
    Ok(())
}
