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

extern crate cc;

fn main() {
    println!("cargo:rerun-if-changed=src/rollup/bindings.c,src/rollup/bindings.h,tests/rollup_test_bindings.c,tests/rollup_test.h");

    let test = std::env::var("USE_ROLLUP_BINDINGS_MOCK").unwrap_or("0".to_string());

    if test == "1" {
        println!("cargo:rustc-link-lib=asan");
        println!("cargo:rustc-link-lib=ubsan");
        println!("cargo:rustc-link-lib=cmt_mock");
    } else {
        println!("cargo:rustc-link-lib=cmt");
    }
}
