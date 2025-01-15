# Copyright Cartesi and individual authors (see AUTHORS)
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

UTILITIES := hex xhalt yield rollup ioctl-echo-loop misc cartesi-init

PREFIX ?= /usr
DESTDIR ?=

all:
	@$(MAKE) $(UTILITIES)

$(UTILITIES):
	@$(MAKE) -C $@

install:
	@$(foreach utility,$(UTILITIES),$(MAKE) -C $(utility) install;)

clean:
	@$(foreach utility,$(UTILITIES),$(MAKE) -C $(utility) clean;)

.PHONY: all $(UTILITIES) install clean
