#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT_DIR/build/myschedule"

if [ ! -x "$BIN" ]; then
  echo "Missing executable: $BIN. Run cmake -S . -B build && cmake --build build first." >&2
  exit 1
fi

export LANG="${LANG:-C.UTF-8}"
export MYSCHEDULE_DATA_DIR="$(mktemp -d)"
trap 'rm -rf "$MYSCHEDULE_DATA_DIR"' EXIT

USER_NAME="testuser"
PASSWORD="123456"
BAD_PASSWORD="wrong-password"
OTHER_USER="otheruser"

pass() {
  echo "[test_cli] $1"
}

fail() {
  echo "[test_cli] $1" >&2
  exit 1
}

run_ok() {
  "$@" || fail "Expected command to succeed: $*"
}

run_fail() {
  if "$@"; then
    fail "Expected command to fail: $*"
  fi
}

capture_ok() {
  local output
  output="$("$@")" || fail "Expected command to succeed: $*"
  printf '%s\n' "$output"
}

extract_id() {
  sed -n 's/.*ID = \([0-9][0-9]*\).*/\1/p' | tail -n 1
}

add_task() {
  local output
  output="$("$BIN" "$USER_NAME" "$PASSWORD" addtask "$@")" || fail "addtask failed: $*"
  local id
  id="$(printf '%s\n' "$output" | extract_id)"
  if [ -z "$id" ]; then
    fail "Could not parse task ID from: $output"
  fi
  printf '%s\n' "$id"
}

assert_contains() {
  local haystack="$1"
  local needle="$2"
  if ! grep -Fq "$needle" <<<"$haystack"; then
    fail "Expected output to contain: $needle"
  fi
}

assert_not_contains() {
  local haystack="$1"
  local needle="$2"
  if grep -Fq "$needle" <<<"$haystack"; then
    fail "Expected output not to contain: $needle"
  fi
}

line_number_for() {
  local haystack="$1"
  local needle="$2"
  grep -nF "$needle" <<<"$haystack" | head -n 1 | cut -d: -f1
}

pass "help and early argument validation"
help_output="$(capture_ok "$BIN" --help)"
assert_contains "$help_output" "addtask"
assert_contains "$help_output" "updatetask"
assert_contains "$help_output" "YYYY-MM-DD HH:MM"
assert_contains "$help_output" "background reminders"
capture_ok "$BIN" help >/dev/null
run_fail "$BIN"
run_fail "$BIN" run extra
run_fail "$BIN" "$USER_NAME"
if [ -e "$MYSCHEDULE_DATA_DIR/users.txt" ]; then
  fail "Help or early argument validation unexpectedly created users.txt."
fi

pass "interactive registration and authentication"
set +e
empty_username_output="$(printf '\n%s\n' "$PASSWORD" | "$BIN" run 2>&1)"
empty_username_status=$?
empty_password_output="$(printf 'empty_password_user\n\n' | "$BIN" run 2>&1)"
empty_password_status=$?
set -e
if [ "$empty_username_status" -eq 0 ] || [ "$empty_password_status" -eq 0 ]; then
  fail "An empty username or password was accepted."
fi
assert_contains "$empty_username_output" "username and password cannot be empty"
assert_contains "$empty_password_output" "username and password cannot be empty"

register_output="$(printf '%s\n%s\ny\nexit\n' "$USER_NAME" "$PASSWORD" | "$BIN" run 2>&1)"
assert_contains "$register_output" "User created successfully."
run_fail "$BIN" "$USER_NAME" "$BAD_PASSWORD" showtask --all
run_fail "$BIN" "$USER_NAME" "$PASSWORD" unknown-command
if grep -Fq "$PASSWORD" "$MYSCHEDULE_DATA_DIR/users.txt"; then
  fail "users.txt contains the plaintext password."
fi

other_register_output="$(printf '%s\n%s\ny\nexit\n' "$OTHER_USER" "$PASSWORD" | "$BIN" run 2>&1)"
assert_contains "$other_register_output" "User created successfully."
first_credential="$(awk -F: -v user="$USER_NAME" '$1 == user {print $2 ":" $3}' "$MYSCHEDULE_DATA_DIR/users.txt")"
second_credential="$(awk -F: -v user="$OTHER_USER" '$1 == user {print $2 ":" $3}' "$MYSCHEDULE_DATA_DIR/users.txt")"
if [ -z "$first_credential" ] || [ -z "$second_credential" ] || [ "$first_credential" = "$second_credential" ]; then
  fail "Equal passwords for different users did not produce different salted credentials."
