# Gameplay and animation improvement proposal

Phase 1: proposals only. Priority is player impact, with dependencies reflected in the implementation order below. No gameplay, balance, assets, or renderer changes are included in this commit.

## Inspection and verified baseline

Reviewed all four runtime translation units, all four public headers, every gameplay JSON file, the complete headless checks, both project files, build/verification/packaging scripts, asset style/manifest structure, and repository documentation. The current runtime is the small native Card Quest implementation, not the historical web/RE code mentioned in documentation.

- `quest_core.hpp`: a 10-card deck (5 basic attacks, 3 shields, 1 heavy attack, 1 heal); draw 5, energy 3, player HP 50, boss HP 120. End turn discards the entire remaining hand; an empty draw pile reshuffles discard. Block clears on its owner's next turn/action. Attacks grant 8 rage per card and shields 5; a free, 100-rage ultimate deals 40 through block. Healing grants no rage.
- Boss intent is visible: attack 8–13 with nominal 70% attack weight, otherwise gain 6 block. An opening attack and maximum one consecutive guard are enforced, so effective attack frequency differs from the raw weight. There is one enemy, one deck, no escalation, rewards, difficulty selection, or persistence.
- `Combat::Reset`, `Game::NewRun`, and all UI/keyboard restart routes default to seed 1337. Normal rematches repeat the same shuffle and intent streams for equivalent decisions. Separate shuffle/intent RNG streams are a valuable invariant.
- `Game` permits one action at a time. Basic attack contact is at 0.32 logical seconds, completion at 0.65; enemy contact is at 0.77, completion at 1.15. Non-damage cards resolve immediately but retain the 0.65 action lock. `ActEnemy()` also rolls the next intent and draws the next hand at contact, before enemy recovery finishes.
- `main.cpp` caps elapsed time at 0.1 seconds and aims for roughly 60 Hz. `Game::Update` scales almost all visuals by 0.65 × speed; x1/x2/x3 remain available while busy. Toasts alone use real time. Thus a card takes about 1 second at x1, an enemy action 1.77 seconds, and the ultimate 5.54 seconds. Lost wall time during a stall is intentionally not caught up by the main loop.
- Actors have four-frame poses: idle loops at 4 logical fps; most actions clamp at their final frame at 8 fps and reset after 0.65 seconds. Idle bob uses a shared sine wave, including during attacks. Lunges use quadratic approach, a hold, and linear return; hits already have recoil and a brief additive flash. Cards only jump upward on hover, disappear at resolution, and immediately reflow; draw/discard have no movement.
- Ultimate cues provide dim, eased zoom, spear rise/fall, four-frame impact, shake and flash. Impact/end are idempotent under skip/frame jumps. The arena is an offscreen target, so world transforms spare HUD and hit testing. Normal attacks have no impact sheet, particles, or camera shake. There is no death animation, transition banner, sound, or result fade.
- Existing damage events retain absorbed and HP damage, but popups show absorbed damage only for a fully blocked hit; partial absorption is omitted. Concurrent effects can place overlapping popups at the same anchor. HUD bars update immediately. Intent can show the *next* action during the current enemy recovery.

**Linux verification performed:** compiled `tests/core_checks.cpp`, `src/game.cpp`, and `src/quest_data.cpp` with GCC, C++17, `-Wall -Wextra -Wpedantic`, and `-IUndergodCard/include`; copied JSON beside an executable under `/tmp/undergod-proposal-check` because tests resolve data relative to the executable. Result: **93 checks, 0 failures**. Existing misleading-indentation warnings occur in `Combat::ActEnemy`, `Game::ApplyMotion`, and `Game::Ultimate`. No source was changed. DxLib rendering, Windows input, MSVC builds, visual quality, and actual player balance have not been verified here. The documented seed-1337 victory is historical evidence, not a new multi-seed balance assessment.

## Prioritized proposals

