# Project 06 Quest VR

Standalone ARM64 Quest 3 VR project for Sonic Project 06. **0.1.14 is a test
candidate, not a public release.** The user calls 0.1.12 a fantastic working
build; it remains the retained headset-tested fallback.

0.1.14 adds **UI > In-Game HUD Follows View**, also enabled by **Immersive Mode (All)**.
It preserves HUD size/distance/offsets and follows tracked head rotation/position
in first or third person. It defaults off when upgrading; title and VR-panel
follow behavior remain separate. [Details](docs/CANDIDATE-0114.md).

0.1.13 allows much smaller HUD/title UI, adds independent title/menu and VR-panel
controls, anchors the VR panel where it opens, adjusts default glove angles and
adds **Immersive Mode (All)** at the top of VR settings.
[Changes and validation](docs/CANDIDATE-0113.md). These changes await Quest testing.

## Project status

This is a working pre-release repository. There are no releases yet. Headset acceptance is required before a candidate is promoted into
`releases/`. Future work is tracked in [docs/PROJECT-PLAN.md](docs/PROJECT-PLAN.md),
with the current handoff in [docs/ACTIVE-WORK-CHECKPOINT.md](docs/ACTIVE-WORK-CHECKPOINT.md).
The repository is intentionally arranged so a tested candidate can become a
release without moving the source, evidence or build scripts.

The public repository contains source, documentation and small reviewable APK
evidence. The original game, extracted Unity payload, SDK/toolchain binaries,
development key, logs and the 1.76 GB candidate APK stay local and ignored. The
`out/` path below refers to a local build produced from this tree; it is not a
GitHub download link until a tested release is deliberately published.

### Base build provenance

