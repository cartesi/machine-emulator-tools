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

use actix_web::{
    middleware::Logger,
    web::{Data, Json},
    App, HttpRequest, HttpResponse, HttpServer, Responder,
};

use crate::config::Config;
use crate::controller::ControllerChannel;
use crate::model::{AdvanceError, AdvanceRequest, InspectRequest};

pub async fn run(config: &Config, ctx: ControllerChannel) -> std::io::Result<()> {
    HttpServer::new(move || {
        App::new()
            .app_data(Data::new(ctx.clone()))
            .wrap(Logger::default())
            .service(advance)
            .service(inspect)
    })
    .bind((config.http_address.as_str(), config.http_port))?
    .run()
    .await
    .map_err(|e| e.into())
}

#[actix_web::post("/advance")]
async fn advance(
    req: Json<AdvanceRequest>,
    ctx: Data<ControllerChannel>,
) -> Result<impl Responder, AdvanceError> {
    match ctx.process_advance(req.into_inner()).await {
        Ok(()) => Ok(HttpResponse::Accepted()),
        Err(e) => {
            log::error!("Unable to accept advance request: {}", e);
            Ok(HttpResponse::InternalServerError())
        }
    }
}

#[actix_web::get("/inspect/{payload}")]
async fn inspect(
    req: HttpRequest,
    ctx: Data<ControllerChannel>,
) -> Result<impl Responder, AdvanceError> {
    match req.match_info().get("payload") {
        Some(payload) => match ctx
            .process_inspect(InspectRequest {
                payload: payload.to_string(),
            })
            .await
        {
            Ok(_inspect_result) => Ok(HttpResponse::Accepted()),
            Err(e) => {
                log::error!("Unable to accept inspect request: {}", e);
                Ok(HttpResponse::InternalServerError())
            }
        },
        None => {
            log::error!("Unable to accept inspect request: no payload provided");
            Ok(HttpResponse::BadRequest())
        }
    }
}
