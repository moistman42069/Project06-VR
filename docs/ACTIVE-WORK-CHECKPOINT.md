# Active work checkpoint

Date: 2026-09-27. New candidate: **0.1.14 / versionCode 114**.
Accepted fallback: **0.1.12**, source e2f8a79baee89e357ebbe32708e2cd86b997bec7.

- 0.1.14 adds UI > In-Game HUD Follows View, included in Immersive Mode (All).
  Uses the current tracked head pose before the existing first-person/camera transform.
  Applies only during immersive gameplay; title UI and anchored VR panel are unchanged.
  Defaults off on migration; enabling the bundle sets it on. Settings schema 8
  reads versions 1-7. Details: CANDIDATE-0114.md. 0.1.13 has no new headset result.
- User calls 0.1.12 a fantastic build. Latest log is preserved in out/headset-logs;
  hash/report in evidence/candidate-0112-run.json. One existing game settings
  null reference remains; no reported startup/transition failure.
- 0.1.13 lowers HUD/title size to 5%, extends HUD/screen depth and UI offsets,
  adds separate title/menu and VR panel controls, and defaults title UI to 65%.
- VR panel captures opening head pose in renderSpace; head motion does not drag it.
  It recaptures after close/open, recenter or switching follow-view off. Optional
  follow-view remains. Invalid pose suppresses only the panel until valid tracking.
- Immersive Mode (All) is first VR row: first person + physical running + homing
  + crouch spindash; enabling also selects immersive display and positional tracking.
  Disabling retains third-person VR. Individual switches/sensitivities remain.
- Default glove angles: left (-10,-5,-10), right (-10,+5,+10). These are modest
  ergonomic choices, not measured headset calibration. User had not adjusted hands.
  Preserve nonzero schema-6 custom calibration; zeroed hands adopt new defaults.
- Settings schema 7 reads older formats. **Never bump src/save_schema.h**:
  game save identity stays 0.1.10-vr-candidate independently of package version.
- Keep managed object writes on field_set_value_object, arithmetic-only scalar
  wrapper, and native-only early hook install. They prevent the old liveness crash.
- Preserve stereo submission, title pink fix, smooth authored meshes, input and
  gameplay hooks. Water remains unchanged without concrete further evidence.
- New controls and changed defaults await user Quest test. Host tests are not
  headset acceptance. Details: CANDIDATE-0113.md and HEADSET-TEST-CHECKLIST.md.

Artifacts: out/p06-quest-0.1.14.apk, matching source ZIP, receipt and checksums.
Retain tested 0.1.12 fallback, original APK, source archives and run evidence.
No public release. MCC untouched. Install over existing app; do not clear data.
