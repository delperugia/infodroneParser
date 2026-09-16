#!/bin/bash

set -u

TESTS_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
PARSER="$TESTS_DIR/../build/infodroneParser"
result=0

run_success_test()
{
	local input=$1
	local marker=$2
	local output
	local exit_code

	output=$("$PARSER" "$TESTS_DIR/$input" 2>&1)
	exit_code=$?
	printf '     %s\n' "$output"

	if (( exit_code != 0 )); then
		printf 'FAIL: %s returned exit code %d\n' "$input" "$exit_code" >&2
		result=1
	elif ! grep -q "$marker" <<< "$output"; then
		printf 'FAIL: %s output does not contain %s\n' "$input" "$marker" >&2
		result=1
	else
		printf 'PASS: %s\n' "$input"
	fi
}

run_error_test()
{
	local input=$1
	local output
	local exit_code

	output=$("$PARSER" "$TESTS_DIR/$input" 2>&1)
	exit_code=$?
	printf '     %s\n' "$output"

	if (( exit_code == 0 )); then
		printf 'FAIL: %s returned exit code 0\n' "$input" >&2
		result=1
	else
		printf 'PASS: %s\n' "$input"
	fi
}

# Valid parsing, no error
run_success_test 01_valid_v0_minimal.pcapng '#0'
run_success_test 02_valid_v0_full_beacon.pcapng '#0'

# Processing error, no error
run_success_test 03_corrupt_present_bitmask_mismatch.pcapng 'E0'
run_success_test 04_corrupt_itlen_too_short.pcapng 'E0'
run_success_test 05_corrupt_itlen_too_long.pcapng 'E0'
run_success_test 06_corrupt_dbm_out_of_range.pcapng 'E0'
run_success_test 07_corrupt_channel_freq_invalid.pcapng 'E0'
run_success_test 08_corrupt_alignment_broken.pcapng 'E0'
run_success_test 09_corrupt_truncated_mid_tsft.pcapng 'E0'
run_success_test 10_corrupt_truncated_header_only.pcapng 'E0'
run_success_test 11_invalid_version.pcapng 'E0'
run_success_test 12_valid_beacon_bad_ssid_len.pcapng 'E0'
run_success_test 13_valid_management_bad_subtype.pcapng 'E0'
run_success_test 14_valid_beacon_wrong_fc_type.pcapng 'E0'
run_success_test 15_valid_beacon_truncated_mid_ie.pcapng 'E0'
run_success_test 16_valid_beacon_missing_essential_fields.pcapng 'E0'
run_success_test 17_corrupt_radiotap_and_payload_combo1.pcapng 'E0'
run_success_test 18_corrupt_radiotap_and_payload_combo2.pcapng 'E0'
run_success_test 19_edge_empty_payload_after_radiotap.pcapng 'E0'
run_success_test 20_edge_extended_present_bit_no_second_word.pcapng 'E0'

# Error
run_error_test 21_ethernet.pcapng
run_error_test 22_nonexisting.pcapng
run_error_test 23_tests.sh

exit "$result"
