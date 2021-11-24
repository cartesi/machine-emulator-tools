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

use serde::{Deserialize, Serialize};
use std::fmt::Write;
use std::os::unix::prelude::RawFd;

mod bindings;

pub use bindings::CARTESI_ROLLUP_ADDRESS_SIZE;
pub use bindings::CARTESI_ROLLUP_ADVANCE_STATE;
pub use bindings::CARTESI_ROLLUP_INSPECT_STATE;

fn convert_address_to_string(
    address: &[u8; bindings::CARTESI_ROLLUP_ADDRESS_SIZE as usize],
) -> String {
    let mut result = String::new();
    for i in 0..address.len() {
        result
            .write_fmt(format_args!("{:02x}", address[i]))
            .unwrap_or(());
    }
    result
}

fn convert_string_to_address(
    address: &String,
) -> [u8; bindings::CARTESI_ROLLUP_ADDRESS_SIZE as usize] {
    let mut result: [u8; bindings::CARTESI_ROLLUP_ADDRESS_SIZE as usize] =
        [0; bindings::CARTESI_ROLLUP_ADDRESS_SIZE as usize];
    let start = if address.starts_with("0x") { 2 } else { 0 };
    for i in 0..&address[start..].len() / 2 {
        result[i] = u8::from_str_radix(&address[i * 2..i * 2 + 2], 16).unwrap_or_default();
    }
    result
}

#[derive(Debug, Default)]
pub struct RollupError {
    message: String,
}

impl RollupError {
    pub fn new(message: &str) -> Self {
        RollupError {
            message: String::from(message),
        }
    }
}

impl std::fmt::Display for RollupError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "Rollup error: {}", &self.message)
    }
}

impl std::error::Error for RollupError {}

pub struct RollupFinish {
    pub accept_previous_request: bool,
    pub next_request_type: i32,
    pub next_request_payload_length: i32,
}

impl Default for RollupFinish {
    fn default() -> Self {
        RollupFinish {
            next_request_payload_length: 0,
            next_request_type: 0,
            accept_previous_request: false,
        }
    }
}

impl From<RollupFinish> for bindings::rollup_finish {
    fn from(other: RollupFinish) -> Self {
        bindings::rollup_finish {
            next_request_type: other.next_request_type,
            accept_previous_request: other.accept_previous_request,
            next_request_payload_length: other.next_request_payload_length,
        }
    }
}

impl From<&mut RollupFinish> for bindings::rollup_finish {
    fn from(other: &mut RollupFinish) -> Self {
        bindings::rollup_finish {
            next_request_type: other.next_request_type,
            accept_previous_request: other.accept_previous_request,
            next_request_payload_length: other.next_request_payload_length,
        }
    }
}