Sizes: **S** = localized change; **M** = coordinated controller/core/UI work; **L** = a new gameplay subsystem. These are relative scope estimates, not time promises. **Linux-safe** means the behavior can be implemented and meaningfully verified headlessly. **Split** means portable logic can be verified here but DxLib presentation requires Windows. **Windows-required** means perceived quality/input/rendering is the principal acceptance gate. All paths below are relative to `UndergodCard/` unless stated otherwise.

### 1. Make rematches varied, reproducible, and measurable — Gameplay · M · Split

**What / why:** distinguish “New match” (new explicit seed) from “Retry same seed”; show the seed on results. Preserve `NewRun(seed)` for tests. Add a headless multi-seed balance runner comparing at least attack-first, defense-aware, and heal-aware policies, reporting wins, turns, HP, rage use, and unspent energy. This immediately gives repeat matches variety and creates evidence for later tuning instead of treating one successful scripted run as balance proof.

**Touches:** `src/main.cpp::WinMain` (session seed generation and restart keys); `include/game.hpp` / `src/game.cpp::Game::NewRun` (record seed); `src/ui.cpp::Ui::Draw, Ui::Click` and `include/ui.hpp` (explicit retry/new-match controls); `tests/core_checks.cpp`; proposed `tests/balance_checks.cpp` (standalone portable runner); `CoreChecks.vcxproj` only if integrating runner execution into that build workflow.

**Risks / acceptance:** do not make `Combat::Reset()` implicitly random or consume combat RNG to choose a seed. Same explicit seed and actions must reproduce hands, intent, and outcome; fresh-match routes must differ. Policy win rates are diagnostic, not human difficulty targets. Linux verifies reproducibility and distributions; Windows verifies controls and labels.

### 2. Give existing cards distinct tactical jobs — Gameplay · M · Split

**What / why:** heavy attack currently deals exactly two basics' damage for two energy, but grants 8 rage rather than 16. Its advantage is hand/action compression, so its reward needs a clearer identity. Prototype a modest data-only heavy-damage advantage first, then compare an alternative explicit block-breaking effect if needed; choose one using item 1's measurements. Expose rage gain and effective capped healing on cards. Keep shields valuable against announced attacks, and clarify that excess block expires. Avoid adding many cards before the four existing choices are meaningful.

**Touches:** `data/cards.json`, optionally `data/balance.json`; `include/quest_core.hpp::CardDef, Combat::Play` only if a new effect wins the prototype comparison; `src/quest_data.cpp::LoadConfig` for any schema extension; `src/ui.cpp::Ui::Card, Ui::Draw`; `tests/core_checks.cpp`, proposed `tests/balance_checks.cpp`.

**Risks / acceptance:** no final damage number is justified yet. New effects need explicit resolution order, validation, and tests for partial/full block and lethal hits. Do not silently infer rules from card IDs. Linux-safe for rules and sweeps; Windows required for text fit and usability, particularly narrow hands.

### 3. Animate card play, draw, discard, and hand reflow — Animation / feedback · M · Split

**What / why:** add visual card instances with stable presentation IDs and start/end rectangles. A selected card lifts toward a play slot, resolves at the existing contact boundary, then travels to discard; remaining cards ease into position. At turn start, stagger a short draw from the pile; end-turn cards sweep to discard. Reserve the selected slot while an attack is pending. This makes ownership, spending, and deck flow readable and removes abrupt hand jumps.

**Touches:** `include/game.hpp` / `src/game.cpp::Game::Play, EndTurn, ApplyMotion, Update, NewRun` (visual snapshots and lifecycle); `include/ui.hpp::Ui::CardRect, Ui::Card`; `src/ui.cpp::Ui::Card, Ui::Draw, Ui::Click`; `tests/core_checks.cpp`. If core events are extended, `include/quest_core.hpp::Event, Combat::Draw, Play, EndPlayer` must carry value-owned identity information.

**Risks / acceptance:** repeated card IDs cannot identify individual copies. Do not keep references/indices into a mutated hand or call `Play` twice. Logical hit tests must track displayed slots or remain locked during movement. Linux checks snapshots, conservation, reset, and cadence independence; Windows checks trajectories, overlap, and pointer alignment. Respect existing contact timing and speed controls.