fi

pass "direct CLI task commands"
custom_id="$(add_task \
  --name "复习高等数学第三章" \
  --start "2099-07-10 14:30" \
  --priority high \
  --category study \
  --remind "2099-07-10 14:00")"
default_id="$(add_task \
  --name "默认属性任务" \
  --start "2099-07-11 09:00")"
same_name_diff_time_id="$(add_task \
  --name "复习高等数学第三章" \
  --start "2099-07-12 10:00" \
  --category life)"

other_task_output="$(capture_ok "$BIN" "$OTHER_USER" "$PASSWORD" addtask \
  --name "其他用户的私有任务" --start "2099-07-10 14:30")"
assert_contains "$other_task_output" "ID = 1"

all_output="$(capture_ok "$BIN" "$USER_NAME" "$PASSWORD" showtask --all)"
assert_contains "$all_output" "复习高等数学第三章"
assert_contains "$all_output" "high"
assert_contains "$all_output" "study"
assert_contains "$all_output" "默认属性任务"
assert_contains "$all_output" "medium"
assert_contains "$all_output" "life"
assert_not_contains "$all_output" "其他用户的私有任务"
other_output="$(capture_ok "$BIN" "$OTHER_USER" "$PASSWORD" showtask --all)"
assert_contains "$other_output" "其他用户的私有任务"
assert_not_contains "$other_output" "默认属性任务"

pass "validation, filtering, and sorting"
run_fail "$BIN" "$USER_NAME" "$PASSWORD" addtask \
  --name "不同名称同时间" \
  --start "2099-07-10 14:30"
run_fail "$BIN" "$USER_NAME" "$PASSWORD" addtask \
  --name "bad date" --start "2099/07/10 14:30"
run_fail "$BIN" "$USER_NAME" "$PASSWORD" addtask \
  --name "bad priority" --start "2099-09-01 10:00" --priority urgent
run_fail "$BIN" "$USER_NAME" "$PASSWORD" addtask \
  --name "bad remind" --start "2099-09-03 10:00" --remind "2099-09-03 11:00"

early_id="$(add_task --name "排序最早" --start "2099-07-08 08:00")"
late_id="$(add_task --name "排序较晚" --start "2099-07-20 08:00")"
day_output="$(capture_ok "$BIN" "$USER_NAME" "$PASSWORD" showtask --day 2099-07-10)"
assert_contains "$day_output" "复习高等数学第三章"
month_output="$(capture_ok "$BIN" "$USER_NAME" "$PASSWORD" showtask --month 2099-07)"
assert_contains "$month_output" "排序最早"
assert_contains "$month_output" "排序较晚"
sorted_output="$(capture_ok "$BIN" "$USER_NAME" "$PASSWORD" showtask --all)"
early_line="$(line_number_for "$sorted_output" "排序最早")"
custom_line="$(line_number_for "$sorted_output" "复习高等数学第三章")"
late_line="$(line_number_for "$sorted_output" "排序较晚")"
if [ "$early_line" -ge "$custom_line" ] || [ "$custom_line" -ge "$late_line" ]; then
  fail "Tasks are not sorted by start time."
fi

pass "UTF-8 and long-name table boundaries"
add_task --name "数学" --start "2099-08-01 09:00" >/dev/null
add_task --name "这是一个非常非常长的中文任务名称用于检查列对齐" --start "2099-08-02 09:00" >/dev/null
add_task --name "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789" --start "2099-08-03 09:00" >/dev/null
wide_output="$(capture_ok "$BIN" "$USER_NAME" "$PASSWORD" showtask --month 2099-08)"
assert_contains "$wide_output" "数学"
assert_contains "$wide_output" "..."
if ! grep -Eq '\.\.\.[[:space:]]+\| 2099-08-0[23] 09:00' <<<"$wide_output"; then
  fail "Long task names did not preserve a clear column boundary before the start time."
fi

pass "update and delete commands"
run_ok "$BIN" "$USER_NAME" "$PASSWORD" updatetask --id "$custom_id" \
  --name "复习高等数学第四章" \
  --priority low \
  --category entertainment
updated_output="$(capture_ok "$BIN" "$USER_NAME" "$PASSWORD" showtask --all)"
assert_contains "$updated_output" "复习高等数学第四章"
assert_contains "$updated_output" "low"
assert_contains "$updated_output" "entertainment"

