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

use std::env;
use std::path::PathBuf;

extern crate pkg_config;

fn main() {
    let mock_build = env::var("MOCK_BUILD").unwrap_or_else(|_| "false".to_string()) == "true";

    let lib_dir = if mock_build {
        "../../sys-utils/libcmt/build/mock".into()
    } else {
        pkg_config::Config::new()
            .statik(true)
            .probe("libcmt")
            .expect("Could not find library")
            .link_paths
            .iter()
            .map(|path| path.to_string_lossy().into_owned())
            .collect::<Vec<String>>()
            .get(0)
            .expect("Library path not found")
            .clone()
    };

    let header_path = if mock_build {
        "../../sys-utils/libcmt/src/rollup.h".into()
    } else {
        pkg_config::get_variable("libcmt", "includedir").expect("Could not find include directory")
            + "/libcmt/rollup.h"
    };

    let include_path = if mock_build {
        "-I../../sys-utils/libcmt/src".into()
    } else {
        "-I".to_string()
            + &pkg_config::get_variable("libcmt", "includedir")
                .expect("Could not find include directory")
    };

    // link the libcmt shared library
    println!("cargo:rustc-link-search=native={}", lib_dir);
    println!("cargo:rerun-if-changed={}/libcmt.a", lib_dir);
    println!("cargo:rustc-link-lib=static=cmt");

    let bindings = bindgen::Builder::default()
        // the input header we would like to generate bindings for
        .header(header_path)
        .clang_arg(include_path)
        // invalidate the built crate whenever any of the included header files changed
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
