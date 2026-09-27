# Active work checkpoint

Date: 2026-09-26. Current candidate: **0.1.9 / versionCode 109**.

- User confirmed 0.1.8 launch, immersive VR and good haptics. Preserve its managed
  startup guard, EGL/OpenXR submission and haptic implementation.
- Latest log 203905 and 06bug.webp are preserved under ignored out/headset-logs.
- 0.1.9 corrects first-person steering basis, swing-speed envelope/acceleration,
  homing/crouch handling, aim-oriented gloves and accessory/singular-renderer hide.
- Blue root-category menu: stick scroll, A enter/apply, X back, B close.
- Menu-only forward/mono-preview/static-background compatibility path addresses
  right-eye flicker. The exact failing GPU draw is unproven; do not claim a
  guaranteed or headset-accepted fix.
- Latest water run has successful entry, boosters and normal spline-end exit.
  Preserve swept triggers/native exits; ground-grace remains disabled.
- Details, primary references and limits: [CANDIDATE-019.md](CANDIDATE-019.md).

Artifacts: out/p06-quest-0.1.9.apk, matching source ZIP, receipt and external
checksums. The archive must not embed a stale checksum of itself. Preserve
original APK, logs, source archives and receipts. Superseded generated APKs may
be removed only after successful replacement packaging. MCC remains untouched.

Pending: user Quest verification of movement, gestures, body/gloves, menu eyes,
character transitions, water route and general gameplay. No headset connection
through Shadow PC. No public release before user testing.