### 4. Strengthen ordinary impacts with a bounded feedback layer — Animation / feedback · M · Split

**What / why:** use damage events to spawn a small contact spark/dust burst, brief decaying world shake, and distinct shield/heal pulses. Scale feedback modestly for heavy attacks; retain the existing hit flash/recoil. Display both absorbed and HP damage on partial blocks and stack popup lanes. Add an optional short visual hit pause only after the basic effects work; it must pause presentation consistently without repeating callbacks. This makes ordinary cards feel consequential rather than saving nearly all effects for the ultimate.

**Touches:** `include/game.hpp::Popup, CinematicVisual` (or a separate general-purpose effect state); `src/game.cpp::Game::ConsumeEvents, Update, NewRun, CueStep`; `src/ui.cpp::Ui::Actor, Ui::World`; `include/quest_core.hpp::Event` only if source/strength metadata is needed; `tests/core_checks.cpp`.

**Risks / acceptance:** current shake/flash timers tick only while cinematic is active; simply setting them on normal damage would leave effects stuck. Generalize expiry and cleanup first. Effects must not consume shuffle/intent RNG; use deterministic visual variation or a separate stream. Cap particle counts, restore DxLib blend state, and offer reduced flash/shake. Linux verifies expiry and unchanged combat at all speeds; Windows judges intensity, clipping, and comfort. Reuse existing sheet/primitives before commissioning assets.

### 5. Separate visible turn transitions from enemy recovery — Gameplay clarity / animation · M · Split

**What / why:** retain the resolving enemy intent while it acts; reveal the next intent and incoming hand with a short “Your turn” transition after recovery. Show explicit enemy attack/guard/recovery status instead of one generic busy label. Add an incoming-HP-loss preview (`max(0, intent damage − player block)`) and explain block expiry. This connects defenses to consequences and avoids suggesting the next boss action is already underway.

**Touches:** `include/game.hpp` / `src/game.cpp::Game::EndTurn, ApplyMotion, Update, CanAct` (display intent/hand snapshots and transition state); `src/ui.cpp::Ui::Draw, Ui::World`; `tests/core_checks.cpp`. Prefer keeping `include/quest_core.hpp::Combat::ActEnemy, BeginPlayer` semantics intact and delaying presentation only; split core turn advancement only if snapshots prove inadequate.

**Risks / acceptance:** never show a forecast calculated from a next-turn intent against current-turn block. Retain input lock through recovery/draw, but keep speed and cinematic skip available. Linux tests exact contact/recovery boundaries, lethal enemy actions, and no extra draw/intent rolls; Windows verifies labels and transitions remain readable at x3.

### 6. Tune action pacing and frame sampling as one coherent system — Animation / pacing · M · Split

**What / why:** centralize windup/contact/recovery durations and derive pose sampling from action progress. Give shield/heal shorter recovery than a full lunge; preserve a readable enemy windup. Smooth lunge return, suppress idle bob during attacks, and stagger idle phases between actors. Age newly spawned hit effects by overshoot when a large update crosses contact so feedback is sampled consistently. Keep x1's deliberate character; measure before shortening the ultimate, whose current skip support already addresses repetition.

**Touches:** `include/game.hpp` / `src/game.cpp::Game::Play, ApplyMotion, Update, ActorOffset`; `src/ui.cpp::Ui::Actor`; `src/main.cpp::WinMain` (document or adjust stall/focus timing policy only if required); `data/cinematic.json` and `src/quest_data.cpp::LoadTimeline` if cinematic timings change; `tests/core_checks.cpp`.

**Risks / acceptance:** pose lifetimes, sprite fps, contact, and return duration are currently separate constants. Adjusting only one can cause a pose reset mid-lunge. Preserve exactly-once damage, speed-change continuity, skip cleanup, and fine/coarse-step combat equivalence. Linux checks timing boundaries and visual samples; Windows determines whether the new pacing feels better. Avoid an input queue until these boundaries are stable.

### 7. Add a restrained, telegraphed boss difficulty curve — Gameplay · M · Split

