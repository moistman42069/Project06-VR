# Active work checkpoint

Date: 2026-09-26. Current candidate: **0.1.11 / versionCode 111**.

- User is angry about recurring crashes and saves becoming incompatible. Acknowledge
  the impact; do not promise headset guarantees from static checks.
- User confirmed title pink flicker is fixed in 0.1.10; preserve that camera path.
- Both latest runs (224650 and 224727) crash after slot selection. Logs preserved
  under out/headset-logs; hashes and frames in evidence/crash-0110-runs.json.
- Exact crash: IL2CPP liveness traversal at 0x109cb9c follows a stack address
  stored in the managed XR descriptor field. field_set_value stores a reference
  argument directly: passing &desc was wrong. Same issue existed in new camera
  bindings. Both now use field_set_value_object with direct object and read-back.
- tools/test-reference-abi.py executes the pinned ARM64 setters with Unicorn and
  reproduces the old bug, verifies all 4 reference kinds and unchanged float writes.
- Application.version is the game's save compatibility key. Freeze game-facing
  schema identity at 0.1.10-vr-candidate while APK/menu/log versions advance.
  Accept exact prior candidate labels in memory; no file rewrite on slot browsing.
  Do not bump src/save_schema.h when bumping the package version.
- Keep all previous features: authored smooth gloves, outward homing, head-relative
  steering, blue grouped VR menu, crouch, haptics, visibility and title fixes.
- Water remains unchanged: no concrete further diagnosis, per user instruction.
- Details: [CANDIDATE-0111.md](CANDIDATE-0111.md).

Artifacts: out/p06-quest-0.1.11.apk, matching source ZIP, receipt, compatibility
audit and external checksums. Preserve original, logs, source archives, receipts
and 0.1.9 playable fallback. Do not distribute 0.1.10 as a working candidate.
No public release; MCC untouched. Install over existing app without clearing data.

Pending Quest acceptance: select an intact older slot, reach level select and
stage, return to title, relaunch/reopen the slot, then check retained features.
No connected Quest through Shadow PC. Host checks are not headset acceptance.
