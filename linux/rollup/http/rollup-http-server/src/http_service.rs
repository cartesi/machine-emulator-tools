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

/// Implement http dispatcher http REST api, including
/// voucher/notice/report/finish endpoints, used by the DApp
/// to communicate its output
use actix_web::{middleware::Logger, web::Data, web::Json, App, HttpResponse, HttpServer};

use crate::config::Config;
use crate::rollup;
use crate::rollup::{Notice, Report, Voucher};
use async_mutex::Mutex;
use serde::{Deserialize, Serialize};
use std::os::unix::io::RawFd;
use std::sync::Arc;
use rollup_http_server::rollup::{AdvanceRequest, InspectRequest, RollupRequest, REQUEST_TYPE_INSPECT_STATE, REQUEST_TYPE_ADVANCE_STATE};


#[derive(Debug, Serialize, Deserialize)]
#[serde(tag = "type")]
enum RollupHttpRequest {
    Advance {r#request_type: String, data: AdvanceRequest},
    Inspect {r#request_type: String, data: InspectRequest},
}

/// Setup the HTTP server that receives requests from the DApp backend
pub async fn run(
    config: &Config,
    start_rollup_tx: std::sync::mpsc::Sender<bool>,
    rollup_fd: Arc<Mutex<RawFd>>,
    finish_tx: tokio::sync::mpsc::Sender<bool>,
    request_rx: tokio::sync::mpsc::Receiver<RollupRequest>,
) -> std::io::Result<()> {
    log::info!("starting http dispatcher http service!");
    let request_rx = Arc::new(Mutex::new(request_rx));
    HttpServer::new(move || {
        let data = Data::new(Mutex::new(Context {
            rollup_fd: rollup_fd.clone(),
            finish_tx: finish_tx.clone(),
            request_rx: request_rx.clone()
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
    .map(|t| {
        start_rollup_tx.send(true).unwrap();
        t
    })?
    .run()
    .await
}

/// Process voucher request from DApp, write voucher to rollup device
#[actix_web::post("/voucher")]
async fn voucher(mut voucher: Json<Voucher>, data: Data<Mutex<Context>>) -> HttpResponse {
    log::debug!("received voucher request");
    // Check if address is valid
    if voucher.address.len() != (rollup::CARTESI_ROLLUP_ADDRESS_SIZE * 2 + 2) as usize
        || (!voucher.address.starts_with("0x"))
    {
        log::error!(
            "address not valid: '{}' len: {}",
            voucher.address,
            voucher.address.len()
        );
        return HttpResponse::BadRequest().body("Address not valid");
    }
    let context = data.lock().await;
    // Write voucher to linux rollup device
    return match rollup::rollup_write_voucher(*context.rollup_fd.lock().await, &mut voucher.0) {
        Ok(voucher_index) => {
            log::debug!("voucher successfully inserted {:#?}", voucher);
            HttpResponse::Created().json(IndexResponse {
                index: voucher_index,
            })
        }
        Err(e) => {
            log::error!(
                "unable to insert voucher, error details: '{}'",
                e.to_string()
            );
            HttpResponse::Conflict()
                .body(format!("unable to insert voucher, error details: '{}'", e))
        }
    };
}

/// Process notice request from DApp, write notice to rollup device
#[actix_web::post("/notice")]
async fn notice(mut notice: Json<Notice>, data: Data<Mutex<Context>>) -> HttpResponse {
    log::debug!("received notice request");
    let context = data.lock().await;
    // Write notice to linux rollup device
    return match rollup::rollup_write_notices(*context.rollup_fd.lock().await, &mut notice.0) {
        Ok(notice_index) => {
            log::debug!("notice successfully inserted {:#?}", notice);
            HttpResponse::Created().json(IndexResponse {
                index: notice_index,
            })
        }
        Err(e) => {
            log::error!("unable to insert notice, error details: '{}'", e);
            HttpResponse::Conflict()
                .body(format!("Unable to insert notice, error details: '{}'", e))
        }
    };
}

/// Process report request from DApp, write report to rollup device
#[actix_web::post("/report")]
async fn report(report: Json<Report>, data: Data<Mutex<Context>>) -> HttpResponse {
    log::debug!("received report request");
    let context = data.lock().await;
    // Write report to linux rollup device
    return match rollup::rollup_write_report(*context.rollup_fd.lock().await, &report.0) {
        Ok(_) => {
            log::debug!("report successfully inserted {:#?}", report);
            HttpResponse::Accepted().body("")
        }
        Err(e) => {
            log::error!("unable to insert report, error details: '{}'", e);
            HttpResponse::Conflict()
                .body(format!("unable to insert notice, error details: '{}'", e))
        }
    };
}


/// Process finish request from DApp, write finish to rollup device
/// and pass RollupFinish struct to linux rollup advance/inspect requests loop thread
#[actix_web::post("/finish")]
async fn finish(finish: Json<FinishRequest>, data: Data<Mutex<Context>>) -> HttpResponse {
    log::debug!("received finish request {:#?}", finish);
    // Prepare finish status for the rollup manager
    let accept = match finish.status.as_str() {
        "accept" => {true}
        "reject" => {false}
        _ => {
            return HttpResponse::UnprocessableEntity().body("status must be 'accept' or 'reject'");
        }
    };
    let context = data.lock().await;
    // Indicate to loop thread that request is finished
    if let Err(e) = context.finish_tx.send(accept).await {
        log::error!("error passing finish request to linux driver loop {}", e.to_string());
    }

    // Wait for the next request
    let mut request_rx = context.request_rx.lock().await;
    match request_rx.recv().await {
        Some(rollup_request) => {
            match rollup_request {
                RollupRequest::Advance(advance_request) => {
                    // prepare http response with advance request
                    return HttpResponse::Ok()
                        .append_header((hyper::header::CONTENT_TYPE, "application/json"))
                        .json(RollupHttpRequest::Advance {request_type: REQUEST_TYPE_ADVANCE_STATE.to_string(), data: advance_request });
                },
                RollupRequest::Inspect(inspect_reqeust) => {
                    // prepare http response with inspect request
                    return HttpResponse::Ok()
                        .append_header((hyper::header::CONTENT_TYPE, "application/json"))
                        .json(RollupHttpRequest::Inspect {request_type: REQUEST_TYPE_INSPECT_STATE.to_string(), data: inspect_reqeust });
                }
            }
        }
        None => {
            log::error!("error getting new rollup request from the driver");
            return HttpResponse::NotFound().body("Unable to provide new rollup request");
        }
    }
}

#[derive(Debug, Clone, Deserialize)]
struct FinishRequest {
    status: String,
}

#[derive(Debug, Clone, Serialize)]
struct IndexResponse {
    index: u64,
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

struct Context {
    pub rollup_fd: Arc<Mutex<RawFd>>,
    pub finish_tx: tokio::sync::mpsc::Sender<bool>,
    pub request_rx: Arc<Mutex<tokio::sync::mpsc::Receiver<RollupRequest>>>,
}
