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

mod config;
mod controller;
mod http_service;
mod model;

use crate::config::Config;
use crate::model::TestEchoData;
use controller::new_controller;
use getopts::Options;
use model::Model;
use std::io::ErrorKind;

fn print_usage(program: &str, opts: Options) {
    let brief = format!(
        "Usage: {} [options]\n Where options are: \n
          --address=<n>  address/port to open the dapp http service (default: 127.0.0.1:5002)\n,
          --proxy=<n>  address/port of the target proxy service (default: 127.0.0.1:5001)\n,
          --vouchers=<n>   replicate input in n vouchers (default: 0)\n
          --notices=<n>    replicate input in n notices (default: 0)\n
          --reports=<n>    replicate input in n reports (default: 1)\n",
        program
    );
    print!("{}", opts.usage(&brief));
}

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    // Set the default log level to debug
    env_logger::Builder::from_env(env_logger::Env::default().default_filter_or("debug")).init();

    let args: Vec<String> = std::env::args().collect();
    let program = args[0].clone();
    // Process command line arguments
    let mut opts = Options::new();
    opts.optopt(
        "",
        "address",
        "Address to listen (default: 127.0.0.1:5002)",
        "",
    );
    opts.optopt(
        "",
        "proxy",
        "Address to listen (default: 127.0.0.1:5002)",
        "",
    );
    opts.optopt(
        "",
        "vouchers",
        "Replicate input in n vouchers (default 0)",
        "",
    );
    opts.optopt(
        "",
        "notices",
        "Replicate input in n notices (default 0)",
        "",
    );
    opts.optopt(
        "",
        "reports",
        "Replicate input in n reports (default 1)",
        "",
    );
    opts.optflag("h", "help", "show this help message and exit");
    opts.optopt(
        "",
        "address",
        "Address to listen (default: 127.0.0.1:5001)",
        "",
    );
    let matches = match opts.parse(&args[1..]) {
        Ok(m) => m,
        Err(e) => {
            log::error!("error parsing arguments: {}", &e);
            return Err(std::io::Error::new(ErrorKind::InvalidInput, e.to_string()));
        }
    };
    if matches.opt_present("h") {
        print_usage(&program, opts);
        return Ok(());
    }

    let mut config = Config::new();
    {
        // Parse addresses and ports
        let tmp = matches
            .opt_get_default("address", "127.0.0.1:5002".to_string())
            .unwrap_or_default();
        let mut address = tmp.split(":");
        config.http_address = address.next().expect("Address is not valid").to_string();
        config.http_port = address
            .next()
            .expect("Port is not valid")
            .to_string()
            .parse::<u16>()
            .unwrap();
        let tmp = matches
            .opt_get_default("proxy", "127.0.0.1:5001".to_string())
            .unwrap_or_default();
        let mut proxy = tmp.split(":");
        config.proxy_address = proxy.next().expect("Address is not valid").to_string();
        config.proxy_port = proxy
            .next()
            .expect("Port is not valid")
            .to_string()
            .parse::<u16>()
            .unwrap();
    }
    let vouchers = matches
        .opt_get_default("vouchers", 0)
        .expect("vouchers could not be parsed");
    let notices = matches
        .opt_get_default("notices", 0)
        .expect("notices could not be parsed");
    let reports = matches
        .opt_get_default("reports", 1)
        .expect("reports could not be parsed");

    let model = Box::new(Model::new(
        &config.proxy_address,
        config.proxy_port,
        TestEchoData {
            vouchers,
            notices,
            reports,
        },
    ));
    let (channel, service) = new_controller(model, config.clone());

    tokio::spawn(async {
        service.run().await;
        log::info!("proxy service terminated successfully");
    });

    match http_service::run(&config, channel).await {
        Ok(_) => log::info!("http service terminated successfully"),
        Err(e) => log::warn!("http service terminated with error: {}", e),
    }

    Ok(())
}
