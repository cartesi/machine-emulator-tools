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

use crate::model::{AdvanceResult, IndexResponse, Notice, Report, Voucher};

// Perform get request to this app inspect endpoint, return true
// if it is up and running
pub async fn probe_inspect_endpoint(http_service_addr: &str) -> bool {
    let client = hyper::Client::new();
    let uri = format!("http://{}/health", http_service_addr);
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

pub async fn send_voucher(proxy_addr: &str, voucher: Voucher) {
    let client = hyper::Client::new();
    let req = hyper::Request::builder()
        .method(hyper::Method::POST)
        .header(hyper::header::CONTENT_TYPE, "application/json")
        .uri(proxy_addr.to_string() + "/voucher")
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
            log::error!("failed to send voucher request to the proxy: {}", e);
        }
    }
}

pub async fn send_notice(proxy_addr: &str, notice: Notice) {
    let client = hyper::Client::new();
    let req = hyper::Request::builder()
        .method(hyper::Method::POST)
        .header(hyper::header::CONTENT_TYPE, "application/json")
        .uri(proxy_addr.to_string() + "/notice")
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
            log::error!("failed to send notice request to the proxy: {}", e);
        }
    }
}

pub async fn send_report(proxy_addr: &str, report: Report) {
    let client = hyper::Client::new();
    let req = hyper::Request::builder()
        .method(hyper::Method::POST)
        .header(hyper::header::CONTENT_TYPE, "application/json")
        .uri(proxy_addr.to_string() + "/report")
        .body(hyper::Body::from(serde_json::to_string(&report).unwrap()))
        .expect("report request");
    if let Err(e) = client.request(req).await {
        log::error!("failed to send report request to the proxy: {}", e);
    }
}

pub async fn send_finish_request(proxy_addr: &str, result: AdvanceResult) {
    // Application advance request resulting status
    let status = match result {
        Ok(()) => "accept",
        Err(e) => {
            log::error!("failed to advance state: {}", e);
            "reject"
        }
    };
    // Reconstruct http dispatcher finish target endpoint
    let proxy_endpoint = format!("http://{}/finish", proxy_addr);
    log::debug!("sending finish request to {}", proxy_endpoint);
    // Send finish request to target-proxy
    {
        let mut json_status = std::collections::HashMap::new();
        json_status.insert("status", status);
        let client = hyper::Client::new();
        // Prepare http request
        let req = hyper::Request::builder()
            .method(hyper::Method::POST)
            .header(hyper::header::CONTENT_TYPE, "application/json")
            .uri(proxy_endpoint)
            .body(hyper::Body::from(
                serde_json::to_string(&json_status).expect("status json"),
            ))
            .expect("finish request");
        // Send http request targeting target-proxy /finish endpoint
        if let Err(e) = client.request(req).await {
            log::error!("Failed to send `{}` response to the server: {}", status, e);
        }
    }
}
