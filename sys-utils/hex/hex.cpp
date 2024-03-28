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
#include <array>
#include <cstdio>
#include <cstring>

// clang-format off
static const unsigned char dec[256] = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 255, 255, 255, 255, 255, 255, 255, 10, 11, 12, 13, 14, 15, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    10, 11, 12, 13, 14, 15, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
};

static const unsigned char enc[512] = {
    '0','0', '0','1', '0','2', '0','3', '0','4', '0','5', '0','6', '0','7', '0','8', '0','9', '0','a', '0','b', '0','c',
    '0','d', '0','e', '0','f', '1','0', '1','1', '1','2', '1','3', '1','4', '1','5', '1','6', '1','7', '1','8', '1','9',
    '1','a', '1','b', '1','c', '1','d', '1','e', '1','f', '2','0', '2','1', '2','2', '2','3', '2','4', '2','5', '2','6',
    '2','7', '2','8', '2','9', '2','a', '2','b', '2','c', '2','d', '2','e', '2','f', '3','0', '3','1', '3','2', '3','3',
    '3','4', '3','5', '3','6', '3','7', '3','8', '3','9', '3','a', '3','b', '3','c', '3','d', '3','e', '3','f', '4','0',
    '4','1', '4','2', '4','3', '4','4', '4','5', '4','6', '4','7', '4','8', '4','9', '4','a', '4','b', '4','c', '4','d',
    '4','e', '4','f', '5','0', '5','1', '5','2', '5','3', '5','4', '5','5', '5','6', '5','7', '5','8', '5','9', '5','a',
    '5','b', '5','c', '5','d', '5','e', '5','f', '6','0', '6','1', '6','2', '6','3', '6','4', '6','5', '6','6', '6','7',
    '6','8', '6','9', '6','a', '6','b', '6','c', '6','d', '6','e', '6','f', '7','0', '7','1', '7','2', '7','3', '7','4',
    '7','5', '7','6', '7','7', '7','8', '7','9', '7','a', '7','b', '7','c', '7','d', '7','e', '7','f', '8','0', '8','1',
    '8','2', '8','3', '8','4', '8','5', '8','6', '8','7', '8','8', '8','9', '8','a', '8','b', '8','c', '8','d', '8','e',
    '8','f', '9','0', '9','1', '9','2', '9','3', '9','4', '9','5', '9','6', '9','7', '9','8', '9','9', '9','a', '9','b',
    '9','c', '9','d', '9','e', '9','f', 'a','0', 'a','1', 'a','2', 'a','3', 'a','4', 'a','5', 'a','6', 'a','7', 'a','8',
    'a','9', 'a','a', 'a','b', 'a','c', 'a','d', 'a','e', 'a','f', 'b','0', 'b','1', 'b','2', 'b','3', 'b','4', 'b','5',
    'b','6', 'b','7', 'b','8', 'b','9', 'b','a', 'b','b', 'b','c', 'b','d', 'b','e', 'b','f', 'c','0', 'c','1', 'c','2',
    'c','3', 'c','4', 'c','5', 'c','6', 'c','7', 'c','8', 'c','9', 'c','a', 'c','b', 'c','c', 'c','d', 'c','e', 'c','f',
    'd','0', 'd','1', 'd','2', 'd','3', 'd','4', 'd','5', 'd','6', 'd','7', 'd','8', 'd','9', 'd','a', 'd','b', 'd','c',
    'd','d', 'd','e', 'd','f', 'e','0', 'e','1', 'e','2', 'e','3', 'e','4', 'e','5', 'e','6', 'e','7', 'e','8', 'e','9',
    'e','a', 'e','b', 'e','c', 'e','d', 'e','e', 'e','f', 'f','0', 'f','1', 'f','2', 'f','3', 'f','4', 'f','5', 'f','6',
    'f','7', 'f','8', 'f','9', 'f','a', 'f','b', 'f','c', 'f','d', 'f','e', 'f','f',
};
// clang-format on

template <size_t IN, size_t OUT>
static int encode(unsigned char (&in)[IN], unsigned char (&out)[OUT]) {
    static_assert(OUT >= 2 * IN, "output buffer must be at least double of input buffer");
    for (;;) {
        auto got = fread(in, 1, std::size(in), stdin);
        if (got < 0) {
            fprintf(stderr, "error reading stdin\n");
            return 1;
        }
        decltype(got) j = 0;
        for (decltype(got) i = 0; i < got; ++i) {
            out[j] = enc[2 * in[i]];
            ++j;
            out[j] = enc[2 * in[i] + 1];
            ++j;
        }
        if (fwrite(out, 1, j, stdout) < 0) {
            fprintf(stderr, "error writing to stdout\n");
            return 1;
        }
        if (got != std::size(in)) {
            break;
        }
    }
    return 0;
}

template <size_t IN, size_t OUT>
static int decode(unsigned char (&in)[IN], unsigned char (&out)[OUT]) {
    static_assert(OUT >= IN / 2, "output buffer must be at least half of input buffer");
    for (;;) {
        auto got = fread(in, 1, std::size(in), stdin);
        if (got < 0) {
            fprintf(stderr, "error reading stdin\n");
            return 1;
        }
        if (got & 1) {
            fprintf(stderr, "invalid encoding\n");
            return 1;
        }
        decltype(got) j = 0;
        for (decltype(got) i = 0; i < got; i += 2) {
            unsigned char n0 = dec[in[i]];
            if (n0 > 15) {
                fprintf(stderr, "invalid encoding\n");
                return 1;
            }
            unsigned char n1 = dec[in[i + 1]];
            out[j] = (n0 << 4) + n1;
            ++j;
        }
        if (fwrite(out, 1, j, stdout) < 0) {
            fprintf(stderr, "error writing to stdout\n");
            return 1;
        }
        if (got != std::size(in)) {
            break;
        }
    }
    return 0;
}

// Print help message with program usage
static void print_help(void) {
    fprintf(stderr, R"(Usage:
  hex [options]

    --encode
      encode hex from stdin to stdout

    --decode
      decode hex from stdin to stdout

    --prefix
      output 0x prefix when encoding and expect it when encoding

    --no-prefix
      do not add 0x prefix when encoding or expect 0x when decoding

    --help
      print this error message

)");
}

int main(int argc, char *argv[]) {
    unsigned char in[1024];
    unsigned char out[2048];
    bool dec = true;
    bool pre = true;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--decode") == 0) {
            dec = true;
        } else if (strcmp(argv[i], "--encode") == 0) {
            dec = false;
        } else if (strcmp(argv[i], "--no-prefix") == 0) {
            pre = false;
        } else if (strcmp(argv[i], "--prefix") == 0) {
            pre = true;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_help();
            return 0;
        }
    }
    static const char *zx = "0x";
    if (dec) {
        if (pre) {
            if (fread(in, 1, 2, stdin) != 2 || strncmp(reinterpret_cast<char *>(in), zx, 2) != 0) {
                fprintf(stderr, "expected prefix 0x\n");
                return 1;
            }
        }
        return decode(in, out);
    } else {
        if (pre) {
            fwrite(zx, 1, 2, stdout);
        }
        return encode(in, out);
    }
}
