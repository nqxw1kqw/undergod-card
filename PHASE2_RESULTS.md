# Phase 2 validation

Portable command: `UndergodCard/tests/run_portable.sh` (GCC C++17).
Policies: 0 attack-first, 1 defense-aware, 2 heal-aware; seeds 0–999,
each repeated exactly, with per-action multiset conservation checks.
Metrics include both wins and losses; policy heuristics are diagnostics.

Item 1: core 93 -> 95, balance 199366, zero failures.
Baseline damage 12: win rates 82/100/100%; mean turns 5.76/8.17/8.32;
mean HP 8.44/26.11/27.00; ultimates .83/1/1; unspent energy 1.46/.40/.35.

Item 2: damage sweeps 13 and 14 produced win rates 78.5/100/100% for both,
mean turns 5.75/7.17/7.31 and 5.75/6.97/7.13 respectively.
A one-point increase regresses attack-first wins (rage/turn opportunities change),
while the other policies already win every seed. Keep damage 12: current evidence
does not justify tuning or block breaking. Heavy retains hand/action compression;
cards now expose capped rage and effective healing, including zero healing.
Core 95 -> 98, balance unchanged at 199366; zero failures.

Item 5: core 98 -> 106, balance 199366, zero failures. Core advancement
remains at contact; displayed intent and incoming hand stay behind recovery
(1.15 logical seconds), followed by a locked .20-second player transition.
Lethal hits bypass the incoming transition. Forecast is suppressed during the
enemy action, so next intent is never paired with expiring block.
