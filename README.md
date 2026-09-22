# Project 06 Quest VR

Standalone ARM64 Quest 3 VR project for Sonic Project 06. The current tree contains
an experimental, locally built candidate, **not yet tested in a headset**. Signing, alignment, payload integrity,
native compilation, input-menu tests and view-math tests are build evidence;
they do not prove startup, correct stereo, HUD appearance or playable performance.

## Project status

This is a working pre-release repository. There are no releases yet. The first
headset result will determine whether candidate 0.1.0 is promoted into
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

Sideload `out/p06-quest-0.1.0.apk` using your existing Quest sideload method.
Launch **Project 06 Quest** from Unknown Sources. Package `com.p06.quest` installs
alongside the original Android game; its saves/settings are separate.

The default mode is immersive third-person VR. The original gameplay camera
remains the third-person anchor; the native XR provider supplies tracked left and
right eye views. Sonic remains visible. No hand models or motion-gesture combat
are implemented.

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
A applies and B closes. Game time is paused while this menu is open. Closing
inputs remain captured until controls return to neutral.

Display modes: **Immersive third person**, **3D stereo screen**, **2D theatre**.
The screen modes are optional. World scale, positional tracking, screen size,
distance, stereo strength and HUD distance are adjustable and saved. Render scale
defaults to 70% and requires relaunch after changes. The runtime is asked for
72 Hz; this is not a frame-rate guarantee. Shadows and post-effects are explicitly
marked WIP and leave the game's settings in effect.

HUD/menu overlay canvases are moved onto a separate camera for stereo rendering.
Touch-control canvases are hidden without disabling their input rig. The bridge
handles the APK's active OutlineCamera when Camera.main is absent. Actual
readability, scene transitions, effects, cutscenes and stereo correctness require
headset testing; there is no claim that every original screen-space effect is VR compatible.

## Per-run logs

Each launch creates `Downloads/P06Quest/P06Quest-<date>-<time>.log` through Android
MediaStore. Send the log from the failed/problematic run, even if the game never
gets past startup. It includes device/build information, loader/hook results,
OpenXR session/frame failures, controller transitions and game-input query counts.
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
SDK/JDK archives into their expected vendor paths. Install Ninja with
`python -m pip install --target vendor/ninja ninja`, then configure using
`python tools/configure.py` and run `python tools/build-apk.py --apk <base.apk>`.
The build script creates a local development signing key if absent; retain that
key for future upgrade-compatible builds. A rebuilt package with a different key
requires uninstalling this candidate first (which removes app data).

`tools/build-menu-test.cmd` uses the locally installed VS 18 Build Tools to test
the production menu and view mathematics. Adjust its vcvars path on other hosts.
The production bindings are supplied in `src/bindings.h`; regeneration requires
the original APK's Il2CppDumper output and extracted native ELF.

Native dependencies included at these revisions:

- Khronos OpenXR-SDK: `f2448a8797c85814aa892efc1ab8707900fbcc78`.
- Dobby: `9c85e74f92eda36b99ddb0fddb17a9df2278a4d3`.
- Valve unity-xr-plugin headers: `a30a0100daacef8c3f0290ce5b179c8a1148abb0`.
- dhepper/font8x8 basic font, public domain; notice in its header.

`out/package-receipt.json` identifies the exact APK, native plugin and mod source
tree and records preserved game entries. The original APK and MCC workspace were
not modified. No headset installation or acceptance has occurred.
