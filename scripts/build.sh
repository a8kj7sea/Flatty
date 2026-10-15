#!/bin/bash
run_step() {
    local desc="$1"
    shift
    printf "\r\033[34m[ .... ]\033[0m %s" "$desc"
    local output
    if ! output=$("$@" 2>&1); then
        printf "\r\033[31m[FAILED]\033[0m %s\n" "$desc"
        echo -e "\n\033[31mError Output:\033[0m"
        echo "$output"
        exit 1
    fi
    printf "\r\033[32m[  OK  ]\033[0m %s\n" "$desc"
}

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR/.."

if [ ! -d "build" ]; then
    run_step "Setting up build" meson setup build
fi

run_step "Building project" ninja -C build
run_step "Running tests" meson test -C build