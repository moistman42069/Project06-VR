# Active work checkpoint

Date: 2026-09-26. Current candidate: **0.1.8 / versionCode 108**.

- User confirmed 0.1.4 immersive VR, controls and VR menu. This is the working
  headset baseline; later features are not accepted yet.
- 0.1.7 failed startup on all six supplied runs. The same IL2CPP null-class
  dereference occurred before the verified input hooks were installed.
- 0.1.8 removes all managed reflection from Android startup and guards runtime
  lookup behind managed Unity callbacks. Optional hook addresses are generated
  from pinned APK metadata and validated against 16 native prologue bytes.
- Categorized menus, first-person body/glove/camera fixes, three opt-in gestures,
  adjustable haptics, bounded water-trigger recovery and AnimatedUV row guards
  are implemented. Details and known limits: [CANDIDATE-018.md](CANDIDATE-018.md).
- Source remains separate from MCC. No release, device installation or headset
  acceptance is authorized by a successful build. User sideloads through Shadow PC.

Artifacts: `out/p06-quest-0.1.8.apk`, matching `-source.zip`, and `-receipt.json`.
The receipt carries the APK/source-tree/native hashes and preserves the pinned
base identity. The final source-archive checksum is in a separate checksums file,
so the archive never contains a stale copy of its own checksum.

Still pending: Quest startup verification, right-eye menu flicker, first-person
HUD/hand proportions and controller orientation, gesture tuning, haptic timing,
repeat of the recorded water route, scene/death transitions and performance.
No exact cause of the recorded water fall is claimed; missed trigger recovery is
bounded and observable, and native low-speed/spline-end exits remain intact.
Preserve logs and source archives. No public GitHub release until user testing.
