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
pub struct InspectReport {}

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
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct Voucher {
    pub address: String,
    pub payload: String,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct Report {
    pub payload: String,
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
}

pub struct Model {
    pub test_echo_data: TestEchoData,
    pub proxy_addr: String
}

impl Model {
    // DApp logic
    pub fn new(proxy_address: &String, proxy_port: u16, test_echo_data: TestEchoData) -> Self {
        Self {
            proxy_addr : format!("http://{}:{}", proxy_address.clone(), proxy_port),
            test_echo_data }
    }

    pub async fn advance(
        &self,
        request: AdvanceRequest,
        finish_channel: mpsc::Sender<AdvanceResult>,
    ) {
        println!("Dapp Received advance request {:?}", &request);
        let client = reqwest::Client::new();

        if self.test_echo_data.vouchers > 0 {
            log::info!("Generating {} echo vouchers", self.test_echo_data.vouchers);
            //Generate test vouchers
            for _ in 0..self.test_echo_data.vouchers {
                let mut voucher_payload = request.payload.clone();
                voucher_payload.push_str("-voucher");
                let voucher = Voucher {
                    address: request.metadata.address.clone(),
                    payload: voucher_payload,
                };
                if let Err(e) = client
                    .post(self.proxy_addr.clone() + "/voucher")
                    .json(&voucher)
                    .send()
                    .await
                {
                    log::error!("Failed to send voucher request to the proxy: {}", e);
                }
            }
        }

        if self.test_echo_data.notices > 0 {
            // Generate test notice
            log::info!("Generating {} echo notices", self.test_echo_data.notices);
            for _ in 0..self.test_echo_data.notices {
                let mut notice_payload = request.payload.clone();
                notice_payload.push_str("-notice");
                let notice = Notice {
                    payload: notice_payload,
                };
                if let Err(e) = client
                    .post(self.proxy_addr.clone() + "/notice")
                    .json(&notice)
                    .send()
                    .await
                {
                    log::error!("Failed to send notice request to the proxy: {}", e);
                }
            }
        }

        if self.test_echo_data.reports > 0 {
            // Generate test reports
            log::info!("Generating {} echo reports", self.test_echo_data.reports);
            for _ in 0..self.test_echo_data.reports {
                let mut notice_payload = request.payload.clone();
                notice_payload.push_str("-report");
                let report = Report {
                    payload: notice_payload,
                };
                if let Err(e) = client
                    .post(self.proxy_addr.clone() + "/report")
                    .json(&report)
                    .send()
                    .await
                {
                    log::error!("Failed to send report request to the proxy: {}", e);
                }
            }
        }

        //Finish request
        log::info!("Sending finish request");
        finish_channel.send(Ok(())).await.expect("failed to send finish request");
    }
    pub async fn inspect(&self, request: InspectRequest) -> InspectResult {
        println!("Dapp Received inspect request {:?}", &request);
        Ok(InspectReport {})
    }
}
