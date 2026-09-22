# Quest 3 startup failure and candidate 0.1.1

## Observed failure

The user ran 0.1.0 twice on Quest 3, Android 14 / API 34. The longer log,
`P06Quest-20260922-134830-882.log.txt`, reports:

```
Resources$NotFoundException: String resource ID #0x0
com.unity3d.player.i0.a
com.unity3d.player.i0.<init>
com.unity3d.player.UnityPlayer.<init>
com.unity3d.player.UnityPlayerActivity.onCreate
com.p06.quest.QuestActivity.onCreate
```

The other log ends after successful native hook installation and does not
contain an exception. Do not infer a separate cause from its truncated tail.
Neither log proves XR startup, gameplay input, HUD or stereo rendering.

The original DEX disassembly at method `i0.a(Context)` reads the activity's
package name, calls `Resources.getIdentifier` for
`string/game_view_content_description`, then passes the result to `getString`
without checking for zero. Candidate 0.1.0 changed the manifest package to
`com.p06.quest` while retaining the original resource namespace
`com.NightsofKronos.SonictheHedgehog`. AAPT2 independently confirms this mismatch.
The entry exists as 0x7f040006, under the wrong namespace.

## Correction

`tools/apk_resources.py` updates only the resource package name buffer in
`resources.arsc`. Package ID 0x7f, all entry IDs, offsets, chunk sizes and
resource payloads are preserved. The patch requires the pinned original
namespace and rejects malformed input, unexpected package layouts and a second
application. The layout follows AOSP's
[ResTable_package definition](https://android.googlesource.com/platform/frameworks/base/+/refs/heads/main/libs/androidfw/include/androidfw/ResourceTypes.h).

All six getIdentifier call sites in the original Java DEX were inspected:
two require game_view_content_description, one assigns unitySurfaceView,
two cover optional splash drawable/background color, and one looks up the
Android framework OK string. App namespace correction covers all app-package
lookups. The optional splash drawable is absent in the original too and its
lookup is zero-guarded. Framework resources are unaffected.

Java startup records resource lookup IDs, resolved description and completion
of UnityPlayerActivity.onCreate. A synchronous catch writes startup exceptions
before rethrowing; a delegating uncaught-exception handler also records later
Java failures when it remains installed. This reduces reliance on a background
logcat reader surviving process termination. It cannot guarantee logs after OS
termination or storage failure.

## Validation

- Resource regression tests pass, including bounds and unexpected-namespace rejection.
- The actual failed APK fails the namespace condition; 0.1.1 passes.
- AAPT2 parses the final manifest and resource table: package com.p06.quest,
  description 0x7f040006, surface ID 0x7f020000 and theme 0x7f050001.
- Native compilation, Java compilation and DEX generation pass. The Unity
  compile-time stub is excluded from the APK.
- All five packaged native libraries are AArch64. DT_NEEDED libraries are
  bundled or available in the Android API 29 NDK platform stubs; no missing
  private dependency was found. This is not a runtime symbol-resolution test.
- Existing production menu and view-math tests pass.
- APK signature verification, 16 KB ZIP alignment and full ZIP CRC checks pass.
- All 35 retained original APK entries match by streamed SHA-256, including
  the Unity game bundle and original native libraries.
- Signing certificate matches 0.1.0; versionCode increases from 100 to 101.

Local detailed verification: out/compatibility-audit.json, packaged-resources.txt,
packaged-manifest.txt, apk-signature-verification.log and the versioned receipt.
The original package/failed candidate and both supplied logs are preserved.

The identified startup defect is corrected. There is no headset/emulator result
for 0.1.1, and no claim that all later rendering or gameplay paths are defect-free.
The next acceptance test is the user's Quest 3 run. No public release is created.
