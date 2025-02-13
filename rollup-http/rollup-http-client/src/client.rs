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

use crate::rollup::{
    AdvanceRequest, Exception, GIORequest, IndexResponse, InspectRequest, Notice, Report,
    RollupRequest, RollupResponse, Voucher, DelegateCallVoucher,
};
use hyper::Response;
use serde::{Deserialize, Serialize};
use std::io::ErrorKind;

#[derive(Debug, Serialize, Deserialize)]
#[serde(tag = "request_type")]
enum RollupHttpRequest {
    #[serde(rename = "advance_state")]
    Advance { data: AdvanceRequest },
    #[serde(rename = "inspect_state")]
    Inspect { data: InspectRequest },
}

pub async fn send_voucher(rollup_http_server_addr: &str, voucher: Voucher) {
    log::debug!("sending voucher request to {}", rollup_http_server_addr);
    let client = hyper::Client::new();
    let req = hyper::Request::builder()
        .method(hyper::Method::POST)
        .header(hyper::header::CONTENT_TYPE, "application/json")
        .uri(rollup_http_server_addr.to_string() + "/voucher")
        .body(hyper::Body::from(serde_json::to_string(&voucher).unwrap()))
        .expect("voucher request");
    match client.request(req).await {
        Ok(res) => {
            let id_response = serde_json::from_slice::<IndexResponse>(
                &hyper::body::to_bytes(res)
                    .await
                    .expect("error in voucher in response handling")
                    .to_vec(),
            );
            log::debug!("voucher generated: {:?}", &id_response);
        }
        Err(e) => {
            log::error!(
                "failed to send voucher request to rollup http server: {}",
                e
            );
        }
    }
}

pub async fn send_delegate_call_voucher(rollup_http_server_addr: &str, delegate_call_voucher: DelegateCallVoucher) {
    log::debug!("sending delegate call voucher request to {}", rollup_http_server_addr);
    let client = hyper::Client::new();
    let req = hyper::Request::builder()
        .method(hyper::Method::POST)
        .header(hyper::header::CONTENT_TYPE, "application/json")
        .uri(rollup_http_server_addr.to_string() + "/delegate-call-voucher")
        .body(hyper::Body::from(serde_json::to_string(&delegate_call_voucher).unwrap()))
        .expect("delegate call voucher request");
    match client.request(req).await {
        Ok(res) => {
            let id_response = serde_json::from_slice::<IndexResponse>(
                &hyper::body::to_bytes(res)
                    .await
                    .expect("error in voucher in response handling")
                    .to_vec(),
            );
            log::debug!("voucher generated: {:?}", &id_response);
        }
        Err(e) => {
            log::error!(
                "failed to send delegate call voucher request to rollup http server: {}",
                e
            );
        }
    }
}

pub async fn send_notice(rollup_http_server_addr: &str, notice: Notice) {
    log::debug!("sending notice request to {}", rollup_http_server_addr);
    let client = hyper::Client::new();
    let req = hyper::Request::builder()
        .method(hyper::Method::POST)
        .header(hyper::header::CONTENT_TYPE, "application/json")
        .uri(rollup_http_server_addr.to_string() + "/notice")
        .body(hyper::Body::from(serde_json::to_string(&notice).unwrap()))
        .expect("notice request");
    match client.request(req).await {
        Ok(res) => {
            let id_response = serde_json::from_slice::<IndexResponse>(
                &hyper::body::to_bytes(res)
                    .await
                    .expect("error in notice id response handling")
                    .to_vec(),
            );
            log::debug!("notice generated: {:?}", &id_response);
        }
        Err(e) => {
            log::error!("failed to send notice request to rollup http server: {}", e);
        }
    }
}

pub async fn send_report(rollup_http_server_addr: &str, report: Report) {
    log::debug!("sending report request to {}", rollup_http_server_addr);
    let client = hyper::Client::new();
    let req = hyper::Request::builder()
        .method(hyper::Method::POST)
        .header(hyper::header::CONTENT_TYPE, "application/json")
        .uri(rollup_http_server_addr.to_string() + "/report")
        .body(hyper::Body::from(serde_json::to_string(&report).unwrap()))
        .expect("report request");
    if let Err(e) = client.request(req).await {
        log::error!("failed to send report request to rollup http server: {}", e);
    }
}

