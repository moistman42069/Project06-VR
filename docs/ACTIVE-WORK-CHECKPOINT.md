# Active work checkpoint

Date: 2026-09-26
Status: candidate 0.1.3 launches Unity and creates the OpenXR instance but the
user's Quest log proves graphics startup fails at EGL config resolution. Candidate
0.1.4 addresses that binding and adds startup/frame evidence. Quest 3 visibility
acceptance remains pending.

APK: `out/p06-quest-0.1.4.apk` (versionCode 104).
SHA-256: `54b6f34480e341eca4ecc4a595e199acfdc4425042fa1bb7b7c1e6e54677d66e`.
Matching source ZIP: `out/p06-quest-0.1.4-source.zip`.
Receipt: `out/p06-quest-0.1.4-receipt.json`.

Read [EGL and session startup investigation](EGL-SESSION-STARTUP-20260926.md).
The user tested 0.1.3 through Shadow PC and cannot connect the Quest by USB.
No headset acceptance or public release is recorded. Keep MCC untouched.

On 0.1.4, check the EGL context/draw-surface config-query line, resolved config,
`XR graphics initialized`, OpenXR session state transitions, and
`First OpenXR frame submitted`. The headset loading view must actually clear;
logs and static checks alone do not prove visible or correct stereo output.
