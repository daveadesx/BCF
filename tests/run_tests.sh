#!/bin/bash

# Test runner for betty-fmt
# Runs integration tests with input/expected pairs

FORMATTER="../betty-fmt"
TESTS_DIR="./cases"
PASSED=0
FAILED=0

if [ ! -x "$FORMATTER" ]; then
    echo "Error: Formatter not found or not executable: $FORMATTER"
    echo "Please run 'make' first"
    exit 1
fi

echo "Running integration tests..."
echo

for test_dir in "$TESTS_DIR"/*/; do
    if [ ! -d "$test_dir" ]; then
        continue
    fi

    test_name=$(basename "$test_dir")
    input_file="$test_dir/input.c"
    expected_file="$test_dir/expected.c"

    if [ ! -f "$input_file" ] || [ ! -f "$expected_file" ]; then
        echo "SKIP: $test_name (missing input.c or expected.c)"
        continue
    fi

    output=$($FORMATTER "$input_file" 2>&1)
    expected=$(cat "$expected_file")

    if [ "$output" = "$expected" ]; then
        echo "PASS: $test_name"
        ((PASSED++))
    else
        echo "FAIL: $test_name"
        echo "  Expected:"
        echo "$expected" | head -5
        echo "  Got:"
        echo "$output" | head -5
        echo
        ((FAILED++))
    fi
done

echo
echo "Results: $PASSED passed, $FAILED failed"

if [ $FAILED -gt 0 ]; then
    exit 1
fi

exit 0