impl From<bindings::rollup_finish> for RollupFinish {
    fn from(other: bindings::rollup_finish) -> Self {
        RollupFinish {
            next_request_type: other.next_request_type,
            accept_previous_request: other.accept_previous_request,
            next_request_payload_length: other.next_request_payload_length,
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct AdvanceMetadata {
    pub msg_sender: String,
    pub block_number: u64,
    pub time_stamp: u64,
    pub epoch_index: u64,
    pub input_index: u64,
}

impl From<bindings::rollup_input_metadata> for AdvanceMetadata {
    fn from(other: bindings::rollup_input_metadata) -> Self {
        AdvanceMetadata {
            input_index: other.input_index,
            epoch_index: other.epoch_index,
            time_stamp: other.time_stamp,
            block_number: other.block_number,
            msg_sender: convert_address_to_string(&other.msg_sender),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct AdvanceRequest {
    pub metadata: AdvanceMetadata,
    pub payload: String,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct InspectRequest {
    pub payload: String,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct Notice {
    pub payload: String,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct Voucher {
    pub address: String,
    pub payload: String,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct Report {
    pub payload: String,
}

pub fn rollup_finish_request(
    fd: RawFd,
    finish: &mut RollupFinish,
) -> Result<(), Box<dyn std::error::Error>> {
    let mut finish_c = Box::new(bindings::rollup_finish::from(&mut *finish));
    let res = unsafe { bindings::rollup_finish_request(fd as i32, finish_c.as_mut()) };
    if res != 0 {
        return Err(Box::new(RollupError::new(&format!(
            "IOCTL_ROLLUP_FINISH returned error {}",
            res
        ))));
    }
    *finish = RollupFinish::from(*finish_c);
    Ok(())
}

pub fn rollup_read_advance_state_request(
    fd: RawFd,
    finish: &mut RollupFinish,
) -> Result<AdvanceRequest, Box<dyn std::error::Error>> {
    let mut finish_c = Box::new(bindings::rollup_finish::from(&mut *finish));
    let mut bytes_c = Box::new(bindings::rollup_bytes {
        data: std::ptr::null::<::std::os::raw::c_uchar>() as *mut ::std::os::raw::c_uchar,
        length: 0,
    });
    let mut input_metadata_c = Box::new(bindings::rollup_input_metadata {
        msg_sender: Default::default(),
        block_number: 0,
        time_stamp: 0,
        epoch_index: 0,
        input_index: 0,
    });
    let res = unsafe {
        bindings::rollup_read_advance_state_request(
            fd as i32,
            finish_c.as_mut(),
            bytes_c.as_mut(),
            input_metadata_c.as_mut(),
        )
    };

    if res != 0 {
        return Err(Box::new(RollupError::new(&format!(
            "IOCTL_ROLLUP_READ_ADVANCE_STATE returned error {}",
            res
        ))));
    }
    println!("Advanced state bytes length: {}", bytes_c.length);

    if bytes_c.length == 0 {
        return Err(Box::new(RollupError::new(&format!(
            "Read zero size from advance state request "
        ))));
    }

    let mut payload: Vec<u8> = Vec::with_capacity(bytes_c.length as usize);
    unsafe {
        std::ptr::copy(bytes_c.data, payload.as_mut_ptr(), bytes_c.length as usize);
        payload.set_len(bytes_c.length as usize);
    }
    let result = AdvanceRequest {
        metadata: AdvanceMetadata::from(*input_metadata_c),
        payload: std::str::from_utf8(&payload)?.to_string(),
    };
    *finish = RollupFinish::from(*finish_c);
    Ok(result)
}

pub fn rollup_write_notices(
    fd: RawFd,
    count: u32,
    notice: &Notice,
) -> Result<(), Box<dyn std::error::Error>> {
    if count == 0 {
        return Ok(());
    }
    let mut buffer: Vec<u8> = Vec::with_capacity(notice.payload.len());
    let mut bytes_c = Box::new(bindings::rollup_bytes {
        data: buffer.as_mut_ptr() as *mut ::std::os::raw::c_uchar,
        length: notice.payload.len() as u64,
    });
    let res = unsafe {
        std::ptr::copy(
            notice.payload.as_ptr(),
            buffer.as_mut_ptr(),
            notice.payload.len(),
        );
        bindings::rollup_write_notices(fd as i32, count, bytes_c.as_mut())
    };
    if res != 0 {
        return Err(Box::new(RollupError::new(&format!(
            "IOCTL_ROLLUP_WRITE_NOTICE returned error {}",
            res
        ))));
    }
    Ok(())
}

pub fn rollup_write_voucher(
    fd: RawFd,
    count: u32,
    voucher: &Voucher,
) -> Result<(), Box<dyn std::error::Error>> {
    if count == 0 {
        return Ok(());
    }
    let mut buffer: Vec<u8> = Vec::with_capacity(voucher.payload.len());
    let mut bytes_c = Box::new(bindings::rollup_bytes {
        data: buffer.as_mut_ptr() as *mut ::std::os::raw::c_uchar,
        length: voucher.payload.len() as u64,
    });

    let mut address_c = convert_string_to_address(&voucher.address);

    let res = unsafe {
        std::ptr::copy(
            voucher.payload.as_ptr(),
            buffer.as_mut_ptr(),
            voucher.payload.len(),
        );
        bindings::rollup_write_vouchers(fd as i32, count, address_c.as_mut_ptr(), bytes_c.as_mut())
    };
    if res != 0 {
        return Err(Box::new(RollupError::new(&format!(
            "IOCTL_ROLLUP_WRITE_VOUCHER returned error {}",
            res
        ))));
    }
    Ok(())
}

pub fn rollup_write_report(
    fd: RawFd,
    count: u32,
    report: &Report,
) -> Result<(), Box<dyn std::error::Error>> {
    if count == 0 {
        return Ok(());
    }
    let mut buffer: Vec<u8> = Vec::with_capacity(report.payload.len());
    let mut bytes_c = Box::new(bindings::rollup_bytes {
        data: buffer.as_mut_ptr() as *mut ::std::os::raw::c_uchar,
        length: report.payload.len() as u64,
    });
    let res = unsafe {
        std::ptr::copy(
            report.payload.as_ptr(),
            buffer.as_mut_ptr(),
            report.payload.len(),
        );
        bindings::rollup_write_reports(fd as i32, count, bytes_c.as_mut())
    };
    if res != 0 {
        return Err(Box::new(RollupError::new(&format!(
            "IOCTL_ROLLUP_WRITE_REPORT returned error {}",
            res
        ))));
    }
    Ok(())
}

pub fn print_address(address: &String) {
    if address.starts_with("0x") {
        println!("{}", address);
    } else {
        println!("0x{}", address);
    }
}

pub fn print_advance(advance: &AdvanceRequest) {
    print!("Advance: {{\n\tmsg_sender: ");
    print_address(&advance.metadata.msg_sender);
    println!(
        "\tblock_number: {}\n\ttime_stamp: {}\n\tepoch_index: {}\n\tinput_index: {}\n}}",
        advance.metadata.block_number,
        advance.metadata.time_stamp,
        advance.metadata.epoch_index,
        advance.metadata.input_index
    );
}

pub fn print_notice(notice: &Notice) {
    println!(
        "Notice: {{\n\tlength: {} payload: {}\n}}",
        notice.payload.len(),
        notice.payload
    );
}

pub fn print_voucher(voucher: &Voucher) {
    print!("Voucher: {{\n\taddress:");
    print_address(&voucher.address);
    println!(
        "\tlength: {} payload: {}\n}}",
        voucher.payload.len(),
        voucher.payload
    );
}

pub fn print_report(report: &Report) {
    println!(
        "Report: {{\n\tlength: {} payload: {}\n}}",
        report.payload.len(),
        report.payload
    );
}
