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
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <linux/cartesi/rollup.h>

#include "json.hpp"

#define ROLLUP_DEVICE_NAME "/dev/rollup"

// RAII file descriptor implementation

// NullablePointer implementation for file descriptors
class file_desc {
public:
    explicit file_desc(int fd) : m_fd(fd) {}
    file_desc(std::nullptr_t = nullptr) : m_fd(-1) {}
    operator int() const {
        return m_fd;
    }
    bool operator==(const file_desc &other) const {
        return m_fd == other.m_fd;
    }
    bool operator!=(const file_desc &other) const {
        return m_fd != other.m_fd;
    }
    bool operator==(std::nullptr_t) const {
        return m_fd == -1;
    }
    bool operator!=(std::nullptr_t) const {
        return m_fd != -1;
    }

private:
    int m_fd;
};

// Deleter for unique_ptr representing a file descriptor
struct file_desc_deleter {
    using pointer = file_desc;
    void operator()(file_desc fp) const {
        close(fp);
    }
};

// unique_ptr representing a file descriptor
using unique_file_desc = std::unique_ptr<file_desc, file_desc_deleter>;

// maker for unique_file_desc
static unique_file_desc unique_open(const char *filename, int flags) {
    int fd = open(filename, flags);
    if (fd < 0) {
        throw std::system_error(errno, std::generic_category(), "unable to open '" + std::string{filename} + "'");
    }
    return unique_file_desc(file_desc(fd));
}

// Convert ioctl request name to string
static std::string get_ioctl_name(int ioctl) {
    const static std::unordered_map<int, std::string> ioctl_names = {
        {IOCTL_ROLLUP_WRITE_VOUCHER, "write voucher"},
        {IOCTL_ROLLUP_WRITE_NOTICE, "write notice"},
        {IOCTL_ROLLUP_WRITE_REPORT, "write report"},
        {IOCTL_ROLLUP_FINISH, "finish"},
        {IOCTL_ROLLUP_READ_ADVANCE_STATE, "advance state"},
        {IOCTL_ROLLUP_READ_INSPECT_STATE, "inspect state"},
        {IOCTL_ROLLUP_THROW_EXCEPTION, "throw exception"},
    };
    auto got = ioctl_names.find(ioctl);
    if (got == ioctl_names.end()) {
        return "unknown";
    }
    return got->second;
}

// ioctl for unique_file_desc
static void file_desc_ioctl(const unique_file_desc &fd, unsigned long request, void *data) {
    if (ioctl(fd.get(), request, (unsigned long) data) < 0) {
        throw std::runtime_error{get_ioctl_name(request) + " ioctl returned error '" + strerror(errno) + "'"};
    }
}

// Print help message with program usage
static void print_help(void) {
    std::cerr <<
        R"(Usage:
    rollup [command]

  where [command] is one of

    voucher
      emit a voucher read from stdin as a JSON object in the format
        {"destination": <address>, "payload": <string>}
      where field "destination" contains a 20-byte EVM address in hex.
      if successful, prints to stdout a JSON object in the format
        {"index": <number> }
      where field "index" is the index allocated for the voucher
      in the voucher hashes array.

    notice
      emit a notice read from stdin as a JSON object in the format
        {"payload": <string> }
      if successful, prints to stdout a JSON object in the format
        {"index": <number> }
      where field "index" is the index allocated for the notice
      in the voucher hashes array.

    report
      emit a report read from stdin as a JSON object in the format
        {"payload": <string> }

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
          "metadata": {
            "msg_sender": <msg-sender>,
            "epoch_index": <number>,
            "input_index": <number>,
            "block_number": <number>,
            "timestamp": <number>
            },
            "payload": <string>
        },
      where field "msg_sender" contains a 20-byte EVM address in hex

      when field "request_type" contains "inspect_state",
      field "data" contains a JSON object in the format
        {"payload": <string> }

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
        {"payload": <string> }
)";
}

// Read all stdin input into a string
static std::string read_input(void) {
    std::istreambuf_iterator<char> begin(std::cin), end;
    return std::string(begin, end);
}

