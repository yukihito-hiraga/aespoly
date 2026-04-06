#!/bin/sh
set -eu

BUILD_DIR="${1:-build}"

echo "==> Running arm64 tests"
"$BUILD_DIR/test_gmac"
"$BUILD_DIR/test_pmac"
"$BUILD_DIR/test_aesgcm"
"$BUILD_DIR/test_ocb"
"$BUILD_DIR/test_aespolyW_aes128"
"$BUILD_DIR/test_aespolyW_rijndael256"
"$BUILD_DIR/test_aespolyW_simpira"
"$BUILD_DIR/test_aespolyM"
"$BUILD_DIR/test_hctr2_aes128"
"$BUILD_DIR/test_hctr2_rijndael256"
"$BUILD_DIR/test_hctr2_simpira"
"$BUILD_DIR/test_eme_aes128"
"$BUILD_DIR/test_eme_rijndael256"
"$BUILD_DIR/test_eme_simpira"

echo "All arm64 tests passed."
