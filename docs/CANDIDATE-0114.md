# Candidate 0.1.14: in-game HUD follow-view

Adds **UI > In-Game HUD Follows View** at the end of the UI category. The option
is included in Immersive Mode (All), both its on/off action and derived indicator.
Old settings migrate with HUD follow off; explicitly enable it or the bundle.
Settings schema 8 accepts schemas 1-7. Game save identity remains unchanged.

The HUD offset is rotated by the current Unity-handed tracked head quaternion,
then translated by tracked head position/world scale when positional tracking
is enabled. The existing first-person anchor and game-camera transforms are
applied afterward, matching the eye-pose transform order. Head orientation also
rotates the canvas. Size, distance and horizontal/vertical calibration remain.
When tracking is unavailable, the HUD uses its existing camera-relative path.

The behavior is limited to immersive gameplay, both first and third person.
Front-end canvases and the independently anchored VR panel keep their previous
behavior. No new engine hook or ABI binding; gesture inputs, eye submission,
hand calibration and title-rendering fixes are unchanged.

Native build, menu/gesture/view regression tests and ABI audit pass. New checks
cover head-relative HUD math, translation disabled under rotation-only tracking,
standalone toggle persistence and bundle enable/disable. Final signed APK and
source archive are verified against the receipt and original game payload.
Quest verification remains pending. Preserve the tested 0.1.12 fallback and
0.1.13 predecessor; no public release.