**What / why:** introduce selectable gentle/standard/challenge profiles and one clearly announced second-stage pattern, such as a charged attack preceded by a preparation intent. Prefer a learnable pattern over hidden damage scaling or longer HP bars. Preserve current behavior as the standard baseline until multi-seed results justify changes; warn before escalation and expose exact incoming values.

**Touches:** `include/quest_core.hpp::EnemyDef, Intent, Combat::RollIntent, ActEnemy, Reset`; `src/quest_data.cpp::LoadConfig`; `data/enemies.json`, optionally `data/balance.json`; `src/ui.cpp::Ui::Draw, Ui::Click`, `include/ui.hpp` (profile/intent presentation); `src/main.cpp::WinMain` (profile selection input); `tests/core_checks.cpp`, proposed `tests/balance_checks.cpp`.

**Risks / acceptance:** threshold transitions must not replace an already advertised action without notice. Charged attacks can be unfair with a random five-card hand; measure unavoidable damage and starting-hand variance. Test profile bounds, phase entry once, and reproducible intent sequences. Linux verifies rules/distributions; Windows and human play verify comprehensibility and difficulty. This is a rules extension, not a cosmetic enemy reskin.

### 8. Add defeat, victory, and result transitions — Animation / motivation · M · Split

**What / why:** on lethal impact, keep the defeated actor down via a short fade/tilt/desaturation and dust, then fade in results with turns, HP remaining, seed, and a concise performance recap. Use existing sprites first; add a victory bounce for the survivor. The current hit pose resets to idle and the result panel appears as soon as busy ends, weakening the final blow.

**Touches:** `include/game.hpp` / `src/game.cpp::Game::ConsumeEvents, Update, Result, NewRun` (terminal visual state and summary counters); `src/ui.cpp::Ui::Actor, Ui::Draw, Ui::Click`; `src/main.cpp::WinMain` (restart gating if transition is busy); `tests/core_checks.cpp`.

**Risks / acceptance:** core win/loss must remain immediate; only presentation is delayed. A skipped lethal ultimate must enter the same terminal sequence once. Reuse transforms rather than inventing nonexistent death frames. Linux tests terminal locking, summary accounting, reset, and skip; Windows checks sprite pivots, result fade, and legibility.

### 9. Make hand and pile information accessible at supported sizes — Gameplay clarity · M · Split

**What / why:** add a focused card detail view with wrapped description, cost, rage/effect preview, and why a card is unavailable; show draw/discard contents on request without revealing draw order. Support keyboard focus/navigation or keys 6–0 for configured hands up to 10. Current `CardRect` narrows cards but text does not wrap, and only slots 1–5 have keyboard handlers despite draw allowing 10.

**Touches:** `include/ui.hpp::Ui::CardRect` and detail/pile UI state; `src/ui.cpp::Ui::Card, Ui::Draw, Ui::Click`; `src/main.cpp::WinMain` input handling; `include/game.hpp` only for shared selection/preview data if needed; `tests/core_checks.cpp` for portable preview/layout helpers.

**Risks / acceptance:** inspect piles as a multiset, not an ordered future-hand spoiler. Keep descriptions consistent with the actual resolver. Linux can check rectangle bounds/preview calculations; Windows is required for UTF-8 wrapping, fonts, focus, ten-card fit, mouse scaling, and overlay dismissal.

### 10. Add a small between-match reward loop — Gameplay / replayability · L · Split

**What / why:** after the single-match foundation is tuned, prototype a three-battle session with a choice of one card upgrade/replacement between wins, persistent HP within the session, and a final recap. Start with existing card art and two upgrade options per reward, not a map or a large content set. Distinct decks and adaptation provide a stronger reason to continue than repeatedly fighting the same boss with unchanged cards.

**Touches:** proposed `include/run.hpp` (session/deck/reward state); `include/quest_core.hpp::Config, Combat::Reset` (accept a session deck/config without accumulating copies); `include/game.hpp` / `src/game.cpp::Game::NewRun` and new session transitions; `src/quest_data.cpp::LoadConfig`, `data/cards.json`, `data/enemies.json`; `src/ui.cpp::Ui::Draw, Ui::Click`, `include/ui.hpp`, `src/main.cpp::WinMain`; `tests/core_checks.cpp` plus proposed session checks. Update project file includes if adding a compiled source.