// Try to grow a bytes structure to contain size bytes
static void resize_bytes(struct rollup_bytes *bytes, uint64_t size) {
    if (bytes->length < size) {
        uint8_t *new_data = (uint8_t *) realloc(bytes->data, size);
        if (!new_data) {
            throw std::bad_alloc{};
        }
        bytes->length = size;
        bytes->data = new_data;
    }
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
    if (s.size() != 42) {
        throw std::invalid_argument{"incorrect address size"};
    }
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

// Convert binary data into hex string
static std::string hex(const uint8_t *data, uint64_t length) {
    std::stringstream ss;
    ss << "0x";
    for (auto b : std::string_view{reinterpret_cast<const char *>(data), length}) {
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned>(b);
    }
    return ss.str();
}

// Read input for voucher data, issue voucher, write result to output
static int write_voucher(void) try {
    auto ji = nlohmann::json::parse(read_input());
    auto payload = ji["payload"].get<std::string>();
    struct rollup_voucher v;
    memset(&v, 0, sizeof(v));
    v.payload.data = reinterpret_cast<unsigned char *>(payload.data());
    v.payload.length = payload.size();
    auto destination = unhex(ji["destination"].get<std::string>());
    memcpy(v.destination, destination.data(), std::min(destination.size(), sizeof(v.destination)));
    file_desc_ioctl(unique_open(ROLLUP_DEVICE_NAME, O_RDWR), IOCTL_ROLLUP_WRITE_VOUCHER, &v);
    nlohmann::json jo = {
        {"index", v.index},
    };
    std::cout << jo.dump(2) << '\n';
    return 0;
} catch (std::exception &x) {
    std::cerr << x.what() << '\n';
    return 1;
}

// Read input for notice data, issue notice, write result to output
static int write_notice(void) try {
    auto ji = nlohmann::json::parse(read_input());
    auto payload = ji["payload"].get<std::string>();
    struct rollup_notice n;
    memset(&n, 0, sizeof(n));
    n.payload.data = reinterpret_cast<unsigned char *>(payload.data());
    n.payload.length = payload.size();
    file_desc_ioctl(unique_open(ROLLUP_DEVICE_NAME, O_RDWR), IOCTL_ROLLUP_WRITE_NOTICE, &n);
    nlohmann::json jo = {
        {"index", n.index},
    };
    std::cout << jo.dump(2) << '\n';
    return 0;
} catch (std::exception &x) {
    std::cerr << x.what() << '\n';
    return 1;
}

// Read input for report data, issue report
static int write_report(void) try {
    auto ji = nlohmann::json::parse(read_input());
    auto payload = ji["payload"].get<std::string>();
    struct rollup_report r;
    memset(&r, 0, sizeof(r));
    r.payload.data = reinterpret_cast<unsigned char *>(payload.data());
    r.payload.length = payload.size();
    file_desc_ioctl(unique_open(ROLLUP_DEVICE_NAME, O_RDWR), IOCTL_ROLLUP_WRITE_REPORT, &r);
    return 0;
} catch (std::exception &x) {
    std::cerr << x.what() << '\n';
    return 1;
}

// Read input for exception data, throw exception
static int throw_exception(void) try {
    auto ji = nlohmann::json::parse(read_input());
    auto payload = ji["payload"].get<std::string>();
    struct rollup_exception e;
    memset(&e, 0, sizeof(e));
    e.payload.data = reinterpret_cast<unsigned char *>(payload.data());
    e.payload.length = payload.size();
    file_desc_ioctl(unique_open(ROLLUP_DEVICE_NAME, O_RDWR), IOCTL_ROLLUP_THROW_EXCEPTION, &e);
    return 0;
} catch (std::exception &x) {
    std::cerr << x.what() << '\n';
    return 1;
}

// Read advance state data from driver, write to output
static void write_advance_state(const unique_file_desc &fd, const struct rollup_finish *f) {
    struct rollup_advance_state r;
    memset(&r, 0, sizeof(r));
    resize_bytes(&r.payload, f->next_request_payload_length);
    file_desc_ioctl(fd, IOCTL_ROLLUP_READ_ADVANCE_STATE, &r);
    auto payload = std::string_view{reinterpret_cast<const char *>(r.payload.data), r.payload.length};
    const auto &m = r.metadata;
    nlohmann::json j = {{"request_type", "advance_state"},
        {"data",
            {{"payload", payload},
                {"metadata",
                    {{"msg_sender", hex(m.msg_sender, sizeof(m.msg_sender))}, {"epoch_index", m.epoch_index},
                        {"input_index", m.input_index}, {"block_number", m.block_number},
                        {"timestamp", m.timestamp}}}}}};
    std::cout << j.dump(2) << '\n';
}

// Read inspect state data from driver, write to output
static void write_inspect_state(const unique_file_desc &fd, const struct rollup_finish *f) {
    struct rollup_inspect_state r;
    memset(&r, 0, sizeof(r));
    resize_bytes(&r.payload, f->next_request_payload_length);
    file_desc_ioctl(fd, IOCTL_ROLLUP_READ_INSPECT_STATE, &r);
    auto payload = std::string_view{reinterpret_cast<const char *>(r.payload.data), r.payload.length};
    nlohmann::json j = {{"request_type", "inspect_state"},
        {"data",
            {
                {"payload", payload},
            }}};
    std::cout << j.dump(2) << '\n';
}

// Finish current request and get next
static int finish_request_and_get_next(bool accept) try {
    struct rollup_finish f;
    memset(&f, 0, sizeof(f));
    f.accept_previous_request = accept;
    auto fd = unique_open(ROLLUP_DEVICE_NAME, O_RDWR);
    file_desc_ioctl(fd, IOCTL_ROLLUP_FINISH, &f);
    if (f.next_request_type == CARTESI_ROLLUP_ADVANCE_STATE) {
        write_advance_state(fd, &f);
    } else if (f.next_request_type == CARTESI_ROLLUP_INSPECT_STATE) {
        write_inspect_state(fd, &f);
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
    } else if (strcmp(command, "-h") == 0 || strcmp(command, "--help") == 0) {
        print_help();
        return 0;
    } else {
        std::cerr << "Unexpected command '" << command << "'\n\n";
        return 1;
    }
    return 0;
}
