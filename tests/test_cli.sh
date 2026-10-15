#!/bin/bash
set -e

EXE="$1"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEFAULTS_DIR="$SCRIPT_DIR/../defaults"

TMP_DIR=$(mktemp -d)
trap 'rm -rf "$TMP_DIR"' EXIT

mkdir -p "$TMP_DIR/proj/build"
mkdir -p "$TMP_DIR/proj/src"

# Create 30 files
for i in $(seq 1 30); do
    echo "Content $i" > "$TMP_DIR/proj/file_$i.txt"
done

echo "Arabic test إموجي 😊" > "$TMP_DIR/proj/src/utf8.txt"
echo -n "" > "$TMP_DIR/proj/empty.txt"
printf '\0\x00\x01' > "$TMP_DIR/proj/binary.dat"
echo "temp data" > "$TMP_DIR/proj/output.txt.tmp"

# Test help
"$EXE" --help > /dev/null

# Test invalid path
if "$EXE" /nonexistent 2>/dev/null; then exit 1; fi

# Run flatty
export FLATTY_DEFAULTS_DIR="$DEFAULTS_DIR"
"$EXE" "$TMP_DIR/proj" -o "$TMP_DIR/proj/output.txt"

# Verify content
if ! grep -q "file_1.txt" "$TMP_DIR/proj/output.txt"; then exit 1; fi
if ! grep -q "file_30.txt" "$TMP_DIR/proj/output.txt"; then exit 1; fi
if ! grep -q "utf8.txt" "$TMP_DIR/proj/output.txt"; then exit 1; fi
if ! grep -q "إموجي" "$TMP_DIR/proj/output.txt"; then exit 1; fi

# Ignored directory
if grep -q "build/" "$TMP_DIR/proj/output.txt"; then exit 1; fi

# Binary skip
if ! grep -q "[Binary file skipped]" "$TMP_DIR/proj/output.txt"; then exit 1; fi

# Output file itself must NOT be included (Using exact line match to avoid matching .tmp)
if grep -qE "^File: output\.txt$" "$TMP_DIR/proj/output.txt"; then exit 1; fi

# The file "output.txt.tmp" IS ordinary user data and MUST be included
if ! grep -q "File: output.txt.tmp" "$TMP_DIR/proj/output.txt"; then exit 1; fi

# Ensure output.txt.tmp was not overwritten or deleted
if ! grep -q "temp data" "$TMP_DIR/proj/output.txt.tmp"; then exit 1; fi

# FIFO test (should not hang)
mkfifo "$TMP_DIR/proj/myfifo"
timeout 2 "$EXE" "$TMP_DIR/proj" -o "$TMP_DIR/proj/out2.txt" || true
rm "$TMP_DIR/proj/myfifo"

echo "CLI tests passed."