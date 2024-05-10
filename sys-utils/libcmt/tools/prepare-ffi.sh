#!/bin/sh

sed \
	-e '/\/\*/,/\*\//d' \
	-e '/#if\s/,/#endif/d' \
	-e '/#define/d' \
	-e '/#endif/d' \
	-e '/#ifndef/d' \
	-e '/#include/d' \
