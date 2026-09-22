# Active work checkpoint

Date: 2026-09-22
Status: 0.1.1 still fails on Quest 3; 0.1.2 awaits headset retest.

Current APK: out/p06-quest-0.1.2.apk (versionCode 102).
SHA-256: 7b89fdf6217611cfb278d2186f3714c13ea6dfff615ff1e06ee1ecfb5a178edb.
Source: out/p06-quest-0.1.2-source.zip.
Receipt: out/p06-quest-0.1.2-receipt.json.

Read NATIVE-STARTUP-20260922.md for the exact evidence, changed startup path,
validation and limits. The 0.1.0 resource failure is headset-proven resolved;
0.1.1 completes Java activity startup but exits later, without a captured cause.
0.1.2 removes global loader interception and adds early/native/previous-exit
logging. Do not claim its startup or VR behavior is headset tested.

Both sets of failed logs/receipts are preserved in baseline; prior APK/source
archives remain in out. No candidate is accepted or promoted to a public release.
The next action is the user's test of 0.1.2 as an update over 0.1.1. Read the
new run log, including any previous-exit base64 trace blocks; decode native
protobuf traces before guessing another cause. The user cannot connect Quest
USB through Shadow PC. Keep MCC untouched. Planned mechanics remain in
PROJECT-PLAN.md; full acceptance items are in HEADSET-TEST-CHECKLIST.md.
