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

#include <cstdint>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <errno.h>

#include <fcntl.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <unistd.h>

extern "C" {
#include "libcmt/rollup.h"
};

#include "json.hpp"

// RAII file descriptor implementation
class rollup {
public:
    rollup(bool load_merkle = true) {
        if (cmt_rollup_init(&m_rollup))
            throw std::system_error(errno, std::generic_category(),
                "Unable to initialize. Try runnning again with CMT_DEBUG=yes'");
        if (load_merkle)
            cmt_rollup_load_merkle(&m_rollup, "/tmp/merkle.dat");
    }
    ~rollup() {
        cmt_rollup_save_merkle(&m_rollup, "/tmp/merkle.dat");
        cmt_rollup_fini(&m_rollup);
    }
    operator cmt_rollup_t *(void) {
        return &m_rollup;
    }

private:
    cmt_rollup_t m_rollup;
};

// Print help message with program usage
static void print_help(void) {
    std::cerr <<
        R"(Usage:
    rollup [command]

  where [command] is one of

    voucher
      emit a voucher read from stdin as a JSON object in the format
        {"destination": <address>, "value": <hex-uint256>, "payload": <hex-data>}
      where
        <address> contains a 20-byte EVM address in hex,
        <hex-uint256> contains a big-endian 32-byte unsigned integer in hex, and
        <hex-data> contains arbitrary data in hex
      if successful, prints to stdout a JSON object in the format
        {"index": <number> }
      where field "index" is the index allocated for the voucher

    notice
      emit a notice read from stdin as a JSON object in the format
        {"payload": <hex-data> }
      where
        <hex-data> contains arbitrary data in hex
      if successful, prints to stdout a JSON object in the format
        {"index": <number> }
      where field "index" is the index allocated for the notice

    report
      emit a report read from stdin as a JSON object in the format
        {"payload": <hex-data> }
      where
        <hex-data> contains arbitrary data in hex

    finish
      accept or reject the previous request based on a JSON object
      read from stdin in the format
        {"status": <string> }
      where "status" is either "accept" or "reject".

      print the next request to stdout as a JSON object in the format
        {"request_type": <request-type>, "data": <request-data>}

      when field "request_type" contains "advance_state",
      field "data" contains a JSON object in the format
        {
          "chain_id": <number>,
          "app_contract": <address>,
          "msg_sender": <address>,
          "block_number": <number>,
          "block_timestamp": <number>
          "prev_randao": <hex-uint256>,
          "index": <number>,
          "payload": <hex-data>
        },
      where
        <address> contains a 20-byte EVM address in hex,
        <hex-uint256> contains a big-endian 32-byte unsigned integer in hex, and
        <hex-data> contains arbitrary data in hex

      when field "request_type" contains "inspect_state",
      field "data" contains a JSON object in the format
        {"payload": <hex-data> }
      where
        <hex-data> contains arbitrary data in hex

    accept
      a shortcut for finish with implied input
        {"status": "accept" }
      no input is read from stdin

    reject
      a shortcut for finish with implied input
        {"status": "reject" }
      no input is read from stdin

    exception
      throw an exception read from stdin as a JSON object in the format
        {"payload": <hex-data> }
      where
        <hex-data> contains arbitrary data in hex

    gio
      performs a generic IO operation request based on a JSON object
      read from stdin in the format
        { "domain": <number>, "id": <hex-data> }
      if successful, prints to stdout a JSON object in the format
        { "code": <number>, "data": <hex-data> }
      where
        <hex-data> contains arbitrary data in hex

)";
}

// Read all stdin input into a string
static std::string read_input(void) {
    std::istreambuf_iterator<char> begin(std::cin), end;
    return std::string(begin, end);
}

// Convert a hex character into its corresponding nibble {0..15}
static uint8_t hexnibble(char a) {
    if (a >= 'a' && a <= 'f') {
        return a - 'a' + 10;
    }
    if (a >= 'A' && a <= 'F') {
        return a - 'A' + 10;
    }
    if (a >= '0' && a <= '9') {
        return a - '0';
    }
    throw std::invalid_argument{"invalid hex character"};
    return 0;
}

// Convert two hex character into its corresponding byte {0..255}
static uint8_t hexbyte(char a, char b) {
    return hexnibble(a) << 4 | hexnibble(b);
}