**Risks / acceptance:** largest scope and balance burden; distinguish retry battle, next battle, and restart session. Do not combine with save/load in the first prototype. Linux verifies reward legality, deck conservation, HP carryover, and session determinism; Windows/human play verifies flow and motivation. Defer until items 1, 2, and 7 establish sustainable fights.

### 11. Improve ultimate camera continuity and effect controls — Animation · S–M · Split

**What / why:** capture a zoom cue's starting zoom and interpolate from that value, then ease back before the end instead of resetting instantly. The present formula always starts from 1, so additional zoom cues could snap; the final cleanup also resets directly to 1. Add reduced-motion/flash settings shared with item 4 and maintain the existing skip button. This polishes the current signature move without replacing its working spear/contact design.

**Touches:** `include/game.hpp::CinematicVisual`; `src/game.cpp::Game::CueStep, Update, Ultimate, SkipCinematic, NewRun`; `data/cinematic.json`; `src/ui.cpp::Ui::World, Ui::Draw, Ui::Click`, `include/ui.hpp`; `src/quest_data.cpp::LoadTimeline` if settings require new cue fields; `tests/core_checks.cpp`.

**Risks / acceptance:** skip/reset must restore neutral camera immediately even if normal exit is eased. Test multiple consecutive zoom cues and crossing several cues in one update. Linux verifies interpolation/cleanup; Windows verifies cropping within the 360-pixel arena and perceived continuity. No new assets required.

### 12. Add a minimal sound feedback layer — Feedback · M · Windows-required

**What / why:** play short, distinct card-select, attack-contact, shield, heal, ultimate-release/impact, and result sounds with a mute/volume control. Synchronize sounds to resolved actions rather than every rendered frame. This supplies missing feedback at low visual cost after event lifecycle work is stable.

**Touches:** `src/ui.cpp::Ui::Init, Shutdown` or proposed `src/audio.cpp` / `include/audio.hpp` (DxLib sound handles); `src/game.cpp::Game::ConsumeEvents, CueStep` (value-owned audio cues); `src/main.cpp::WinMain` (dispatch); `include/game.hpp`; proposed licensed files under `assets/audio/`; `UndergodCard.vcxproj` asset-copy/source lists; `package.ps1`; `THIRD_PARTY.md`; `src/main.cpp::Smoke` for optional asset checks.

**Risks / acceptance:** source/licensing, volume clipping, device availability, and skip behavior. Audio failure must not block gameplay. Keep sounds outside the core and avoid replaying queued sounds after reset or emitting a burst when skipping cinematic cues. Linux checks cue counts/cleanup only; Windows must build and verify playback, mixing, muting, and packaging. Asset provenance must be documented.

## Recommended phase 2 sequence and gates

Implement **1 → 2 → 5 → 3 → 6 → 4 → 8 → 9 → 11 → 7 → 10 → 12**. Priority and build order differ: visible turn boundaries support card draw animation, and stable timing should precede hit pause/juice. Item 12 may move earlier if licensed sounds and a Windows test machine are already available. Treat items 7 and 10 as separate gameplay milestones, not prerequisites for the initial polish release.

First deliverable: fresh/retry seeds, balance baseline, distinct existing-card choices, clear turn state, and card movement. Second: motion/impact/death polish and information access. Then validate difficulty/reward expansion against the measured baseline. For each behavior change, preserve RNG isolation, deck conservation, input locks, exactly-once contact/ultimate impact, and reset cleanup. Extend the existing portable checks with meaningful boundary cases rather than snapshotting implementation details. Run Windows Debug/Release and expand `src/main.cpp::Smoke` captures for changed presentation; manually play at all three speeds and scaled window sizes before claiming visual improvements are verified. Do not reinterpret the current 93 passing checks as proof of future visual quality or balanced new mechanics.