pub async fn send_gio_request(
    rollup_http_server_addr: &str,
    gio_request: GIORequest,
) -> Response<hyper::Body> {
    log::debug!("sending gio request to {}", rollup_http_server_addr);
    let client = hyper::Client::new();
    let req = hyper::Request::builder()
        .method(hyper::Method::POST)
        .header(hyper::header::CONTENT_TYPE, "application/json")
        .uri(rollup_http_server_addr.to_string() + "/gio")
        .body(hyper::Body::from(
            serde_json::to_string(&gio_request).unwrap(),
        ))
        .expect("gio request");
    match client.request(req).await {
        Ok(res) => {
            log::info!("got gio response: {:?}", res);
            res
        }
        Err(e) => {
            log::error!("failed to send gio request to rollup http server: {}", e);
            Response::builder()
                .status(500)
                .body(hyper::Body::empty())
                .unwrap()
        }
    }
}

pub async fn throw_exception(rollup_http_server_addr: &str, exception: Exception) {
    log::debug!("throwing exception request to {}", rollup_http_server_addr);
    let client = hyper::Client::new();
    let req = hyper::Request::builder()
        .method(hyper::Method::POST)
        .header(hyper::header::CONTENT_TYPE, "application/json")
        .uri(rollup_http_server_addr.to_string() + "/exception")
        .body(hyper::Body::from(
            serde_json::to_string(&exception).unwrap(),
        ))
        .expect("exception request");
    if let Err(e) = client.request(req).await {
        log::error!(
            "failed to send exception throw request to rollup http server : {}",
            e
        );
    }
    // Here it doesn't matter what application does, as server manager
    // will terminate machine execution
    #[cfg(target_arch = "riscv64")]
    {
        panic!("exception happened due to exception parameter!");
    }
}

pub async fn send_finish_request(
    rollup_http_server_addr: &str,
    result: &RollupResponse,
) -> Result<RollupRequest, std::io::Error> {
    // Application advance request resulting status
    let status = match result {
        RollupResponse::Finish(value) => {
            if *value {
                "accept"
            } else {
                "reject"
            }
        }
    };
    // Reconstruct http dispatcher finish target endpoint
    let rollup_http_server_endpoint = format!("{}/finish", rollup_http_server_addr);
    log::debug!("sending finish request to {}", rollup_http_server_endpoint);
    // Send finish request to rollup http server
    {
        let mut json_status = std::collections::HashMap::new();
        json_status.insert("status", status);
        let client = hyper::Client::new();
        // Prepare http request
        let req = hyper::Request::builder()
            .method(hyper::Method::POST)
            .header(hyper::header::CONTENT_TYPE, "application/json")
            .uri(rollup_http_server_endpoint)
            .body(hyper::Body::from(
                serde_json::to_string(&json_status).expect("status json"),
            ))
            .expect("finish request");

        // Send http request targeting target-proxy /finish endpoint
        // And parse response with the new advance/inspect request
        match client.request(req).await {
            Ok(res) => {
                if res.status().is_success() {
                    // Handle Rollup Http Request received in json body
                    let buf = hyper::body::to_bytes(res)
                        .await
                        .expect("error in rollup http server response handling")
                        .to_vec();
                    let finish_response = serde_json::from_slice::<RollupHttpRequest>(&buf)
                        .expect("rollup http server response deserialization failed");
                    log::debug!(
                        "rollup http request finish response: {:?}",
                        &finish_response
                    );

                    match finish_response {
                        RollupHttpRequest::Advance {
                            data: advance_request,
                        } => Ok(RollupRequest::Advance(advance_request)),
                        RollupHttpRequest::Inspect {
                            data: inspect_request,
                        } => Ok(RollupRequest::Inspect(inspect_request)),
                    }
                } else {
                    // Rollup http server returned error on finish request
                    // Handle error message received in plain http response
                    let finish_error = String::from_utf8(
                        hyper::body::to_bytes(res)
                            .await
                            .expect("error in rollup http server finish response handling")
                            .into_iter()
                            .collect(),
                    )
                    .expect("failed to decode message");

                    Err(std::io::Error::new(ErrorKind::Other, finish_error))
                }
            }
            Err(e) => {
                log::error!("Failed to send `{}` response to the server: {}", status, e);
                Err(std::io::Error::new(ErrorKind::Other, e.to_string()))
            }
        }
    }
}
