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

/// Controller component handles application and data flow
use tokio::sync::{mpsc, oneshot};

use crate::config::Config;
use crate::model::{
    AdvanceError, AdvanceRequest, AdvanceResult, InspectError, InspectRequest, InspectResult,
    Model, SyncInspectRequest,
};

/// Current decentralized application state
/// Application could be in idle state, or upon receiving
/// advance request from http dispatcher it performs some work,
/// generating vouchers, notices and reports
/// and finishes advance state by calling finish target-proxy endpoint
enum State {
    Idle(IdleState),
    Advancing(AdvancingState),
}

/// Implementation of DApp flow when application is in
/// idle state
struct IdleState {
    model: Box<Model>,
    finish_tx: mpsc::Sender<AdvanceResult>,
}
/// Implementation of DApp flow when application is in
/// advancing state
struct AdvancingState {
    model: Box<Model>,
    finish_tx: mpsc::Sender<AdvanceResult>,
}

impl IdleState {
    /// Handle asynchronic advance request
    async fn advance(mut self, request: AdvanceRequest) -> State {
        // Call custom DApp advance state execution logic
        self.model.advance(request, self.finish_tx.clone()).await;
        // Next application state
        State::Advancing(AdvancingState {
            model: self.model,
            finish_tx: self.finish_tx,
        })
    }
    /// Handle synchronic inspect request
    async fn inspect(mut self, request: SyncInspectRequest) -> State {
        // Calls model inspect state implementation to gather DApp current status info
        let response = self.model.inspect(request.value).await.map_err(|e| {
            log::error!("failed to inspect with error: {}", e);
            InspectError {
                cause: e.to_string(),
            }
        });
        // Pass result to controller channel
        request
            .response_tx
            .send(response)
            .expect("send should not fail");
        State::Idle(self)
    }
}

impl AdvancingState {
    /// Handle finish request to target-proxy, prepare `accept`/`reject` result
    async fn finish(self, result: AdvanceResult, config: &Config) -> State {
        log::debug!("processing finish request; setting state to idle");
        // Application advance request resulting status
        let status = match result {
            Ok(()) => "accept",
            Err(e) => {
                log::error!("failed to advance state: {}", e);
                "reject"
            }
        };
        // Reconstruct http dispatcher http address
        let proxy_addr = format!(
            "http://{}:{}/finish",
            config.dispatcher_address.clone(),
            config.dispatcher_port
        );
        log::debug!("sending finish request to {}", proxy_addr);
        // Send finish request to target-proxy
        {
            let mut json_status = std::collections::HashMap::new();
            json_status.insert("status", status);
            let client = hyper::Client::new();
            // Prepare http request
            let req = hyper::Request::builder()
                .method(hyper::Method::POST)
                .header(hyper::header::CONTENT_TYPE, "application/json")
                .uri(proxy_addr)
                .body(hyper::Body::from(
                    serde_json::to_string(&json_status).expect("status json"),
                ))
                .expect("finish request");
            // Send http request targeting target-proxy /finish endpoint
            if let Err(e) = client.request(req).await {
                log::error!("Failed to send `{}` response to the server: {}", status, e);
            }
        }
        // Next application state
        State::Idle(IdleState {
            model: self.model,
            finish_tx: self.finish_tx,
        })
    }
}

#[derive(Debug, Clone)]
pub struct ControllerChannel {
    advance_tx: mpsc::Sender<AdvanceRequest>,
    inspect_tx: mpsc::Sender<SyncInspectRequest>,
}

/// Implement processing of http API requests and responses
/// Pass requests to controller service
impl ControllerChannel {
    /// Process advance async request
    pub async fn process_advance(&self, request: AdvanceRequest) -> Result<(), AdvanceError> {
        log::debug!("Advance request received: {:#?}", request);
        if let Err(e) = self.advance_tx.send(request).await {
            return Err(AdvanceError {
                cause: e.to_string(),
            });
        }
        Ok(())
    }

    /// Process inspect sync request
    pub async fn process_inspect(
        &self,
        request: InspectRequest,
    ) -> Result<InspectResult, InspectError> {
        let (response_tx, response_rx) = oneshot::channel();
        log::debug!("Inspect request received: {:#?}", request);
        // Pass request
        self.inspect_tx
            .send(SyncInspectRequest {
                value: request,
                response_tx,
            })
            .await?;
        // Await for inspect report info
        match response_rx.await {
            Ok(val) => Ok(val),
            Err(e) => Err(InspectError {
                cause: e.to_string(),
            }),
        }
    }
}

/// Service implementing actual model execution of inspect/advance requests
///
pub struct ControllerService {
    state: State,
    config: Config,
    advance_rx: mpsc::Receiver<AdvanceRequest>,
    finish_rx: mpsc::Receiver<AdvanceResult>,
    inspect_rx: mpsc::Receiver<SyncInspectRequest>,
}

impl ControllerService {
    pub async fn run(mut self) {
        // Enter loop to process advance and inspect requests from target-proxy
        loop {
            self.state = match self.state {
                State::Idle(idle) => {
                    // Wait for the next request asynchronously
                    tokio::select! {
                        Some(request) = self.advance_rx.recv() => {
                            idle.advance(request).await
                        }
                        Some(request) = self.inspect_rx.recv() => {
                            idle.inspect(request).await
                        }
                    }
                }
                State::Advancing(advancing) => {
                    if let Some(result) = self.finish_rx.recv().await {
                        // Execute finish request generation
                        advancing.finish(result, &self.config).await
                    } else {
                        log::error!("Advance result should not be none");
                        State::Idle(IdleState {
                            model: advancing.model,
                            finish_tx: advancing.finish_tx,
                        })
                    }
                }
            }
        }
    }
}

/// Constructor for controller channel and service
pub fn new_controller(model: Box<Model>, config: Config) -> (ControllerChannel, ControllerService) {
    let (advance_tx, advance_rx) = mpsc::channel::<AdvanceRequest>(1000);
    let (finish_tx, finish_rx) = mpsc::channel::<AdvanceResult>(1000);
    let (inspect_tx, inspect_rx) = mpsc::channel::<SyncInspectRequest>(1000);
    let service = ControllerService {
        state: State::Idle(IdleState { model, finish_tx }),
        config,
        advance_rx,
        finish_rx,
        inspect_rx,
    };
    let channel = ControllerChannel {
        advance_tx,
        inspect_tx,
    };

    (channel, service)
}
