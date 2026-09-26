# XR descriptor startup on Quest

## Headset result from 0.1.2

The 2026-09-22 Quest log proves Unity is running: Unity Main invokes the title
start hook, audio/input produce feedback, and the game loop continues for more
than 17 seconds. No XR display-provider entry appears. Our own message repeats
`XR bootstrap: 0 descriptors` at two-second intervals. The headset remains in
its app-loading view because there is no registered/running XR display and no
OpenXR compositor session to present frames.

The log does not prove a native crash. The app remains alive, which distinguishes
this from the first two startup failures. The complete user log and 0.1.2 receipt
are preserved under `baseline/0.1.2-xr-discovery/`.

## Code path correction

The previous bootstrap initialized
`UnityEngine.SubsystemsImplementation.SubsystemDescriptorStore` directly, read
`s_IntegratedDescriptors`, and only looked up `UnityEngine.SubsystemManager`
after a provider had already been created. Unity's pinned IL2CPP dump says the
`SubsystemManager` static constructor calls
`StaticConstructScriptingClassMap`; the Unity 2022.3 reference binding marks
that function as internal/native. The store's own initializer only creates its
lists. Accessing the store alone does not run the manager's class-map setup.

Candidate 0.1.3 now finds `UnityEngine.SubsystemManager` and runs its IL2CPP
class initializer on Unity's thread before it initializes/reads the descriptor
store. It logs that boundary and the integrated descriptor count. The rest of
the selected XR display descriptor creation and provider-start path stays
unchanged. This is an evidence-based correction for the exact zero-descriptor
condition in the headset log; it has not yet been observed on the user's Quest.

References: the supplied APK's pinned Il2CppDumper `dump.cs` identifies the
SubsystemManager `.cctor` and `StaticConstructScriptingClassMap` RVA; Unity's
[2022.3 SubsystemManager binding](https://raw.githubusercontent.com/Unity-Technologies/UnityCsReference/2022.3/Modules/Subsystems/SubsystemManager.bindings.cs)
shows its required native class-map entry point. Unity's [native XR provider
setup](https://docs.unity.cn/6000.2/Documentation/Manual/xrsdk-provider-setup.html)
requires a manifest and native plugin with matching provider name/ID, which are
already packaged as P06Quest.

## Candidate

`out/p06-quest-0.1.3.apk`, versionCode 103, SHA-256
`76b2477e7585fb34d76f3d048045d45aeb8d0211ad0d8bc964de4e9fd75d4fe3`.
Install over the same-signed candidate 0.1.2. The APK rebuild, managed/native
compilation, resource/unit/menu checks, signature/alignment/CRC checks passed.
All 35 original game entries match by SHA-256; the five packaged native
libraries are ARM64 with no missing Android/bundled dependencies. No public
release is made. The acceptance test remains the user's Quest 3 run: confirm
a positive descriptor count, provider registration, `XR graphics initialized`,
`XR display running=1`, and actual disappearance of the app-loading view.
