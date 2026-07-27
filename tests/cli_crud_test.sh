#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${1:-$ROOT_DIR/build/myschedule}"
DATA_DIR="$(mktemp -d "${TMPDIR:-/tmp}/myschedule_cli_test.XXXXXX")"

cleanup() {
    rm -rf "$DATA_DIR"
}
trap cleanup EXIT

fail() {
    echo "[FAIL] $*" >&2
    exit 1
}

run_cmd() {
    MYSCHEDULE_DATA_DIR="$DATA_DIR" "$BIN" "$@"
}

assert_contains() {
    local haystack="$1"
    local needle="$2"
    local label="$3"
    if [[ "$haystack" != *"$needle"* ]]; then
        echo "$haystack" >&2
        fail "$label: expected output to contain: $needle"
    fi
}

assert_not_contains() {
    local haystack="$1"
    local needle="$2"
    local label="$3"
    if [[ "$haystack" == *"$needle"* ]]; then
        echo "$haystack" >&2
        fail "$label: expected output not to contain: $needle"
    fi
}

if [[ ! -x "$BIN" ]]; then
    fail "executable not found: $BIN. Build first with: cmake -S . -B build && cmake --build build -j"
fi

USERNAME="cli_test_user"
PASSWORD="cli_test_pass"

echo "[TEST] create test account"
create_output="$(
    printf '%s\n%s\ny\nexit\n' "$USERNAME" "$PASSWORD" |
        MYSCHEDULE_DATA_DIR="$DATA_DIR" "$BIN" run
)"
assert_contains "$create_output" "User created successfully." "create account"

echo "[TEST] add tasks"
add_output="$(run_cmd "$USERNAME" "$PASSWORD" addtask \
    --name "Write CMake report" \
    --start "2099-07-10 14:30" \
    --priority high \
    --category study \
    --remind "2099-07-10 14:20")"
assert_contains "$add_output" "Task added successfully. ID = 1" "add first task"

add_second_output="$(run_cmd "$USERNAME" "$PASSWORD" addtask \
    --name "Read course notes" \
    --start "2099-07-11 09:00" \
    --priority medium \
    --category study \
    --remind "2099-07-11 08:50")"
assert_contains "$add_second_output" "Task added successfully. ID = 2" "add second task"

echo "[TEST] show all/day/month"
all_output="$(run_cmd "$USERNAME" "$PASSWORD" showtask --all)"
assert_contains "$all_output" "Write CMake report" "show all first task"
assert_contains "$all_output" "Read course notes" "show all second task"

day_output="$(run_cmd "$USERNAME" "$PASSWORD" showtask --day 2099-07-10)"
assert_contains "$day_output" "Write CMake report" "show day"
assert_not_contains "$day_output" "Read course notes" "show day excludes other dates"

month_output="$(run_cmd "$USERNAME" "$PASSWORD" showtask --month 2099-07)"
assert_contains "$month_output" "Write CMake report" "show month first task"
assert_contains "$month_output" "Read course notes" "show month second task"

echo "[TEST] update task"
update_output="$(run_cmd "$USERNAME" "$PASSWORD" updatetask --id 1 \
    --name "Write final report" \
    --start "2099-07-10 15:00" \
    --priority medium \
    --category study \
    --remind "2099-07-10 14:45")"
assert_contains "$update_output" "Task updated successfully." "update task"

updated_output="$(run_cmd "$USERNAME" "$PASSWORD" showtask --all)"
assert_contains "$updated_output" "Write final report" "show updated name"
assert_contains "$updated_output" "2099-07-10 15:00" "show updated time"
assert_not_contains "$updated_output" "Write CMake report" "old name removed after update"

echo "[TEST] delete task"
delete_output="$(run_cmd "$USERNAME" "$PASSWORD" deltask --id 1)"
assert_contains "$delete_output" "Task deleted successfully." "delete task"

after_delete_output="$(run_cmd "$USERNAME" "$PASSWORD" showtask --all)"
assert_not_contains "$after_delete_output" "Write final report" "deleted task absent"
assert_contains "$after_delete_output" "Read course notes" "remaining task still present"

echo "[PASS] CLI CRUD test completed"
