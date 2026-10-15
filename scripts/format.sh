#!/bin/bash
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR/.."
find src include tests -type f \( -name '*.c' -o -name '*.h' \) -exec clang-format -i {} +
echo "Formatting complete."