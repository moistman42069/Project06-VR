# Active work checkpoint

Date: 2026-09-27. New candidate: **0.1.12 / versionCode 112**.
Headset-tested baseline: **0.1.11**, commit 252cb42c09fd226b898d3f87875c29cd933ff65b.

- User reports great progress, good hands and all other features working well.
- Current request: HUD left/right/up/down adjustment and hand orientation calibration.
- Added UI HUD distance (extended to 8), horizontal/vertical position, size and reset.
- Added independent left/right pitch, yaw and roll plus hand rotation reset under VR.
- New options default neutral and persist in VR settings schema 6; schemas 1-5 migrate.
- Render-only hand calibration preserves controller pose input for gestures and movement.
- HUD transforms apply to bridge-owned canvases during immersive gameplay; title and
  world-space gameplay canvases keep their existing ownership behavior.
- Latest log preserved in out/headset-logs/P06Quest-20260926-234747-097.log.txt;
  evidence/candidate-0111-run.json records its hash, acceptance and remaining
  Settings.SetLocalSettings null-reference records (unrelated path not changed).
- Preserve the accepted XR submission, title fix, glove geometry, locomotion,
  physical gestures, haptics and managed-reference corrections.
- **Do not bump src/save_schema.h.** Game-facing identity stays 0.1.10-vr-candidate,
  independent of APK/menu/log versions. Exact prior aliases are accepted in memory.
- All reference writes must use il2cpp_field_set_value_object with direct object;
  scalar writes use the arithmetic-only wrapper. The old address-of-reference
  setter corrupted managed fields and caused 0.1.10's liveness crash.
- Water remains unchanged: no concrete further diagnosis, per user instruction.
- Details and checks: CANDIDATE-0112.md and HEADSET-TEST-CHECKLIST.md.

Artifacts: out/p06-quest-0.1.12.apk, matching source ZIP, receipt, compatibility
report and external checksums. Preserve original APK, logs, sources, receipts
and accepted 0.1.11 fallback. No public release; MCC untouched.
Install over existing app without uninstalling or clearing data.

Pending Quest acceptance: new HUD offsets/size/depth in first and third person,
per-hand calibration, persistence/reset, scene transitions and retained gestures.
No connected Quest through Shadow PC. Host checks are not headset acceptance.
