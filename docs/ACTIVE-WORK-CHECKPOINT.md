# Active work checkpoint

Date: 2026-09-26
Status: candidate 0.1.2 runs Unity but never registers XR; candidate 0.1.3 is
packaged with the SubsystemManager descriptor initialization correction and
awaits the user's Quest 3 retest.

APK: `out/p06-quest-0.1.3.apk` (versionCode 103).
SHA-256: `76b2477e7585fb34d76f3d048045d45aeb8d0211ad0d8bc964de4e9fd75d4fe3`.
Matching source ZIP: `out/p06-quest-0.1.3-source.zip`.
Receipt: `out/p06-quest-0.1.3-receipt.json`.

Read [XR descriptor startup notes](XR-DESCRIPTOR-BOOTSTRAP-20260926.md). The
last Quest log and prior artifacts are preserved under baseline/out. Static
verification passes. Headset acceptance has not been recorded; no public release.

Install 0.1.3 over 0.1.2. In its log check SubsystemManager map initialization,
nonzero integrated descriptor count, provider registration, `XR graphics
initialized`, `XR display running=1`, and whether the Quest loading view clears.
If anything fails, share the matching Downloads/P06Quest log. The user's Quest
cannot attach through Shadow PC. Keep MCC untouched. Planned mechanics remain
in PROJECT-PLAN.md; use HEADSET-TEST-CHECKLIST.md after display startup succeeds.
