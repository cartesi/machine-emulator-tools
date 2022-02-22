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
mod http_dispatcher;
mod model;

use crate::config::Config;
use crate::model::TestEchoData;
use controller::new_controller;
use getopts::Options;
use model::Model;
use std::io::ErrorKind;


fn print_usage(program: &str, opts: Options) {
    let brief = format!("Usage: {} [options]\n Where options are:", program);
    print!("{}", opts.usage(&brief));
}

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    // Process command line arguments
    let args: Vec<String> = std::env::args().collect();
    let program = args[0].clone();
    let mut opts = Options::new();
    opts.optopt(
        "",
        "address",
        "Address of the dapp http service (default: 127.0.0.1:5003)",
        "",
    );
    opts.optopt(
        "",
        "dispatcher",
        "Http dispatcher address (default: 127.0.0.1:5004)",
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
    opts.optopt("", "reject", "Reject the nth input (default: -1)", "");
    opts.optflag("h", "help", "show this help message and exit");
    opts.optflag("", "verbose", "print more info about application execution");
    let matches = match opts.parse(&args[1..]) {
        Ok(m) => m,
        Err(e) => {
            eprintln!("error parsing arguments: {}", &e);
            return Err(std::io::Error::new(ErrorKind::InvalidInput, e.to_string()));
        }
    };
    if matches.opt_present("h") {
        print_usage(&program, opts);
        return Ok(());
    }

    // Set log level of application
    let mut log_level = "info";
    let verbose = matches.opt_present("verbose");
    if verbose {
        log_level = "debug";
    }
    // Set the global log level
    env_logger::Builder::from_env(env_logger::Env::default().default_filter_or(log_level)).init();

    // Setup configuration
    let mut config = Config::new();
    {
        // Parse addresses and ports
        let address_match = matches
            .opt_get_default("address", "127.0.0.1:5003".to_string())
            .unwrap_or_default();
        let mut address = address_match.split(':');
        config.http_address = address.next().expect("address is not valid").to_string();
        config.http_port = address
            .next()
            .expect("port is not valid")
            .to_string()
            .parse::<u16>()
            .unwrap();
        let dispatcher_match = matches
            .opt_get_default("dispatcher", "127.0.0.1:5004".to_string())
            .unwrap_or_default();
        let mut dispatcher = dispatcher_match.split(':');
        config.dispatcher_address = dispatcher
            .next()
            .expect("http dispatcher address is not valid")
            .to_string();
        config.dispatcher_port = dispatcher
            .next()
            .expect("http dispatcher port is not valid")
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
    let reject = matches
        .opt_get_default("reject", -1)
        .expect("reject could not be parsed");
    // Instantiate Dapp model that implements DApp logic
    let model = Box::new(Model::new(
        &config.dispatcher_address,
        config.dispatcher_port,
        TestEchoData {
            vouchers,
            notices,
            reports,
            reject,
        },
    ));

    // Controller handles application flow
    let (channel, service) = new_controller(model, config.clone());
    // Run controller service as async service
    tokio::spawn(async {
        service.run().await;
        log::info!("http service service terminated successfully");
    });

    Ok(())
}
