# Active work checkpoint

Date: 2026-09-27. New candidate: **0.1.15 / versionCode 115**.
Accepted baseline: **0.1.14**, source 30998c0 (full SHA in its checksums file).
User reports it works fantastically and is a good baseline.

- Latest run preserved under out/headset-logs/P06Quest-20260927-010856-078.log.txt.
  Hash, acceptance and explicit user defaults: evidence/candidate-0114-run.json.
- New defaults supplied in chat: HUD distance 2, X -0.40, Y -0.30, size 20%;
  screen distance 1, width 3; title size 15%, distance 3; left roll +30,
  right roll -30. Other defaults unchanged. Existing saved settings preserved.
- 0.1.15 adds HUD Width and Title/Menu Width (5-300%) and Hide In-Game UI.
  Only bridge-owned canvases are hidden; prior enabled state is restored.
  Title and VR menu remain accessible; opening VR settings temporarily shows HUD.
- Settings schema 9 reads 1-8; snapshots with field legend are logged at startup
  and after changes. Previous logs did not contain numeric VR preferences.
- Preserve the accepted head-follow HUD, anchored VR panel, interaction bundle,
  stereo path, title fix, authored gloves, gestures and haptics.
- Game save identity remains 0.1.10-vr-candidate. Never bump src/save_schema.h.
  Keep direct object-reference setter/read-back and arithmetic-only scalar writes.
- Three Settings.SetLocalSettings null references remain in the accepted log;
  no speculative change to that unrelated path or the unresolved water issue.
- Details: CANDIDATE-0115.md. Width/hide controls need the user's Quest test.

Artifacts: out/p06-quest-0.1.15.apk, matching source ZIP, receipt and checksums.
Preserve 0.1.14 accepted APK, original APK, sources and logs. No public release.
MCC untouched. Install over current app without clearing data.
