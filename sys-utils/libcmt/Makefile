# Copyright Cartesi and individual authors (see AUTHORS)
#
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

O=build

DESTDIR ?= _install
PREFIX ?= /usr

export DESTDIR
export PREFIX

all: $O
	@ninja --quiet -C $O #&> /dev/null

$O $O/build.ninja:
	meson setup \
		-Dprefix=$(PREFIX) \
		-Dc_std=gnu2x \
		-Dwarning_level=3 \
		-Db_coverage=true \
		--cross-file=riscv64.ini \
		$O #&> /dev/null

cov: coverage-html
coverage-html: test
test: all
install uninstall test coverage-html: $O
	@ninja --quiet -C $O $@
clean:
	rm -rf $O

doc/theme:
	git clone git@github.com:jothepro/doxygen-awesome-css.git $@
	git -C doc/theme checkout 8cea9a073ecd50a5b2c0958a3df100292d6c7374

doc: doc/theme examples
	doxygen doc/Doxyfile
