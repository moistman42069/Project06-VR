# Project 06 VR

> **Work in progress. First public release: 0.1.15.** The user has tested this
> build on Quest 3 and considers it a good first release. Further fixes and
> compatibility work remain planned. (JOIN FLAT2VR DISCORD FOR COMMUNICATION AND OTHER GREAT VR MODS/DETAILS. https://discord.gg/flat2vr)

A standalone Android/ARM64 VR mod for **Sonic Project 06** on Meta Quest 3. The
project uses OpenXR for stereo rendering and maps Quest Touch controllers to the
game. Immersive third-person VR is the default, with an experimental first-person
view and optional physical controls.

Download the single `.apk` asset from the [0.1.15 GitHub release](https://github.com/moistman42069/Project06-VR/releases/tag/v0.1.15)
and sideload it to Quest. The release also includes the matching source archive
and verification files. The APK is about 1.76 GB; it is attached to the release
and is not stored in Git history.

## Android game this is based on

The pinned input is **`p-06RELEASE64.apk`**, a 64-bit Android port of Sonic
Project 06, built with **Unity 2022.3.62f1** and IL2CPP metadata version 31. It
is the Android Sonic Release announced in Lowfriend's YouTube trailer,
[SONIC P-06 ANDROID PORT. SONIC RELEASE](https://www.youtube.com/watch?v=crjt6Sth97Q),
and was uploaded on **September 10, 2025**. It is not a conversion of a newer PC
build. As of **September 27, 2026**, no later publicly documented Android APK,
patch or version tag is known; a private or unindexed build may exist. Future
work hopes to bring this mod forward to a newer Android port/build if one becomes
available and its compatibility can be verified.

- [Original Android APK listing](https://www.mediafire.com/file/tgiales231mf4v0/p-06RELEASE64.apk/file)
- [Lowfriend's Android Sonic Release trailer](https://www.youtube.com/watch?v=crjt6Sth97Q)
- Original APK SHA-256: `3801cfc73cf99157e779d0bc9b1e76e54ba5644dbf9d73303c93f38bc6d305dc`
- The older [Project06OSP PC reconstruction](https://github.com/R3verseG0d/Project06OSP)
  is a research reference, not this Android build's code or input adapter.

## Current state

The user confirmed **0.1.14** as a good baseline and then tested **0.1.15** on
Quest 3, calling it perfect for a first release. Version 0.1.15 adds width and
hide controls, settings logs, and safer settings-file updates. See the
[0.1.15 change record](docs/CANDIDATE-0115.md), [release record](docs/RELEASE-0.1.15.md)
and [Quest test checklist](docs/HEADSET-TEST-CHECKLIST.md).

Implemented features include stereoscopic third-person VR, head translation and
rotation, Quest Touch input, a floating VR settings panel, a configurable HUD,
optional first-person view with Sonic gloves, hand-swing running, outward-swing
homing, crouch spindash and game-action haptics. Feature availability depends on
the game state and character; optional interactions default off unless enabled
in the VR menu.

## Quest controls

| Touch input | In-game action |
| --- | --- |
| Left stick | Original movement; menu navigation while VR menu is open |
| Right stick | Original camera; D-pad while holding right stick click |
| A, B, X, Y | Original gamepad buttons |
| Left/right grip | Left/right bumper |
| Left/right trigger | Left/right trigger |
| Left controller menu button | Start / pause |
| Left stick click | Back; quick taps are delivered after the chord guard |
| Click both sticks together | Open/close the VR settings panel |
| Hold right Meta button | Quest system recenter; the button is not intercepted |

When the VR panel is open, use the left stick to scroll/select, left/right to
change a value, **A** to open/apply, **X** to return to the category list and
**B** to close. The categories are **UI**, **VR**, **GRAPHICS**, **HAPTICS** and **SYSTEM**. Game time pauses while this panel is open. The panel floats at the
pose where it opened; close and reopen to place it again. Enable **VR Panel
Follows View** to attach it to head movement.

### VR category

**Immersive Mode (All)** is the first option. Enabling it selects immersive VR,
first-person view, positional head tracking, physical running, outward-swing
homing, crouch spindash and HUD follow-view. Disabling it turns those optional
interactions off and returns to third-person immersive VR. Individual options
remain available if you want to adjust the bundle.

- **First person:** hides Sonic's body and moves the view to a player-relative
  head anchor. Smooth white Sonic gloves follow the Touch controllers. Eye height
  and each hand's pitch, yaw and roll can be adjusted. Hand adjustments change
  only the rendered glove; gesture tracking retains the original controller pose.
- **Physical running:** with **PHYSICAL RUNNING** enabled, swing both hands
  forward and back in an alternating running rhythm. Both hands must be tracked;
  move the left stick to take manual control. **RUN SWING SENSITIVITY** and
  **RUN SPEED RAMP** tune the response.
- **Outward-swing homing:** while airborne, get a native homing target selected
  by the game, then quickly swing either arm outward away from your body. No
  inward pull is needed. **HOMING SWING DISTANCE** adjusts the extension needed;
  no eligible target means no gesture attack.
- **Crouch spindash:** stand upright and select **CALIBRATE STANDING**, then
  crouch until your head passes the **CROUCH DEPTH** threshold while Sonic is
  grounded. Hold briefly to charge, then stand upright to release toward your
  head direction. Menu/focus/tracking loss cancels an owned charge safely.
- **Haptics:** enabled by default. Toggle and strength controls cover game action
  and movement feedback; focus or menu loss stops continuous output.

Gesture actions require their toggles and a supported game state/character.
Ordinary Touch controls continue to work. See the source notes and test checklist
for character and failure guards.

### UI category and user-selected defaults

The defaults below are the values the user asked to carry forward. Existing
settings files take precedence on updates, so upgrading preserves personal
choices. Reset controls restore these compiled defaults.

| Setting | Default |
| --- | ---: |
| HUD distance | 2.00 m |
| HUD left/right, down/up | -0.40 m, -0.30 m |
| HUD size, width | 20%, 100% |
| Screen distance, width | 1.00 m, 3.00 m |
| Title/menu size, distance, width | 15%, 3.00 m, 100% |
| Left/right glove roll | +30?, -30? |

HUD and title/menu size range from 5% to 200%; width is an independent 5%-300%
horizontal adjustment. Position offsets range from -10 m to +10 m. UI size and
distance remain positive. **Hide In-Game UI** hides bridge-managed gameplay
canvases; the title/menu and VR settings panel remain accessible. Opening the VR
panel temporarily restores the HUD so it can be adjusted.

The **In-Game HUD Follows View** toggle is included in Immersive Mode (All). It
rotates and positions the gameplay HUD with head movement while retaining HUD
size, distance and offsets. It defaults off unless enabled with the immersive
bundle. It is independent of the VR panel's **Follows View** setting.

Optional display modes are **3D stereo screen** and **2D theatre**. Other UI
controls adjust stereo depth, world scale, HUD placement and screen size/distance.
Graphics settings that do not have a safe implementation are marked WIP and leave
the game's settings in control.

## Installation and saves

Download **`p06-quest-0.1.15.apk`** from the
[0.1.15 release](https://github.com/moistman42069/Project06-VR/releases/tag/v0.1.15),
sideload it to Quest, then launch **Project 06 Quest** from Unknown Sources. No
PC launcher or separate mod installer is needed. The package ID is
`com.p06.quest` and it installs beside the original Android game.

For local development, follow the build instructions below. The APK generated
by `tools/build-apk.py` is an unsigned-source project output signed with the
local development key. **Keep that key** for upgrade-compatible builds.

Install updates over the existing app; do not uninstall it or clear its app data.
Candidate APK versions do not change the game's fixed save-schema identity.
Intact saves from prior P06 Quest candidates are accepted in memory without
rewriting slot files. Unknown save versions are rejected. The Quest settings file
is kept separately at `Android/data/com.p06.quest/files/vr-settings.txt`.
Settings use schema migration, atomic replacement and a backup. Future settings
snapshots are written to each run log. Uninstalling/clearing app data removes
local settings and logs; it is not part of a normal update.

## Logs and testing

Each launch writes a per-run log to `Downloads/P06Quest` (with an app-private
fallback). Logs contain build/device details, startup and OpenXR status, controller
transitions, crash diagnostics where Android provides them, and the current VR
settings snapshot. Settings snapshots were added in 0.1.15; earlier logs do not
show the numeric settings selected in their session. Logs remain on-device and
are not uploaded automatically.

The user has tested 0.1.15 on Quest 3 and accepted it as the first release. That
does not establish compatibility with every level, character, scene, or headset
configuration. If a problem occurs, share the log from that run and identify the
first place behavior differs. The test checklist remains useful for future
refinement and regression checks.

Known issues and limits:

- The latest accepted log contains three `Settings.SetLocalSettings`
  `NullReferenceException` records; the game continues and this path has not been
  changed in 0.1.15.
- Some level glitches appear connected to the Android port itself. In particular,
  a water section may fail to register Sonic on the surface. This is not
  conclusively diagnosed or fixed in this release; use an alternate route through
  affected stages where possible until the port or mod is refined.
- Shadows and post-effects are WIP. Full visual compatibility for every original
  screen-space effect, level, character and cutscene is not claimed.

## Build from source

The repository deliberately excludes the original game, extracted Unity payload,
Android toolchain binaries and signing key. Use a legally obtained copy of the
pinned APK and retain your own build key.

On Windows, the documented toolchain is Python 3.11+, CMake 3.22+, Ninja, Android
NDK r27c, Android build-tools 35, platform 35 and JDK 17. Download the pinned
SDK/JDK files with `python tools/download-toolchain.py`, install build dependencies
with `python -m pip install --target vendor/unitypy UnityPy numpy` and
`python -m pip install --target vendor/ninja ninja`, then run
`python tools/configure.py` and `python tools/build-apk.py --apk <base.apk>`.
The local package script verifies the pinned original, preserves its game payload
and writes APK/source receipts under ignored `out/`. The source ZIP contains the
code, tests, selected evidence and dependency sources/licenses; it does not contain
the APK, original game or signing key.

## Project layout

- `src/` ? Android OpenXR provider, game bridge, options, input and save compatibility.
- `tests/` ? host-side menu, settings migration, view math and gesture checks.
- `docs/` ? candidate records, reverse-engineering evidence and headset checklist.
- `evidence/` ? pinned APK identity, binding and candidate evidence.
- `tools/` ? build, package, audit and source-archive scripts.
- `out/` ? local-only builds, receipts and run logs; ignored by Git.

This repository remains a work in progress. Version 0.1.15 is the first public
Quest-tested APK release; later fixes, broader compatibility checks and possible
support for a newer Android port remain future work.
