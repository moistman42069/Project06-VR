# Active work checkpoint

Date: 2026-09-22
Status: 0.1.0 failed on Quest 3; replacement 0.1.1 packaged, awaiting retest.

Current candidate: `out/p06-quest-0.1.1.apk` (versionCode 101).
SHA-256: `7d7e622f35c4e97bf15df1baab8b5545f872724c8b17c521a1884c524b46b7ca`.
Matching source: `out/p06-quest-0.1.1-source.zip`.
Receipt: `out/p06-quest-0.1.1-receipt.json`.

Both user logs are preserved under `baseline/0.1.0-startup-failure/`.
The failed APK and source ZIP remain under `out/` with their original 0.1.0 names.
Do not promote either candidate to a public release without headset acceptance.

See [startup crash diagnosis](STARTUP-CRASH-20260922.md) for the reproduced
package/resource mismatch, patch, checks and limits. This changes Android
resource packaging and startup diagnostics; immersive third-person rendering,
controller bindings, HUD handling and menu modes remain the existing candidate
implementation. Neither run reached their runtime verification.

Next: user sideloads over 0.1.0, launches and supplies the new Downloads/P06Quest
log. Confirm nonzero resource IDs and UnityPlayerActivity.onCreate completion,
then provider registration, XR initialization, controls, gameplay and HUD.
Use HEADSET-TEST-CHECKLIST.md for acceptance. No USB/headset access is available
through the user's Shadow PC. Planned mechanics remain in PROJECT-PLAN.md.
