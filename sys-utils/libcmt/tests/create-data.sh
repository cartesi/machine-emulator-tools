#!/bin/bash

[ ! -d build/data/ ] && mkdir build/data/

echo "#ifndef DATA_H"
echo "#define DATA_H"
echo "#include <stdint.h>"
echo "uint8_t valid_advance_0[] = {"
cast calldata "EvmAdvance(uint256,address,address,uint256,uint256,uint256,bytes)" \
	0x0000000000000000000000000000000000000001 \
	0x0000000000000000000000000000000000000002 \
	0x0000000000000000000000000000000000000003 \
	0x0000000000000000000000000000000000000004 \
	0x0000000000000000000000000000000000000005 \
	0x0000000000000000000000000000000000000006 \
	0x`echo -en "advance-0" | xxd -p -c0` | xxd -r -p | xxd -i
echo "};"

echo "uint8_t valid_inspect_0[] = {"
echo -en "inspect-0" | xxd -i
echo "};"

echo "uint8_t valid_report_0[] = {"
echo -en "report-0" | xxd -i
echo "};"

echo "uint8_t valid_exception_0[] = {"
echo -en "exception-0" | xxd -i
echo "};"

echo "uint8_t valid_voucher_0[] = {"
cast calldata "Voucher(address,uint256,bytes)" \
	0x0000000000000000000000000000000000000001 \
	0x00000000000000000000000000000000deadbeef \
	0x`echo -en "voucher-0" | xxd -p -c0` | xxd -r -p | xxd -i
echo "};"

echo "uint8_t valid_notice_0[] = {"
cast calldata "Notice(bytes)" \
	0x`echo -en "notice-0" | xxd -p -c0` | xxd -r -p | xxd -i
echo "};"

echo "uint8_t valid_gio_reply_0[] = {"
echo -en "gio-reply-0" | xxd -i
echo "};"


echo "#endif /* DATA_H */"
