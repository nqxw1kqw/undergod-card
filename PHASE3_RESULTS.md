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
