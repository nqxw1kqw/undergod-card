# Phase 4 validation

Portable command: `UndergodCard/tests/run_portable.sh` (GCC C++17).

## Item 8

Core 152 -> 161; balance 569849 -> 569849; zero failures.
Core resolution remains immediate. A separate terminal sequence locks restart/profile
controls through 1.5 logical seconds, fades results from .8 to 1.5 seconds, holds
the defeated actor using an existing hit sprite with monochrome treatment, tilt,
fade and dust, and bounces the survivor (suppressed by reduced feedback).
Event-owned accounting records cards, actual HP damage, absorbed player damage,
effective healing and ultimates; results include turns, remaining HP and seed.
Skip before/after lethal ultimate impact enters once; reset clears all terminal data.
Two earlier immediate-result assertions now assert the requested terminal transition;
core resolution and absence of an incoming turn transition remain covered.
Files: game.hpp, game.cpp, ui.cpp, core_checks.cpp.

Windows rendering, monochrome filter/pivots, bounce/dust, result fade/legibility,
UTF-8 text and controls are statically verified only. Windows Debug/Release and
manual x1/x2/x3/scaled-window checks remain unavailable.

## Item 9

Core 161 -> 177; balance 569849 -> 569849; zero failures.
Focused/hovered card details show cost, UTF-8-aware pixel-wrapped description,
actual HP damage/absorption, block, effective heal, capped rage and unavailability.
Previews execute the resolver on a value-owned copy, including combined effects.
Draw/discard inspection converts piles to sorted ID/count maps, never draw order.
P/O and pile clicks open inspection; Esc closes; arrows focus every hand slot,
Space plays focused cards, and keys 1–0 play all ten supported slots. Focus/pile
state clears on reset/actions. Compact card text clips within its card; detail
text supplies the full description. Hand layout clamps invalid bounds.
Files: game.hpp, game.cpp, ui.hpp, ui.cpp, main.cpp, core_checks.cpp.
Checks cover resolver preview values/nonmutation/reasons, multiset permutation
invariance, all 1–10 layouts, invalid bounds, focus traversal/empty/reset.

UTF-8/font wrapping and fit, overlays/dismissal, keyboard/mouse controls,
ten-card layout/clipping and scaled pointer alignment are statically verified
only; Windows builds and manual checks remain required.