This project uses the newest publicly documented Android build located during
research: `p-06RELEASE64.apk`, uploaded September 10, 2025 and associated with
Lowfriend's Sonic Release Android port. The exact source listing is the
[MediaFire APK page](https://www.mediafire.com/file/tgiales231mf4v0/p-06RELEASE64.apk/file),
with the release context in the [Lowfriend video](https://www.youtube.com/watch?v=crjt6Sth97Q).
No later public APK, patch or version tag was found as of September 22, 2026;
a private or unindexed Discord test build could still exist.

## Install and play

Sideload `out/p06-quest-0.1.14.apk` using your existing Quest sideload method.
This replacement uses the same signing key and a higher version code; install it
over the previous candidate to retain settings. See [latest XR startup investigation](docs/EGL-SESSION-STARTUP-20260926.md).
Do not uninstall or clear app data to update. Save compatibility is now tied to
the unchanged game schema, not the candidate version; intact 0.1.0–0.1.10 slots
are accepted without resetting progress. Deleted/overwritten slots cannot be
recovered by this fix. Unknown save versions remain rejected.
Launch **Project 06 Quest** from Unknown Sources. Package `com.p06.quest` installs
alongside the original Android game; its saves/settings are separate.

The default mode is immersive third-person VR. The original gameplay camera
remains the third-person anchor by default; the native XR provider supplies
tracked left and right eye views. Experimental first person uses a player-relative
eye-height anchor, with stable yaw, head tracking, right-stick turning and body
hiding. Smooth white Sonic gloves use each controller's grip position and aim orientation independently. Body hiding includes separate
upgrade attachments; no floating replacement feet are added.
Eye height is adjustable; losing one controller hides that glove and preserves
the camera. The glove geometry comes from this APK's Sonic mesh, with one
subdivision pass and smooth normals; there is no finger animation or arm IK yet.
Missing required player/render APIs leave third person active.

| Quest input | Game action |
| --- | --- |
| Left stick | Original left stick / movement / menu navigation |
| Right stick | Original right stick / camera |
| A, B, X, Y | Corresponding original gamepad buttons |
| Left/right grip | Left/right bumper |
| Left/right trigger | Left/right trigger |
| Left controller menu button | Start / game pause |
| Left stick click | Back (150 ms chord guard; quick taps delivered on release) |
| Hold right stick click and move right stick | D-pad |
| Both stick clicks together | Toggle VR menu |
| Hold right Meta button | Quest system recenter; reserved button is not intercepted |

In the VR menu, use left stick up/down to select and left/right to change;
the root screen lists **UI**, **VR**, **GRAPHICS**, **HAPTICS**, and **SYSTEM**.
Running, homing and crouch controls are inside **VR**; scroll down to reach them.
Scroll with the stick, A opens a
category/applies a setting, X returns to the category list and B closes. Game time is paused while this menu is open. Closing
inputs remain captured until controls return to neutral.

Display modes: **Immersive third person**, **3D stereo screen**, **2D theatre**.
The screen modes are optional. World scale, positional tracking, screen size,
distance, stereo strength and HUD placement are adjustable and saved. Under **UI**,
HUD distance (0.25-20), left/right and down/up offsets (-10 to +10), size (5-200%)
and reset are available. Title/main-menu UI has separate size, distance and
position controls, defaulting to 65% size at distance 3. These controls resize
the UI canvas, not the background 3D scene. VR-panel size/distance and optional
follow-view behavior are also under UI. Negative positions are supported;
size/depth stay positive to avoid mirroring or placing a panel behind you.

Under **VR**, **Immersive Mode (All)** enables first person, positional tracking,
physical running, outward homing and crouch spindash together. Turning it off
disables the four optional interactions while retaining third-person VR.
Individual switches and sensitivities remain available; the bundle reads OFF
when not all its settings are enabled. Scroll for independent left/right hand
pitch, yaw and roll (-180 to +180 degrees, 5-degree steps) and reset. Defaults
are left (-10, -5, -10) and right (-10, +5, +10). Existing nonzero calibration
is preserved; untouched hands adopt the new defaults. Calibration affects only
rendered gloves, preserving gesture/controller input.

The VR panel stays at the captured opening pose while you look around. Close
and reopen to place it in front of you again. Recenter also places an open panel
in front of the new origin. **VR Panel Follows View** restores head-following.
Render scale
defaults to 70% and requires relaunch after changes. The runtime is asked for
72 Hz; this is not a frame-rate guarantee. Shadows and post-effects are explicitly
marked WIP and leave the game's settings in effect.

HUD/menu overlay canvases are moved onto a separate camera for stereo rendering.
The bridge now redirects runtime requests for Screen Space Overlay immediately
after the stereo HUD camera is available, with the periodic conversion retained
as a fallback. The user confirmed level select displays correctly in 0.1.9 and
title-screen pink flicker is fixed in 0.1.10. The 0.1.11 managed-reference
correction resolved the reported transition failure in the latest user test.
Touch-control canvases are hidden without disabling their input rig. The bridge
handles the APK's active OutlineCamera when Camera.main is absent. Actual
readability, scene transitions, effects, cutscenes and stereo correctness require
headset testing; there is no claim that every original screen-space effect is VR compatible.

## Experimental controls and haptics

The three gesture toggles default off. Shared ground running supports the
standard character motors; homing is enabled for Sonic, Shadow, Metal Sonic and
Princess/Sonic-with-Elise, and crouch spindash for Sonic, Shadow and Metal Sonic.
Mach-speed and snowboard controls retain their original directional semantics.
Code presence does not establish which characters this port exposes in menus. Normal
Touch controls remain available. Gestures are suspended in menus, unsupported
states, screen modes, tracking loss and after a reference-space change.

- **Running:** alternating swings from both hands drive head-directed movement.
  Swing sensitivity and speed ramp are adjustable. Acceleration remains native; only the requested speed cap changes during
  gesture-owned ground running; native
  physics, slopes and maximum speed remain authoritative. Left-stick input wins.
- **Homing:** swing either hand outward while airborne with an eligible native
  target. Adjustable travel distance (default 10 cm), velocity gates and cooldown
  reject accidental gestures. No pointing dwell or inward pull is required.
  It does not select a different enemy or bypass the
  game's homing rules. No target means no gesture attack.
- **Spindash:** stand normally when enabling/calibrating, crouch past the chosen
  depth to hold native charge, then stand to release toward your head direction.
  Focus/menu/tracking loss cancels an owned charge through a native state change.
- **Haptics:** enabled by default at 70%, with toggle/intensity controls. Feedback
  covers game state/action transitions, damage/death, jump/landing, attacks,
  rings, movement, grinding/sliding, charge and release. Continuous effects are
  bounded; focus/menu loss stops output. Character-specific timing and intensity
  still need headset verification; no claim of exhaustive action coverage is made.

The water recovery only supplements complete crossings of live WaterSlider or
WaterslideBooster sphere/box triggers that native overlap detection missed. It
does not fabricate grounding or extend a water slide indefinitely. Logs capture
entries, booster speeds and exits. Whether it fixes the exact filmed fall is
not established by the existing logs/video. The user reports it still happens;
0.1.10 leaves water behavior unchanged because no concrete further fix was found.

## Per-run logs

Each launch creates `Downloads/P06Quest/P06Quest-<date>-<time>.log` through Android
MediaStore. Send the log from the failed/problematic run, even if the game never
gets past startup. It includes device/build information, loader/hook results,
OpenXR session/frame failures, controller transitions and game-input query counts.
Before Unity starts, the app queries Android's saved exit information for its
own package (up to three prior exits). Available trace bytes are embedded as
base64 in the log, including native tombstone protobufs on API 31+. Capture is
bounded and may be unavailable. This allows diagnosis without USB debugging.
Startup Java exceptions are written directly to the run log before rethrowing;
resource lookup IDs and activity startup completion are recorded.
Unity/app error output is collected where Android permits it. A minimal native
crash recorder includes signal and register addresses; it is not a full crash dump.

If Downloads creation fails, the fallback is
`Android/data/com.p06.quest/files/P06Quest-<date>-<time>.log`.
Settings live in that app files directory as `vr-settings.txt`. Logs are local,
not uploaded automatically. Abrupt process/OS termination may truncate the log.

For the first test, check title/menu input, one gameplay stage, head rotation and
translation, HUD visibility, both-stick menu, all three modes and system recenter.
Report the first failure and share that run's log. A successful build cannot
replace this check on your Quest.

## Source and reproduction

The supplied APK is pinned to SHA-256
`3801cfc73cf99157e779d0bc9b1e76e54ba5644dbf9d73303c93f38bc6d305dc`.
It contains Unity 2022.3.62f1, IL2CPP metadata 31. The reference repository
<https://github.com/R3verseG0d/Project06OSP> is an older desktop reconstruction.
Its Rewired input is not the adapter target: native callers in this Android APK
use ControlFreak2.CF2Input. All patched method prologues are verified at runtime.

The source ZIP includes the mod, tests, packaging tools, selected evidence and
native dependency sources/licenses. It excludes the original game, extracted
assets/assemblies, SDK binaries and signing key. Supply your own original APK.

Windows prerequisites: Python 3.11+, CMake 3.22+, Ninja, NDK r27c, Android
build-tools 35, platform 35, JDK 17. `tools/download-toolchain.py` downloads the
SDK/JDK archives into their expected vendor paths. Install UnityPy and NumPy with
`python -m pip install --target vendor/unitypy UnityPy numpy`; the native build
extracts only the pinned glove mesh locally into ignored `out/generated/`.
Extracted game geometry is excluded from Git and the source ZIP. Install Ninja with
`python -m pip install --target vendor/ninja ninja`, then configure using
`python tools/configure.py` and run `python tools/build-apk.py --apk <base.apk>`.
The build script creates a local development signing key if absent; retain that
key for future upgrade-compatible builds. A rebuilt package with a different key
requires uninstalling this candidate first (which removes app data).

`tools/build-menu-test.cmd` uses the locally installed VS 18 Build Tools to test
the production menu and view mathematics. Adjust its vcvars path on other hosts.
The production bindings are supplied in `src/bindings.h`; regeneration requires
the original APK's Il2CppDumper output and extracted native ELF.

`tools/test-reference-abi.py` executes the pinned ARM64 field setters on the host
to catch managed-reference ABI mistakes. It requires the extracted original ELF
and `python -m pip install --target vendor/python unicorn==2.1.4 pyelftools`.
Keep `src/save_schema.h` fixed when updating APK/menu/log versions: that identity
describes unchanged save data, not the mod's release number.

Native dependencies included at these revisions:

- Khronos OpenXR-SDK: `f2448a8797c85814aa892efc1ab8707900fbcc78`.
- Dobby: `9c85e74f92eda36b99ddb0fddb17a9df2278a4d3`.
- Valve unity-xr-plugin headers: `a30a0100daacef8c3f0290ce5b179c8a1148abb0`.
- dhepper/font8x8 basic font, public domain; notice in its header.

`out/p06-quest-0.1.14-receipt.json` identifies the exact APK, native plugin, generated glove geometry and mod source
tree and records preserved game entries. The original APK and MCC workspace were
not modified. 0.1.0 crashed on a resource lookup; 0.1.1 fixed that crash but did not enter the app; 0.1.2 ran Unity while the headset loading screen remained; 0.1.3 registered XR but failed before creating an OpenXR session. The user confirmed 0.1.4 reaches immersive VR and the settings panel. 0.1.8 startup/haptics, 0.1.9 first-person view/hand orientation/crouch/menu and 0.1.10 title rendering are confirmed. 0.1.10 then crashed after slot selection; the user reports 0.1.11 works well. The user reports 0.1.12 works well; 0.1.13 UI/panel/default refinements await headset verification.
