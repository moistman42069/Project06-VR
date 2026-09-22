# Native startup investigation: 0.1.1 to 0.1.2

## Headset evidence

The two user logs from 2026-09-22 at 14:12:35 and 14:12:52 both show:

- Correct nonzero resource IDs and the resolved description "Game view".
- Completion of UnityPlayerActivity.onCreate.
- Unity native memory/CPU/build information and Company/Product Name.
- No provider-registration, XR-startup or TitleScreen.Start log.

Therefore the 0.1.0 resource fault is resolved on the headset. The app still
fails later. Neither log contains an exception, native stack, exit status or
OS termination reason; the exact second failure is **not confirmed**.
The complete logs and prior receipt/signature record are preserved locally under
baseline/0.1.1-startup-failure. Older APK/source artifacts remain intact.

## Startup change

Previous nativePrepare installed process-wide dlopen and android_dlopen_ext
interceptors merely to notice libil2cpp loading. Those interceptors call the
original loader from the mod/trampoline, altering caller identity. Android's
linker uses caller identity to select a namespace. This is an avoidable risk to
subsequent graphics-driver loading, regardless of whether it caused this exit.
See [AOSP caller handling](https://android.googlesource.com/platform/bionic/+/a7fc7f9909c221a0f64c5c5ecc5fadd5fba467c5/linker/dlfcn.cpp).

0.1.2 leaves system library loading untouched. After super.onCreate returns,
nativeAttach obtains an already-loaded libil2cpp handle with RTLD_NOLOAD and
installs the existing signature-checked game hooks. Both failed-run logs prove
that this library is loaded before activity creation completes. The original
DEX confirms UnityPlayer construction inside onCreate and onResume forwarding.
A missing handle is explicitly logged; it never silently forces an early load.
The obsolete interceptor code is unreachable and eliminated from the compiled
plugin, verified from its ELF symbols and strings.

No camera, HUD, menu or control behavior was changed. They remain unaccepted
in a headset because neither run reached their verification.

## Diagnostics

The native signal recorder is armed during nativePrepare, before Unity creation,
instead of first waiting for managed game code. Unity/system handlers can still
replace it later. Logcat capture includes all priorities/tags for this app PID,
covering libc errors omitted by the previous filter. Provider entry and interface
acquisition have explicit startup markers.

Before the next Unity start, Android ApplicationExitInfo is queried for up to
three exits of this package. Trace data, when available, is preserved verbatim
as base64 (2 MiB maximum per trace, truncation explicitly marked). Native crash
traces on API 31+ are protobuf, not text; decode after extraction. Prior trace
availability depends on Android retaining the record. No other app's history is
queried. See [Android exit-report API](https://developer.android.com/reference/android/app/ApplicationExitInfo).

## Candidate and checks

APK: out/p06-quest-0.1.2.apk; versionCode 102.
SHA-256: 7b89fdf6217611cfb278d2186f3714c13ea6dfff615ff1e06ee1ecfb5a178edb.

Native/Java compilation and DEX generation pass. Resource regression tests and
existing production menu/view tests pass. Final signature and ZIP alignment/CRC
checks pass; all 35 retained game entries match the original by SHA-256. Native
libraries remain AArch64 with bundled or Android API 29 platform dependencies.
The certificate matches 0.1.1 for installation as an update. Source/archive and
receipt verification are recorded locally. No release is published.

This is a targeted startup candidate with better crash evidence, **not a
headset-confirmed fix for the second failure**. Install over 0.1.1 and retain
the new Downloads/P06Quest log whether startup succeeds or fails. It may contain
the system's saved report for the prior failed build. Do not clear app data or
uninstall merely to install this same-signed update.
