/* Copyright 2021 Cartesi Pte. Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

use actix_web::{middleware::Logger, web::Data, web::Json, App, HttpResponse, HttpServer};

use crate::config::Config;
use crate::rollup;
use crate::rollup::{Notice, Report, Voucher};
use async_mutex::Mutex;
use serde::{Deserialize, Serialize};
use std::os::unix::io::RawFd;
use std::sync::Arc;

/// Setup the HTTP server that receives requests from the DApp backend
pub async fn run(
    config: &Config,
    start_rollup_tx: std::sync::mpsc::Sender<bool>,
    rollup_fd: Arc<Mutex<RawFd>>,
) -> std::io::Result<()> {
    println!("Starting http service!");
    let id_generator = Arc::new(Mutex::new(IdGenerator::new()));
    HttpServer::new(move || {
        let data = Data::new(Mutex::new(Context {
            rollup_fd: rollup_fd.clone(),
            id_generator: id_generator.clone(),
        }));
        App::new()
            .app_data(data)
            .wrap(Logger::default())
            .service(voucher)
            .service(notice)
            .service(report)
            .service(finish)
    })
    .bind((config.http_address.as_str(), config.http_port))
    .and_then(|t| {
        start_rollup_tx.send(true).unwrap();
        Ok(t)
    })?
    .run()
    .await
    .map_err(|e| e.into())
}

#[actix_web::post("/voucher")]
async fn voucher(voucher: Json<Voucher>, data: Data<Mutex<Context>>) -> HttpResponse {
    log::info!("Received voucher request {:?}", voucher);
    if voucher.address.len() != (rollup::CARTESI_ROLLUP_ADDRESS_SIZE * 2 + 2) as usize
        || !voucher.address.starts_with("0x")
    {
        return HttpResponse::BadRequest().body("Address not valid");
    }
    let context = data.lock().await;
    let fd = context.rollup_fd.lock().await;
    return match rollup::rollup_write_voucher(*fd, 1, &voucher.0) {
        Ok(_) => {
            drop(fd);
            let id = context.id_generator.lock().await.get_new_voucher_id();
            HttpResponse::Created().json(IdResponse { id })
        }
        Err(e) => HttpResponse::Conflict().body(format!(
            "Unable to insert voucher, error details: '{}'",
            e.to_string()
        )),
    };
}

#[actix_web::post("/notice")]
async fn notice(notice: Json<Notice>, data: Data<Mutex<Context>>) -> HttpResponse {
    log::info!("Received notice request {:?}", notice);
    let context = data.lock().await;
    let fd = context.rollup_fd.lock().await;
    return match rollup::rollup_write_notices(*fd, 1, &notice.0) {
        Ok(_) => {
            drop(fd);
            let id = context.id_generator.lock().await.get_new_notice_id();
            HttpResponse::Created().json(IdResponse { id })
        }
        Err(e) => HttpResponse::Conflict().body(format!(
            "Unable to insert notice, error details: '{}'",
            e.to_string()
        )),
    };
}

#[actix_web::post("/report")]
async fn report(report: Json<Report>, data: Data<Mutex<Context>>) -> HttpResponse {
    log::info!("Received report request {:?}", report);
    let context = data.lock().await;
    let fd = context.rollup_fd.lock().await;
    return match rollup::rollup_write_report(*fd, 1, &report.0) {
        Ok(_) => {
            drop(fd);
            let id = context.id_generator.lock().await.get_new_report_id();
            HttpResponse::Created().json(IdResponse { id })
        }
        Err(e) => HttpResponse::Conflict().body(format!(
            "Unable to insert notice, error details: '{}'",
            e.to_string()
        )),
    };
}

#[actix_web::post("/finish")]
async fn finish(finish: Json<FinishRequest>, data: Data<Mutex<Context>>) -> HttpResponse {
    log::info!("Received finish request {:?}", finish);
    match finish.status.as_str() {
        "accept" => {}
        "reject" => {}
        _ => {
            return HttpResponse::UnprocessableEntity().body("status must be 'accept' or 'reject'");
        }
    }
    // let context = data.lock().await;
    // let fd = context.rollup_fd.lock().await;
    // return match rollup::rollup_finish_request(*fd, &mut finish.0) {
    //     Ok(_) => {
    //         drop(fd);
    //         let id = context.id_generator.lock().await.get_new_report_id();
    //         HttpResponse::Accepted().finish()
    //     }
    //     Err(e) => HttpResponse::Conflict().body(format!(
    //         "Unable to insert notice, error details: '{}'",
    //         e.to_string()
    //     )),
    // };
    HttpResponse::Accepted().finish()
}

#[derive(Debug, Clone, Deserialize)]
struct FinishRequest {
    status: String,
}

#[derive(Debug, Clone, Serialize)]
struct IdResponse {
    id: u64,
}

#[derive(Debug, Clone, Serialize)]
struct ErrorDescription {
    code: u16,
    reason: String,
    description: String,
}

#[derive(Debug, Serialize)]
struct Error {
    error: ErrorDescription,
}

#[derive(Debug, Clone, Serialize)]
struct IdGenerator {
    voucher_count: u64,
    notice_count: u64,
    report_count: u64,
}

impl IdGenerator {
    pub fn new() -> Self {
        IdGenerator {
            voucher_count: 0,
            notice_count: 0,
            report_count: 0,
        }
    }
    pub fn get_new_voucher_id(&mut self) -> u64 {
        self.voucher_count = self.voucher_count + 1;
        self.voucher_count
    }

    pub fn get_new_notice_id(&mut self) -> u64 {
        self.notice_count = self.notice_count + 1;
        self.notice_count
    }

    pub fn get_new_report_id(&mut self) -> u64 {
        self.report_count = self.report_count + 1;
        self.report_count
    }
}

struct Context {
    pub rollup_fd: Arc<Mutex<RawFd>>,
    pub id_generator: Arc<Mutex<IdGenerator>>,
}
