# Active work checkpoint

Date: 2026-09-26. Current candidate: **0.1.10 / versionCode 110**.

- User accepted 0.1.9 crouch spindash, glove orientation, first-person head-follow
  view and blue menu; startup, immersive VR and haptics also work.
- Latest log 211947 is preserved in ignored out/headset-logs. Hash and findings
  are in evidence/candidate-0110-run.json. 0.1.9 remains the tested fallback.
- 0.1.10 applies the title-specific Forward/LDR/no-forced-RT camera correction on
  every title entry; MainMenu's accepted path is preserved.
- Native-consumer head steering covers RotatePlayer/SlopePhysics, including air.
- Either-hand outward homing replaces point/pull, with native eligibility gates.
- Smooth authored Sonic gloves replace primitives; no feet or arm IK. Geometry
  is derived locally from the pinned APK and excluded from source distribution.
- World cameras bypass baked occlusion; per-eye frustums remain. Local gauge and
  skybox camera bindings address native Camera.main null dereferences.
- Physical controls and sliders are inside the scrollable VR category; X backs out.
- No further concrete water diagnosis: water code remains unchanged, as requested.
- Details/references/limits: [CANDIDATE-0110.md](CANDIDATE-0110.md).

Artifacts: out/p06-quest-0.1.10.apk, source ZIP, receipt, compatibility audit and
external checksums. Preserve original APK, logs, source archives, receipts and
0.1.9 fallback. MCC remains untouched. No public release before user testing.

Pending headset verification: title fresh/return transitions, outward homing,
airborne heading, authored gloves, visibility/performance and baseline regressions.
Static checks cannot establish headset acceptance. No connected Quest on Shadow PC.
