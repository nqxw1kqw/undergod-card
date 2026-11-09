#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
# Both suites share the same compiled core, data loader and controller.
make -C "$root" core CXX="${CXX:-g++}" \
    CXXFLAGS='-O2 -std=c++17 -Wall -Wextra -Wno-unused-parameter -Wno-misleading-indentation -Iinclude'
"$root/bin/linux/core_checks" --integrate-balance "${BALANCE_SEEDS:-20}"
