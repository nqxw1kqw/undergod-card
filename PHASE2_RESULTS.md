# Phase 2 validation

Portable command: `UndergodCard/tests/run_portable.sh` (GCC C++17).
Policies: 0 attack-first, 1 defense-aware, 2 heal-aware; seeds 0–999,
each repeated exactly, with per-action multiset conservation checks.
Metrics include both wins and losses; policy heuristics are diagnostics.

Item 1: core 93 -> 95, balance 199366, zero failures.
Baseline damage 12: win rates 82/100/100%; mean turns 5.76/8.17/8.32;
mean HP 8.44/26.11/27.00; ultimates .83/1/1; unspent energy 1.46/.40/.35.
