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

use actix_web::ResponseError;
use serde::{Deserialize, Serialize};
use std::error::Error;
use std::fmt;
use tokio::sync::{mpsc, oneshot};

#[derive(Debug)]
pub struct SyncRequest<T: Send + Sync, U: Send + Sync> {
    pub value: T,
    pub response_tx: oneshot::Sender<U>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct AdvanceMetadata {
    pub address: String,
    pub epoch_number: u64,
    pub input_number: u64,
    pub block_number: u64,
    pub timestamp: u64,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct AdvanceRequest {
    pub metadata: AdvanceMetadata,
    pub payload: String,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct InspectRequest {
    pub payload: String,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct InspectReport {
    pub reports: Vec<Report>,
}

#[derive(Debug, PartialEq, Eq)]
pub struct AdvanceError {
    pub cause: String,
}

#[derive(Debug, PartialEq, Eq)]
pub struct InspectError {
    pub cause: String,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct Notice {
    pub payload: String,
    pub index: u64,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct Voucher {
    pub address: String,
    pub payload: String,
    pub index: u64,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct Report {
    pub payload: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
struct IndexResponse {
    index: u64,
}

pub type AdvanceResult = Result<(), AdvanceError>;
pub type InspectResult = Result<InspectReport, InspectError>;

pub type SyncInspectRequest = SyncRequest<InspectRequest, InspectResult>;

impl Error for AdvanceError {}
impl Error for InspectError {}
impl ResponseError for AdvanceError {}

impl fmt::Display for AdvanceError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "failed to advance ({})", self.cause)
    }
}

impl fmt::Display for InspectError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "failed to inspect ({})", self.cause)
    }
}

impl From<mpsc::error::SendError<SyncInspectRequest>> for InspectError {
    fn from(error: mpsc::error::SendError<SyncInspectRequest>) -> Self {
        InspectError {
            cause: error.to_string(),
        }
    }
}

pub struct TestEchoData {
    pub vouchers: u32,
    pub reports: u32,
    pub notices: u32,
    pub reject: i32,
}

pub struct Model {
    pub test_echo_data: TestEchoData,
    pub proxy_addr: String,
}

impl Model {
    // DApp logic
    pub fn new(proxy_address: &str, proxy_port: u16, test_echo_data: TestEchoData) -> Self {
        Self {
            proxy_addr: format!("http://{}:{}", proxy_address, proxy_port),
            test_echo_data,
        }
    }

    pub async fn advance(
        &mut self,
        request: AdvanceRequest,
        finish_tx: mpsc::Sender<AdvanceResult>,
    ) {
        log::debug!("dapp Received advance request {:?}", &request);
        let client = hyper::Client::new();
        // Generate test echo vouchers
        if self.test_echo_data.vouchers > 0 {
            log::debug!("generating {} echo vouchers", self.test_echo_data.vouchers);
            // Generate test vouchers
            for _ in 0..self.test_echo_data.vouchers {
                let voucher_payload = request.payload.clone();
                let voucher = Voucher {
                    address: request.metadata.address.clone(),
                    payload: voucher_payload,
                    index: 0,
                };
                let req = hyper::Request::builder()
                    .method(hyper::Method::POST)
                    .header(hyper::header::CONTENT_TYPE, "application/json")
                    .uri(self.proxy_addr.clone() + "/voucher")
                    .body(hyper::Body::from(serde_json::to_string(&voucher).unwrap()))
                    .expect("voucher request");
                match client.request(req).await {
                    Ok(res) => {
                        let id_response = serde_json::from_slice::<IndexResponse>(
                            &hyper::body::to_bytes(res)
                                .await
                                .expect("error in voucher ind response handling")
                                .to_vec(),
                        );
                        log::debug!("voucher generated: {:?}", &id_response);
                    }
                    Err(e) => {
                        log::error!("failed to send voucher request to the proxy: {}", e);
                    }
                }
            }
        }
        // Generate test echo notices
        if self.test_echo_data.notices > 0 {
            // Generate test notice
            log::debug!("Generating {} echo notices", self.test_echo_data.notices);
            for _ in 0..self.test_echo_data.notices {
                let notice_payload = request.payload.clone();
                let notice = Notice {
                    payload: notice_payload,
                    index: 0,
                };
                let req = hyper::Request::builder()
                    .method(hyper::Method::POST)
                    .header(hyper::header::CONTENT_TYPE, "application/json")
                    .uri(self.proxy_addr.clone() + "/notice")
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
        }
        // Generate test echo reports
        if self.test_echo_data.reports > 0 {
            // Generate test reports
            log::debug!("generating {} echo reports", self.test_echo_data.reports);
            for _ in 0..self.test_echo_data.reports {
                let report_payload = request.payload.clone();
                let report = Report {
                    payload: report_payload,
                };
                let req = hyper::Request::builder()
                    .method(hyper::Method::POST)
                    .header(hyper::header::CONTENT_TYPE, "application/json")
                    .uri(self.proxy_addr.clone() + "/report")
                    .body(hyper::Body::from(serde_json::to_string(&report).unwrap()))
                    .expect("report request");
                if let Err(e) = client.request(req).await {
                    log::error!("failed to send report request to the proxy: {}", e);
                }
            }
        }

        //Finish request
        finish_tx
            .send(if self.test_echo_data.reject == 0 {
                Err(AdvanceError {
                    cause: "rejected due to reject parameter".to_string(),
                })
            } else {
                Ok(())
            })
            .await
            .expect("failed to send finish request");

        // Decrease test reject counter
        if self.test_echo_data.reject >= 0 {
            self.test_echo_data.reject -= 1;
        }
    }
    pub async fn inspect(&mut self, request: InspectRequest) -> InspectResult {
        let mut inspect_report = InspectReport {
            reports: Vec::new(),
        };
        log::debug!("Dapp received inspect request {:?}", &request);
        if self.test_echo_data.reports > 0 {
            // Generate test reports for inspect
            log::debug!(
                "generating {} echo inspect state reports",
                self.test_echo_data.reports
            );
            for _ in 0..self.test_echo_data.reports {
                let report_payload = request.payload.clone();
                inspect_report.reports.push(Report {
                    payload: report_payload,
                });
            }
        }
        log::info!("inspect state result {:?}", &inspect_report);
        Ok(inspect_report)
    }
}
