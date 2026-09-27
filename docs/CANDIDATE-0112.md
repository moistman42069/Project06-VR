# Candidate 0.1.12: HUD and glove calibration

The user reports 0.1.11 works well and requests visual calibration. Its log,
P06Quest-20260926-234747-097.log.txt, is preserved locally; the hash and findings
are in evidence/candidate-0111-run.json. The log shows valid XR descriptor
read-back, older-save acceptance and both glove meshes created. Two existing
Settings.SetLocalSettings null-reference records remain; this change does not
modify that unrelated game settings path.

## Controls

- **UI:** HUD distance 0.75-8, horizontal/vertical offsets -2 to +2 in 0.05
  steps, size 50-150% in 5% steps, and reset. Positive offsets mean right/up.
- **VR:** independent pitch/tilt, yaw/angle and roll/twist for each hand,
  -180 to +180 degrees in 5-degree steps, plus reset for both hands.
- Left stick selects/changes, A applies, X returns to the category list.
  Both stick clicks still toggle the menu. Scroll to reach lower rows.
- Adjustments persist. VR settings schema 6 reads schemas 1-5; new defaults
  are centered HUD, 100% size and zero added hand rotation. Game save schema
  stays at the fixed 0.1.10-vr-candidate identity used by 0.1.11.

## Implementation boundaries

HUD offsets operate on bridge-owned canvases during immersive gameplay. Existing
world-space gameplay UI is not captured. First person retains its accepted
player-relative anchor; third person uses the dedicated HUD camera when offsets
or size are customized. Returning to ordinary camera-space UI restores the saved
scale. HUD size is independent of depth; depth scaling retains the previous
angular-size behavior. Reset restores centered 100% HUD at distance 2.

Unity's camera-space Canvas automatically fits the camera frustum, so transforming
it directly is not sufficient for free placement. The existing first-person
world-space conversion provides explicit position/scale control, consistent with
the [Unity 2022.3 Canvas documentation](https://docs.unity3d.com/2022.3/Documentation/Manual/UICanvas.html).

Hand calibration composes a local yaw/pitch/roll quaternion after controller aim
rotation and before the accepted model alignment. Grip position, smooth authored
mesh, gesture input poses, movement direction and haptics remain unchanged.
Zero adjustment produces identity. No new engine binding or native hook is added.
XR frame submission, title rendering and managed-reference/save fixes are retained.
No additional speculative water/level changes are included.

## Validation and acceptance

Native ARM64 build, host menu/view/gesture tests, API audit and actual pinned
ARM64 reference-setter emulation pass. New tests cover neutral and rotated hand
quaternions, settings migration from the accepted schema 5, independent menu
controls, persistence and reset without changing gesture preferences. Both new
menu pages have been rendered and inspected for legibility.

Package checks verify signing, alignment, ARM64 libraries, plugin exports and
preservation of the original game payload. Matching source and receipt accompany
the APK. 0.1.11 remains the tested fallback; 0.1.12 needs Quest verification of
placement, calibration and scene transitions. There is no public release.
