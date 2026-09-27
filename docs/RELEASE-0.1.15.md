# First public release: 0.1.15

Published as the first public Quest APK release after the user tested the exact
0.1.15 build on Quest 3 and said it was perfect as a first release. This remains
an early work-in-progress VR project; acceptance does not claim every level,
character, effect or headset configuration has been exhaustively verified.

## Artifact identity

- APK: `p06-quest-0.1.15.apk` (1,762,045,083 bytes)
- APK SHA-256: `36d814590be4463c14115ee4049f0425d967ec6605c33da12e0fc12abfa2de4c`
- Matching source archive: `p06-quest-0.1.15-source.zip`
- Source archive SHA-256: `6f2f48d1fe86be249920567412ac2fda5ae25e18b6738215a332da84b85184e8`
- Build source commit: `82b4e909e35f7c9d6d98c2750a7c200c851ca49b`
- Pinned original APK SHA-256: `3801cfc73cf99157e779d0bc9b1e76e54ba5644dbf9d73303c93f38bc6d305dc`
- Published SHA-256 list: [releases/SHA256SUMS-0.1.15.txt](../releases/SHA256SUMS-0.1.15.txt)

The APK is based on the ARM64 Android **Sonic Release** port announced in
Lowfriend's [SONIC P-06 ANDROID PORT. SONIC RELEASE trailer](https://www.youtube.com/watch?v=crjt6Sth97Q),
uploaded September 10, 2025. It identifies as Unity 2022.3.62f1 with IL2CPP
metadata version 31. As of September 27, 2026, no newer publicly documented
Android APK or patch is known. A private or unindexed build may exist. Updating
the VR work to a newer Android port and incorporating its latest changes is a
future goal if a newer build becomes available and can be verified.

## Included

The release adds Quest OpenXR stereo rendering, Touch input, adjustable UI,
the categorized in-game VR menu, optional first-person view and Sonic gloves,
physical running, outward-swing homing, crouch spindash, haptics, persistent VR
settings and per-run logs. The README documents controls, categories, settings,
installation and the known limits.

## Using VR movement and gestures

In a level, click both controller sticks together to open the VR menu. Use the
left stick to move through the list, **A** to enter/change a setting, **X** to
return to the category list and **B** to close. Open **VR** and turn on
**Immersive Mode (All)** to enable immersive VR, first person, physical running,
outward-swing homing, crouch spindash and HUD follow together. To keep third
person, leave that bundle off and toggle the individual gesture options instead.
Gesture options are in the **VR** category and their values save automatically.

- **Run:** turn on **PHYSICAL RUNNING**. Swing both tracked hands back and forth
  in an alternating running rhythm, with one hand moving forward as the other
  moves back. Swing clearly and keep the alternating rhythm going. Move the left
  stick to take manual control; the hand gesture yields to it. Adjust **RUN
  SWING SENSITIVITY** if swings need to register more easily, and **RUN SPEED
  RAMP** to change how quickly the motion builds speed. The gesture is for normal
  free movement; special rail, vehicle and board controls remain native.
- **Homing attack:** turn on **OUTWARD SWING HOMING**. In an eligible airborne
  homing state, let the game select a homing target, then make a quick outward
  arm swing, extending either hand away from your body. No pull-back is required.
  The game still decides whether a target is valid and performs its native
  attack, so a swing with no selected/eligible target does nothing. Adjust
  **HOMING SWING DISTANCE** to change the required extension. Supported character
  types in this build are Sonic, Shadow, Metal Sonic and Princess.
- **Crouch spindash:** turn on **CROUCH SPINDASH**. Stand upright and select
  **CALIBRATE STANDING** in the VR menu. While grounded, crouch until your head
  drops beyond **CROUCH DEPTH** and hold briefly to begin charging; stand back up
  to launch once in the direction you are facing. Adjust **CROUCH DEPTH** if it
  triggers too easily or requires too deep a crouch. Sonic, Shadow and Metal
  Sonic support this gesture in the current build. Opening a menu or losing
  tracking cancels an owned charge rather than firing it.
- **Haptics:** in **HAPTICS**, leave **GAME HAPTICS** on and set **HAPTIC
  STRENGTH** to taste. Feedback covers supported game actions and movement.

If gestures do not engage, check that the matching toggle is on, the headset and
both hands are tracked, the game is focused and unpaused, and the current
character/state supports that move. Left-stick movement intentionally overrides
physical running.

Install the APK over an existing P06 Quest install to preserve settings and
saves. Do not uninstall or clear app data when updating. The package ID is
`com.p06.quest`.

## Known port-level issue

Some level glitches appear connected to the underlying Android port. The
observed water section can fail to register Sonic running/sliding on the surface
and drop him into the water. This release does not claim a concrete fix for that
behavior. Until the port or VR compatibility is refined, an alternate route may
be needed to pass affected levels. Other level-specific behavior may also need
investigation; report the stage, route and per-run log when practical.

Other known limits include incomplete compatibility for original screen-space
effects, shadows and post-effects, and three `Settings.SetLocalSettings`
null-reference log entries that did not stop the accepted run.

