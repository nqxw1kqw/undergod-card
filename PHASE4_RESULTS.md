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
