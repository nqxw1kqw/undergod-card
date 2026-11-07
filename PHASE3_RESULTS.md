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