// Convert an hex string into the corresponding bytes
static std::string unhex(const std::string &s) {
    if (s.find_first_not_of("abcdefABCDEF0123456789", 2) != std::string::npos) {
        throw std::invalid_argument{"invalid character in address"};
    }
    std::string res;
    res.reserve(20 + 1);
    for (unsigned i = 2; i < s.size(); i += 2) {
        res.push_back(hexbyte(s[i], s[i + 1]));
    }
    return res;
}

static std::string unhex20(const std::string &s) {
    if (s.size() != 2 + 40) {
        throw std::invalid_argument{"incorrect address size"};
    }
    return unhex(s);
}

static std::string unhex32(const std::string &s) {
    if (s.size() > 2 + 64) {
        throw std::invalid_argument{"incorrect value size"};
    }
    return unhex(s);
}

// Convert binary data into hex string
static std::string hex(const uint8_t *data, uint64_t length) {
    static const char t[] = "0123456789abcdef";
    std::stringstream ss;
    ss << "0x";
    for (uint64_t i = 0; i < length; ++i) {
        char hi = t[(data[i] >> 4) & 0x0f];
        char lo = t[(data[i] >> 0) & 0x0f];
        ss << std::hex << hi << lo;
    }
    return ss.str();
}

// Read input for voucher data, issue voucher, write result to output
static int write_voucher(void) try {
    rollup r;
    auto ji = nlohmann::json::parse(read_input());
    auto payload_bytes = unhex(ji["payload"].get<std::string>());
    auto destination_bytes = unhex20(ji["destination"].get<std::string>());
    auto value_bytes = unhex32(ji["value"].get<std::string>());
    uint64_t index = 0;
    cmt_abi_address_t destination;
    cmt_abi_u256_t value;
    cmt_abi_bytes_t payload;
    payload.data = reinterpret_cast<unsigned char *>(payload_bytes.data());
    payload.length = payload_bytes.size();

    memcpy(destination.data, reinterpret_cast<unsigned char *>(destination_bytes.data()), destination_bytes.size());
    memcpy(value.data, reinterpret_cast<unsigned char *>(value_bytes.data()), value_bytes.size());

    int ret = cmt_rollup_emit_voucher(r, &destination, &value, &payload, &index);
    if (ret)
        return ret;

    nlohmann::json j = {{"index", index}};
    std::cout << j.dump(2) << '\n';

    return 0;
} catch (std::exception &x) {
    std::cerr << x.what() << '\n';
    return 1;
}

// Read input for notice data, issue notice, write result to output
static int write_notice(void) try {
    rollup r;
    auto ji = nlohmann::json::parse(read_input());
    auto payload_bytes = unhex(ji["payload"].get<std::string>());
    cmt_abi_bytes_t payload;
    payload.data = reinterpret_cast<unsigned char *>(payload_bytes.data());
    payload.length = payload_bytes.size();
    uint64_t index = 0;
    int ret = cmt_rollup_emit_notice(r, &payload, &index);
    if (ret)
        return ret;

    nlohmann::json j = {{"index", index}};
    std::cout << j.dump(2) << '\n';

    return 0;

} catch (std::exception &x) {
    std::cerr << x.what() << '\n';
    return 1;
}

// Read input for report data, issue report
static int write_report(void) try {
    rollup r;
    auto ji = nlohmann::json::parse(read_input());
    auto payload_bytes = unhex(ji["payload"].get<std::string>());
    cmt_abi_bytes_t payload;
    payload.data = reinterpret_cast<unsigned char *>(payload_bytes.data());
    payload.length = payload_bytes.size();
    return cmt_rollup_emit_report(r, &payload);
} catch (std::exception &x) {
    std::cerr << x.what() << '\n';
    return 1;
}

// Read input for exception data, throw exception
static int throw_exception(void) try {
    rollup r;
    auto ji = nlohmann::json::parse(read_input());
    auto payload_bytes = unhex(ji["payload"].get<std::string>());
    cmt_abi_bytes_t payload;
    payload.data = reinterpret_cast<unsigned char *>(payload_bytes.data());
    payload.length = payload_bytes.size();
    return cmt_rollup_emit_exception(r, &payload);
} catch (std::exception &x) {
    std::cerr << x.what() << '\n';
    return 1;
}

