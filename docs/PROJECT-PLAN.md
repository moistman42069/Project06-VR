# Project 06 VR project plan

Status: 0.1.4 was headset-confirmed for immersive VR/input/menu. 0.1.7 regressed
startup. Replacement 0.1.8 is awaiting headset testing; no public release.

## Current foundation

- Standalone ARM64 Quest APK wrapper around the supplied Unity IL2CPP game.
- Native OpenXR display provider with immersive third-person stereo as the default.
- Quest Touch mapping for sticks, buttons, triggers, grips and stick clicks.
- Both-stick-click VR menu with immersive, 3D screen and 2D theatre modes.
- Separate HUD camera path, active `OutlineCamera` fallback, system recenter reserved.
- Categorized VR menu (UI, VR, Graphics, Running, Homing, Spindash, Haptics, System); immediate screen-overlay redirect.
- Experimental first person: player-relative adjustable eye anchor, Sonic renderer hide, and
  render-only glove meshes following Touch grip poses. Disabled by default; the
  user must verify camera alignment, model proportions and visibility in headset.
- Guarded water-entry/booster trigger sweeps and WaterSlide exit telemetry. The SonicFast ground-grace experiment is disabled.
- Opt-in arm-swing running, native-target pull homing, crouch spindash and configurable game-action haptics in candidate 0.1.8.
- Per-run logs in `Downloads/P06Quest`, with app-private fallback.
- Static build, package, archive and menu/view-math checks passing.

## Near-term milestones

### 0.1 — first headset bring-up

1. Verify package launch and OpenXR session creation on Quest 3.
2. Verify title screen, main menu and gameplay input mapping.
3. Verify third-person stereo, head rotation, head translation and recenter.
4. Verify HUD, menus, scene transitions and all three display modes.
5. Use the run log to fix the first concrete failure before adding new mechanics.

### 0.2 — movement and comfort

- Add locomotion options and a comfort/turning section to the VR menu.
- Preserve the original third-person game movement as the baseline.
- Add snap/smooth turn, vignette and seated/standing calibration only after
  measured headset behavior is stable.
- Keep every optional feature fail-open so one feature cannot disable the VR core.

### 0.3 — gesture-based homing attack

- Identify the game's existing homing-target selection and attack state machine.
- Prototype controller pose/velocity gesture detection in the native bridge,
  logging candidate events without changing gameplay.
- Require a stable target, bounded hand velocity and cooldown before triggering.
- Map the gesture to the game's existing homing attack action where possible;
  do not synthesize a separate movement system until the native path is proven.
- Add an option and calibration threshold, disabled by default until tested.

### 0.4 — gesture running

- Determine whether running is an existing analog/button state or a speed scalar.
- Prototype arm-swing detection with deadzones, cadence and hysteresis.
- Prevent false activation while menus, cutscenes, damage states or airborne
  movement are active.
- Add a user toggle, sensitivity and fallback controller binding.

### 0.5 — first-person IK

- Locate the player skeleton, hand/arm bones and camera-relative animation state.
- Build a visual-only first-person hand pose first; preserve third-person gameplay
  and collision transforms.
- Add hand/controller offsets, elbow targets and comfort limits.
- Avoid replacing authored animation until the native pose and transitions are
  verified in several stages.
- Candidate 0.1.8 implements corrected procedural tracked gloves, body hiding and
  an adjustable eye anchor. Full hand/arm IK, authored hand meshes, physical world
  interaction and broader character support remain future work. Gesture running,
  homing and crouch spindash are now implemented for SonicNew and await testing.

### 0.6 — world collision and physical interaction

- Measure the game's character controller and collision queries from the APK.
- Add headset/hand collision only as a bounded VR interaction layer.
- Keep the gameplay character collision authoritative to avoid tunneling,
  camera clipping and accidental level-breaking.
- Test slopes, rails, springs, loops, moving platforms, water and death/reset.

## Engineering rules

- Verify every reverse-engineering lead against the exact supplied APK.
- Keep the original APK, extracted evidence and generated artifacts outside the
  tracked source tree or in ignored folders.
- Record each behavior and runtime result in `docs/`; optional changes remain isolated from the stereo core.
- A static build is not headset acceptance. Preserve logs from every test run.
- Optional features degrade to stock behavior if their proof or runtime hook fails.
- Never intercept the Quest right Meta/system button; it remains available for
  the system recenter action.

## Release gate

The first release can be created by promoting the tested APK and matching source
ZIP into `releases/<version>/`, adding release notes and checksums, and updating
the root README. No source-layout migration should be needed. Before that gate,
`releases/` remains a placeholder and no artifact is called a release.
