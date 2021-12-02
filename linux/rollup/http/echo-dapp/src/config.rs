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

/// Application configuration
#[derive(Clone, Debug, PartialEq)]
pub struct Config {
    pub http_address: String,
    pub http_port: u16,
    pub dispatcher_address: String,
    pub dispatcher_port: u16,
    pub wihtout_dispatcher: bool,
    pub reject: i32,
}

impl Config {
    pub fn new() -> Self {
        Self {
            http_address: String::from("127.0.0.1"),
            http_port: 5002,
            dispatcher_address: String::from("127.0.0.1"),
            dispatcher_port: 5001,
            wihtout_dispatcher: false,
            reject: -1,
        }
    }
}