// Read advance state data from driver, write to output
static int write_advance_state(rollup &r, const cmt_rollup_finish_t *f) {
    (void) f;
    cmt_rollup_advance_t advance;
    int rc = cmt_rollup_read_advance_state(r, &advance);

    if (rc != 0) {
        return 1;
    }

    nlohmann::json j = {{"request_type", "advance_state"},
        {"data",
            {
                {"chain_id", advance.chain_id},
                {"app_contract", hex(advance.app_contract.data, std::size(advance.app_contract.data))},
                {"msg_sender", hex(advance.msg_sender.data, std::size(advance.msg_sender.data))},
                {"block_number", advance.block_number},
                {"block_timestamp", advance.block_timestamp},
                {"prev_randao", hex(advance.prev_randao.data, std::size(advance.prev_randao.data))},
                {"index", advance.index},
                {"payload", hex(reinterpret_cast<const uint8_t *>(advance.payload.data), advance.payload.length)},
            }}};
    std::cout << j.dump(2) << '\n';
    return 0;
}

// Read inspect state data from driver, write to output
static int write_inspect_state(rollup &r, const cmt_rollup_finish_t *f) {
    (void) f;
    cmt_rollup_inspect_t inspect;
    int rc = cmt_rollup_read_inspect_state(r, &inspect);

    if (rc != 0) {
        return 1;
    }

    nlohmann::json j = {{"request_type", "inspect_state"},
        {"data",
            {
                {"payload", hex(reinterpret_cast<const uint8_t *>(inspect.payload.data), inspect.payload.length)},
            }}};
    std::cout << j.dump(2) << '\n';
    return 0;
}

// Finish current request and get next
static int finish_request_and_get_next(bool accept) try {
    rollup r;
    cmt_rollup_finish_t f;
    f.accept_previous_request = accept;
    if (cmt_rollup_finish(r, &f))
        return 1;
    if (f.next_request_type == HTIF_YIELD_REASON_ADVANCE) {
        cmt_rollup_reset_merkle(r);
        return write_advance_state(r, &f);
    } else if (f.next_request_type == HTIF_YIELD_REASON_INSPECT) {
        return write_inspect_state(r, &f);
    }
    return 0;
} catch (std::exception &x) {
    std::cerr << x.what() << '\n';
    return 1;
}

// Accept current request and get next
static int accept_request(void) {
    return finish_request_and_get_next(true);
}

// Reject current request and get next
static int reject_request(void) {
    return finish_request_and_get_next(false);
}

// Finish current request and get next
static int finish_request(void) try {
    auto ji = nlohmann::json::parse(read_input());
    auto status = ji["status"].get<std::string>();
    if (status == "accept") {
        return finish_request_and_get_next(true);
    } else if (status == "reject") {
        return finish_request_and_get_next(false);
    } else {
        std::cerr << "invalid status '" << status << "'\n";
        return 1;
    }
} catch (std::exception &x) {
    std::cerr << x.what() << '\n';
    return 1;
}

// Read GIO request, issue operation, write response to output
static int gio(void) try {
    rollup r;
    auto ji = nlohmann::json::parse(read_input());
    auto id = unhex(ji["id"].get<std::string>());
    auto domain = ji["domain"].get<uint16_t>();

    cmt_gio req{.domain = domain,
        .id_length = static_cast<uint32_t>(id.size()),
        .id = id.data(),
        .response_code = 0,
        .response_data_length = 0,
        .response_data = nullptr};

    int ret = cmt_gio_request(r, &req);
    if (ret)
        return ret;

    nlohmann::json j = {
        {"code", req.response_code},
        {"data", hex(reinterpret_cast<const uint8_t *>(req.response_data), req.response_data_length)},
    };
    std::cout << j.dump(2) << '\n';

    return 0;

} catch (std::exception &x) {
    std::cerr << x.what() << '\n';
    return 1;
}

// Figure out command and run it
int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_help();
        exit(1);
    }
    const char *command = argv[1];
    if (strcmp(command, "voucher") == 0) {
        return write_voucher();
    } else if (strcmp(command, "notice") == 0) {
        return write_notice();
    } else if (strcmp(command, "report") == 0) {
        return write_report();
    } else if (strcmp(command, "exception") == 0) {
        return throw_exception();
    } else if (strcmp(command, "finish") == 0) {
        return finish_request();
    } else if (strcmp(command, "accept") == 0) {
        return accept_request();
    } else if (strcmp(command, "reject") == 0) {
        return reject_request();
    } else if (strcmp(command, "gio") == 0) {
        return gio();
    } else if (strcmp(command, "-h") == 0 || strcmp(command, "--help") == 0) {
        print_help();
        return 0;
    } else {
        std::cerr << "Unexpected command '" << command << "'\n\n";
        return 1;
    }
    return 0;
}