interactive_update_output="$(printf '%s\n%s\nupdatetask --id %s --name "交互 引号 修改" --start "2099-07-14 16:00" --remind "2099-07-14 15:30"\nshowtask --day 2099-07-14\nunknown-interactive-command\nexit\n' \
  "$USER_NAME" "$PASSWORD" "$custom_id" | "$BIN" run 2>&1)"
assert_contains "$interactive_update_output" "Task updated successfully."
assert_contains "$interactive_update_output" "交互 引号 修改"
assert_contains "$interactive_update_output" "Error: unknown command."

run_fail "$BIN" "$USER_NAME" "$PASSWORD" updatetask --id 1abc --name bad
run_fail "$BIN" "$USER_NAME" "$PASSWORD" updatetask --id "$custom_id" --priority urgent
run_fail "$BIN" "$USER_NAME" "$PASSWORD" updatetask --id "$custom_id"

run_ok "$BIN" "$USER_NAME" "$PASSWORD" deltask "$early_id"
run_ok "$BIN" "$USER_NAME" "$PASSWORD" deltask --id "$late_id"
after_delete_output="$(capture_ok "$BIN" "$USER_NAME" "$PASSWORD" showtask --all)"
assert_not_contains "$after_delete_output" "排序最早"
assert_not_contains "$after_delete_output" "排序较晚"
run_fail "$BIN" "$USER_NAME" "$PASSWORD" deltask 1abc
run_fail "$BIN" "$USER_NAME" "$PASSWORD" deltask 0
run_fail "$BIN" "$USER_NAME" "$PASSWORD" deltask -1

pass "interactive addtask prompts"
run_user="runmodeuser"
run_mode_output="$(printf '%s\n%s\ny\naddtask\nRun mode task\n2099-11-01 10:00\n\n\n2099-11-01 09:30\nshowtask --all\ndeltask 1\nshowtask --all\nexit\n' \
  "$run_user" "$PASSWORD" | "$BIN" run 2>&1)"
assert_contains "$run_mode_output" "Task added successfully. ID = 1"
assert_contains "$run_mode_output" "Run mode task"
assert_contains "$run_mode_output" "Task deleted successfully."
assert_contains "$run_mode_output" "No tasks found."

pass "background reminder in run mode"
reminder_start="$(date -d '+1 day' '+%Y-%m-%d %H:%M')"
reminder_due="$(date -d '-1 minute' '+%Y-%m-%d %H:%M')"
add_task --name "立即提醒任务" --start "$reminder_start" --remind "$reminder_due" >/dev/null
set +e
reminder_output="$({ printf '%s\n%s\n' "$USER_NAME" "$PASSWORD"; sleep 1; printf 'showtask --all\n'; sleep 1; printf 'exit\n'; } | timeout 8s "$BIN" run 2>&1)"
reminder_status=$?
set -e
if [ "$reminder_status" -ne 0 ]; then
  fail "run mode did not exit normally after a background reminder (status $reminder_status)."
fi
assert_contains "$reminder_output" "[Reminder] 立即提醒任务 starts at"
reminded_output="$(capture_ok "$BIN" "$USER_NAME" "$PASSWORD" showtask --all)"
if ! grep -F "立即提醒任务" <<<"$reminded_output" | grep -Fq "yes"; then
  fail "Reminder did not persist reminded=yes."
fi
set +e
repeat_output="$({ printf '%s\n%s\n' "$USER_NAME" "$PASSWORD"; sleep 1; printf 'exit\n'; } | timeout 8s "$BIN" run 2>&1)"
repeat_status=$?
set -e
if [ "$repeat_status" -ne 0 ]; then
  fail "run mode did not exit normally after reload (status $repeat_status)."
fi
assert_not_contains "$repeat_output" "[Reminder] 立即提醒任务 starts at"

pass "persistent reload after deletion"
run_ok "$BIN" "$USER_NAME" "$PASSWORD" deltask --id "$default_id"
run_ok "$BIN" "$USER_NAME" "$PASSWORD" deltask --id "$same_name_diff_time_id"
final_output="$(capture_ok "$BIN" "$USER_NAME" "$PASSWORD" showtask --all)"
assert_not_contains "$final_output" "默认属性任务"
assert_not_contains "$final_output" "复习高等数学第三章"
assert_contains "$final_output" "交互 引号 修改"

pass "all CLI tests passed"
