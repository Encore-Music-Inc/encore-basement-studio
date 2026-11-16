#!/usr/bin/env bash
# Ends a sanitizer run: prints what the run wrote (its output, where the
# sanitizers report: with AddressSanitizer built in as well, GCC's
# UndefinedBehaviorSanitizer ignores log_path and always writes there) and
# fails if it holds any finding, or if the run itself failed or timed out.
#
#   report.sh <the run's output file> <exit code of the run>
set -uo pipefail
output="${1:?usage: report.sh <output file> <exit code>}"
code="${2:?exit code}"
cat "$output" 2>/dev/null || true
findings=$(grep -c 'runtime error:\|ERROR: AddressSanitizer' "$output" 2>/dev/null || true)
findings=${findings:-0}
[ "$code" -eq 124 ] && echo "::error::the run timed out"
[ "$code" -ne 0 ] && [ "$code" -ne 124 ] && echo "::error::the run failed ($code)"
[ "$findings" -gt 0 ] && echo "::error::the sanitizers reported $findings finding(s), listed above"
[ "$findings" -eq 0 ] && [ "$code" -eq 0 ] && echo "No sanitizer findings"
if [ "$code" -ne 0 ] || [ "$findings" -gt 0 ]; then exit 1; fi
exit 0
