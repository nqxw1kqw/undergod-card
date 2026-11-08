#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
out="${TMPDIR:-/tmp}/undergod-portable"
mkdir -p "$out"
cp -r "$root/data" "$out/"
flags=(-O2 -std=c++17 -Wall -Wextra -Wpedantic -I"$root/include")
g++ "${flags[@]}" -c "$root/src/quest_data.cpp" -o "$out/quest_data.o"
g++ "${flags[@]}" "$root/tests/core_checks.cpp" "$root/src/game.cpp" "$out/quest_data.o" -o "$out/core_checks"
g++ "${flags[@]}" "$root/tests/balance_checks.cpp" "$out/quest_data.o" -o "$out/balance_checks"
"$out/core_checks"
"$out/balance_checks"
