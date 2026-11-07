# Phase 3 validation

Portable command: `UndergodCard/tests/run_portable.sh`, GCC C++17.

Item 6: core 120 -> 124; balance 199366 -> 199366, zero failures.
Attack timing remains player 0/.32/.65 and enemy .45/.77/1.15
(start/contact/end, logical seconds). Support recovery is .45, with its
card trajectory .20 to play + .22 to discard, leaving .03 settling margin.
Attack pose frames sample the action interval; lunge recovery uses smoothstep,
idle bob is suppressed outside idle and actor idle phases differ. Contact effects
include overshoot. Ultimate timing and main-loop stall policy remain unchanged.
All Phase 2 checks pass; standard balance remains 82/100/100%.

Windows rendering and perceived pacing are statically verified only.

Item 4: core 124 -> 132; balance 199366 -> 199366, zero failures.
General effect aging now runs on every update, including outside cinematics;
new effects receive contact overshoot and already-expired effects are discarded.
A separate seeded visual RNG supplies bounded contact dust/sparks and shield/heal
pulses (64 particles maximum). Ordinary shake lasts .18 logical seconds, decays,
and is capped at 2 pixels / 4 for heavy contact. Particles last .4 seconds;
popups last .95 seconds with eight reserved lanes per actor. Partial absorption
creates separate absorbed and HP-loss popups. The FX button suppresses shake and
flashes. These are presentation bounds, not balance changes; intensity still needs
Windows assessment. Blend and clipping state follow existing restoration patterns.
Portable runner compiles the shared data source once for both executables.
Standard policy wins remain 82/100/100%; all Phase 2 invariants remain covered.
DxLib drawing, FX control, clipping, overlap and perceived intensity are statically
verified only; Windows Debug/Release and manual x1/x2/x3 checks remain required.

## Item 7: measured profiles and one stage pattern

Core 132 -> 152; balance 199366 -> 569849; zero failures. The balance count
now includes all three profiles (different fight lengths change assertion counts).
Each profile/policy uses seeds 0–999, repeated twice; every card/end/enemy action
checks deck conservation, and repeats compare outcomes plus a hand/intent trace. Default runs also assert
the standard profile's 820/1000/1000 wins as a regression contract.
The original 120 core checks remain included. Added checks cover advertised-intent
preservation, exact threshold, harmless preparation, absorbed charged damage,
one-time entry, reset, profile bounds/locks, shuffle/intent isolation, reproducible
sequences, ultimate idempotence and fine/coarse cadence at x1/x2/x3.

Profiles are selected by the difficulty button or D; changing profile retries the
same seed and is blocked during actions. Retry/new match preserve the profile.
`quest_core.hpp`, `game.hpp`, `quest_data.cpp`, `enemies.json`, `ui.hpp`,
`ui.cpp`, `main.cpp`, and both check sources carry this item.

### Final measurements

Means include losses. Policies remain the Phase 2 heuristics, not optimal players.
HP is terminal HP; unavoidable damage is defined below.

| Profile | Policy | Win % | Turns | HP | Ultimates | Unspent energy | Unavoidable / announced attack | Charged announcements / fight | Unavoidable / charged |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| gentle | attack-first | 99.4 | 5.89 | 18.57 | .99 | 1.46 | 3.39 | 1.00 | 6.65 |
| gentle | defense-aware | 100 | 7.42 | 33.51 | 1.00 | .58 | 3.13 | 1.00 | 6.73 |
| gentle | heal-aware | 100 | 7.42 | 33.84 | 1.00 | .49 | 3.13 | 1.00 | 6.72 |
| standard | attack-first | 82 | 5.76 | 8.44 | .83 | 1.46 | 3.64 | 0 | 0 |
| standard | defense-aware | 100 | 8.17 | 26.11 | 1.00 | .40 | 3.59 | 0 | 0 |
| standard | heal-aware | 100 | 8.32 | 27.00 | 1.00 | .35 | 3.59 | 0 | 0 |
| challenge | attack-first | 42.2 | 5.31 | 8.04 | .51 | 1.45 | 9.84 | 1.00 | 26.56 |
| challenge | defense-aware | 91.0 | 7.66 | 12.83 | 1.00 | .59 | 7.97 | 1.00 | 26.57 |
| challenge | heal-aware | 93.2 | 7.88 | 14.15 | 1.00 | .51 | 8.18 | 1.07 | 26.56 |

Standard executes the original intent-roll/action path, with unchanged damage,
HP, attack weight, guard limit, shuffle/intent seeds and RNG call order. It has
no stage entry or pattern. All five reported baseline metrics reproduce Phase 2.

### Chosen numbers and comparison sweeps

Measurements preceded the final selection. Runner CLI is
`balance_checks [heavyDamage] [challengeCharge] [stageThresholdPercent] [gentleReduction] [gentleCharge]`.
Defaults load JSON. All sweeps below use 1,000 seeds per policy and repeat checks.
Heavy damage stays 12; player HP 50, boss HP 120, energy/deck/rage and standard
8–13 damage remain unchanged because the Phase 2 baseline is an explicit invariant.

| Challenge charge (threshold 60%) | Attack-first win % | Defense-aware win % | Heal-aware win % |
|---:|---:|---:|---:|
| 28 | 55.5 | 98.9 | 99.6 |
| 32 | 44.1 | 95.5 | 96.8 |
| **34** | **42.2** | **91.0** | **93.2** |

