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

#[derive(Clone, Debug, PartialEq)]
pub struct TestConfig {
    pub vouchers: u32,
    pub reports: u32,
    pub notices: u32,
    pub reject: i32,
    pub exception: i32,
}

/// Application configuration
#[derive(Clone, Debug, PartialEq)]
pub struct Config {
    pub rollup_http_server_address: String,
    pub test_config: TestConfig,
}

impl Config {
    pub fn new(test_config: TestConfig) -> Self {
        Self {
            rollup_http_server_address: String::from("127.0.0.1:5001"),
            test_config,
        }
    }
}
