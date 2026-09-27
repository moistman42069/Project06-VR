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

