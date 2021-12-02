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

use tokio::sync::{mpsc, oneshot};

use crate::config::Config;
use crate::model::{
    AdvanceError, AdvanceRequest, AdvanceResult, InspectError, InspectRequest, InspectResult,
    Model, SyncInspectRequest,
};

enum State {
    Idle(IdleState),
    Advancing(AdvancingState),
}

struct IdleState {
    model: Box<Model>,
    finish_tx: mpsc::Sender<AdvanceResult>,
}
struct AdvancingState {
    model: Box<Model>,
    finish_tx: mpsc::Sender<AdvanceResult>,
}

impl IdleState {
    async fn advance(self, request: AdvanceRequest) -> State {
        self.model.advance(request, self.finish_tx.clone()).await;
        log::debug!("setting state to advance");
        State::Advancing(AdvancingState {
            model: self.model,
            finish_tx: self.finish_tx,
        })
    }
    async fn inspect(self, request: SyncInspectRequest) -> State {
        log::debug!("processing inspect request");
        let response = self.model.inspect(request.value).await.map_err(|e| {
            log::error!("failed to inspect with error: {}", e);
            InspectError {
                cause: e.to_string(),
            }
        });
        request
            .response_tx
            .send(response)
            .expect("send should not fail");
        State::Idle(self)
    }
}

impl AdvancingState {
    async fn finish(self, result: AdvanceResult, config: &Config) -> State {
        log::debug!("processing finish request; setting state to idle");
        let status = match result {
            Ok(()) => "accept",
            Err(e) => {
                log::error!("Failed to advance state: {}", e);
                "reject"
            }
        };
        let proxy_addr = format!("http://{}:{}/finish", config.proxy_address.clone(), config.proxy_port);
        log::debug!("Sending finish request to {}", proxy_addr);
        let client = reqwest::Client::new();
        let mut json_status = std::collections::HashMap::new();
        json_status.insert("status", status.clone());
        if let Err(e) = client
            .post(proxy_addr)
            .json(&json_status)
            .send()
            .await
        {
            log::error!("Failed to send `{}` response to the server: {}", status, e);
        }

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

impl ControllerChannel {
    pub async fn process_advance(&self, request: AdvanceRequest) -> Result<(), AdvanceError> {
        if let Err(e) = self.advance_tx.send(request).await {
            return Err(AdvanceError {
                cause: e.to_string(),
            });
        }
        Ok(())
    }

    pub async fn process_inspect(
        &self,
        request: InspectRequest,
    ) -> Result<InspectResult, InspectError> {
        let (response_tx, response_rx) = oneshot::channel();
        self.inspect_tx
            .send(SyncInspectRequest {
                value: request,
                response_tx,
            })
            .await?;
        match response_rx.await {
            Ok(val) => Ok(val),
            Err(e) => Err(InspectError {
                cause: e.to_string(),
            }),
        }
    }
}

pub struct ControllerService {
    state: State,
    config: Config,
    advance_rx: mpsc::Receiver<AdvanceRequest>,
    finish_rx: mpsc::Receiver<AdvanceResult>,
    inspect_rx: mpsc::Receiver<SyncInspectRequest>,
}

impl ControllerService {
    pub async fn run(mut self) {
        loop {
            self.state = match self.state {
                State::Idle(idle) => {
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

pub fn new_controller(model: Box<Model>, config: Config) -> (ControllerChannel, ControllerService) {
    let (advance_tx, advance_rx) = mpsc::channel::<AdvanceRequest>(1000);
    let (finish_tx, finish_rx) = mpsc::channel::<AdvanceResult>(1000);
    let (inspect_tx, inspect_rx) = mpsc::channel::<SyncInspectRequest>(1000);
    let service = ControllerService {
        state: State::Idle(IdleState {
            model,
            finish_tx: finish_tx.clone(),
        }),
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
