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

/// Implementation of http advance/inspect endpoints
use actix_web::{
    middleware::Logger,
    web::{Data, Json},
    App, HttpRequest, HttpResponse, HttpServer, Responder,
};

use crate::config::Config;
use crate::controller::ControllerChannel;
use crate::model::{AdvanceError, AdvanceRequest, InspectRequest};

/// Start http service
pub async fn run(config: &Config, ctx: ControllerChannel) -> std::io::Result<()> {
    HttpServer::new(move || {
        App::new()
            .app_data(Data::new(ctx.clone()))
            .wrap(Logger::default())
            .service(advance)
            .service(inspect)
            .service(ping)
    })
    .bind((config.http_address.as_str(), config.http_port))?
    .run()
    .await
    .map_err(|e| e)
}

/// Implementation of http advance request endpoint
#[actix_web::post("/advance")]
async fn advance(
    req: Json<AdvanceRequest>,
    ctx: Data<ControllerChannel>,
) -> Result<impl Responder, AdvanceError> {
    match ctx.process_advance(req.into_inner()).await {
        Ok(()) => Ok(HttpResponse::Accepted()),
        Err(e) => {
            log::error!("unable to accept advance request: {}", e);
            Ok(HttpResponse::InternalServerError())
        }
    }
}

/// Implementation of http inspect request endpoint
#[actix_web::get("/inspect/{payload}")]
async fn inspect(req: HttpRequest, ctx: Data<ControllerChannel>) -> impl Responder {
    match req.match_info().get("payload") {
        Some(payload) => {
            if payload.is_empty() {
                // Empty query string, return OK
                return HttpResponse::Ok().body("");
            }
            match ctx
                .process_inspect(InspectRequest {
                    payload: payload.to_string(),
                })
                .await
            {
                Ok(inspect_result) => match inspect_result {
                    Ok(inspect_report) => {
                        return HttpResponse::Ok()
                            .append_header((hyper::header::CONTENT_TYPE, "application/json"))
                            .json(inspect_report);
                    }
                    Err(inspect_error) => {
                        HttpResponse::InternalServerError().body(inspect_error.cause)
                    }
                },
                Err(e) => {
                    log::error!("unable to accept inspect request: {}", e.to_string());
                    HttpResponse::InternalServerError().body(e.to_string())
                }
            }
        }
        None => {
            log::error!("unable to accept inspect request: no payload provided");
            HttpResponse::BadRequest().body("unable to accept inspect request: no payload provided")
        }
    }
}

/// Implementation of http dummy ping endpoint
/// Used to check if service is alive when starting up
/// internal emulator http dispatcher service
#[actix_web::get("/ping")]
async fn ping(_req: HttpRequest, _ctx: Data<ControllerChannel>) -> impl Responder {
    // Return ok
    return HttpResponse::Ok().body("");
}