Choose **34**: compared with 32 it adds 4.5/3.6 percentage points of defensive
losses for only 1.9 points of attack-first losses. Compared with 28 it avoids
leaving defensive policies nearly automatic. Attack-first still wins 422 seeds;
this is deliberately a harder optional profile, not a change to standard.

| Stage threshold (charge 34) | Attack-first win % | Defense-aware win % | Heal-aware win % |
|---:|---:|---:|---:|
| 50% | 98.4 | 96.8 | 96.9 |
| **60%** | **42.2** | **91.0** | **93.2** |
| 70% | 8.9 | 90.5 | 83.3 |

Choose **60%** (72 boss HP): 50% lets burst damage bypass the threat, while
70% punishes attack-first excessively. Threshold crossing never changes the
currently visible intent. Entry is tested only when rolling after that action.
The stage repeats **preparation -> charged attack -> one ordinary intent**.
Preparation grants neither block nor damage; it announces the next charged value
for a full turn, and the charged intent then announces it again with the new hand.
The ordinary-intent respite bounds cadence: measured charged announcements are
1.00/1.00/1.07 per challenge fight. No hidden scaling, HP increase, or extra pattern.

Gentle keeps the same teaching pattern/threshold with **6–11** ordinary damage
(a reduction of **2**) and a **14** charge. A reduction of 1 with charge 14 gave
99.2/100/100% wins and terminal HP 15.86/30.92/31.28; reduction 2 gives
99.4/100/100% and 18.57/33.51/33.84, providing a clearer recovery cushion.
At reduction 2, charges 12/14/16 gave attack-first wins 99.6/99.4/99.2%
and HP 19.71/18.57/17.43. Defensive wins stayed 100%; defense-aware HP was
35.15/33.51/32.10. Choose the middle **14**: teaches a visibly stronger-than-normal
hit with average shield-unpreventable charged damage about 6.7 instead of 4.9
(12) or 8.7 (16). Profile bounds reject thresholds outside 1–99%, charged damage
outside 1–100, and ordinary reductions outside 0–minimum damage; these validation
limits prevent invalid content, not intended difficulty settings.

### Unavoidable-damage analysis

At each announced attack, the runner solves a 0/1 energy-budget maximum-block
problem over the current hand and reports `max(0, intent - maximum block)`.
This is damage shields cannot prevent **if that attack resolves**, not guaranteed
HP loss or a proof a seed cannot be won. Killing the boss, an ultimate, and healing
can avert/sustain it; observations include attacks killed before resolution.
No future draw order is exposed to the policy. Preparation removes one hostile
action but does not carry current block through to the next hand.

The stock deck has three 5-block shields and three energy: even the best charged
hand can absorb at most 15. Challenge's 34 therefore leaves at least **19**
shield-unpreventable damage and observed means **26.56–26.57**, versus gentle's
**6.65–6.73**. This is a substantial optional burst, compensated by preparation,
the ordinary respite, unchanged boss HP and the measured 91–93.2% defensive wins.
It is not universally blockable; do not claim all losses are avoidable. Human
playtesting may favor a lower charge despite these policy results.

### Starting-hand variance

Same opening hands across profiles: 0/1/2/3 shields occur in **74/425/412/89**
seeds. Percentages below condition on that opening count; they include the rest
of each seed's draw and intent sequence and do not isolate causation.

| Profile / policy | 0 shields | 1 shield | 2 shields | 3 shields |
|---|---:|---:|---:|---:|
| gentle / attack-first | 97.30 | 99.53 | 100 | 97.75 |
| gentle / defense-aware | 100 | 100 | 100 | 100 |
| gentle / heal-aware | 100 | 100 | 100 | 100 |
| standard / attack-first | 75.68 | 81.41 | 83.98 | 80.90 |
| standard / defense-aware | 100 | 100 | 100 | 100 |
| standard / heal-aware | 100 | 100 | 100 | 100 |
| challenge / attack-first | 50.00 | 42.82 | 37.38 | 55.06 |
| challenge / defense-aware | 82.43 | 92.71 | 91.75 | 86.52 |
| challenge / heal-aware | 87.84 | 94.82 | 93.45 | 88.76 |

All **16** possible opening multisets occurred. Exact-hand win-rate ranges:
gentle attack-first 50–100%, both defensive policies 100%; standard attack-first
50–92.86%, both defensive policies 100%; challenge attack-first 0–78.57%,
defense-aware 0–95.08%, heal-aware 50–96.55%. The weakest exact hand is cookie,
heavy, shield, shield, shield, observed in only **2** seeds: gentle attack-first
1/2, standard attack-first 1/2, challenge attack-first/defense-aware 0/2 and
heal-aware 1/2. These tiny strata are warnings for human testing, not reliable
population estimates. The runner prints raw wins/counts and exact-hand strata
so future changes can be compared without concealing this variance.

### Remaining platform verification

No Windows build was available. DxLib sprites/particles/pulses, decaying shake,
blend/clipping restoration, popup lane readability, difficulty/FX controls,
preparation labels and poses, fonts, keyboard/mouse input, window scaling and
x3 readability are **statically verified only**. Windows Debug/Release, smoke
captures, and manual play at every speed remain unverified. Items 8–12 are
still deferred. No GitHub push was performed.
