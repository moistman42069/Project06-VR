# Reverse-engineering notes

The supplied APK is the authority for bindings. The Project06OSP GitHub source
is useful for naming and gameplay context, but it is an older desktop
reconstruction and must not be treated as an Android binary map.

The Android build is Unity 2022.3.62f1 IL2CPP metadata version 31. Gameplay and
menu input callers use `ControlFreak2.CF2Input`; the older Rewired path is not the
native hook target. The gameplay prefab has a disabled primary PlayerCamera and
an active `OutlineCamera`, so camera selection includes the latter. Game HUD
canvas roots are separated from the ControlFreak touch canvas so the touch rig
can remain alive while its visual overlay is hidden.

## Build equivalence

The pinned APK identifies itself as Android version name `5.0` and Unity player
bundle version `5.0`, with package `com.NightsofKronos.SonictheHedgehog` and
build GUID `adb29565b76047428e20270a1f49a1ed`. These are the APK's own build
identifiers; `5.0` must not be presented as a confirmed PC release number.

The file has now been source-matched externally. The exact filename
`p-06RELEASE64.apk` and the 1.65 GB MediaFire listing match the APK link on the
Sonic P-06 fan page; that listing says it was uploaded on 2025-09-10. The same
date matches Lowfriend's video titled **SONIC P-06 ANDROID PORT. SONIC RELEASE**.
The available gameplay descriptions identify this Android port as the Sonic-only
release line, rather than the later all-campaign PC package.

Provenance links: [MediaFire APK listing](https://www.mediafire.com/file/tgiales231mf4v0/p-06RELEASE64.apk/file),
[Lowfriend release video](https://www.youtube.com/watch?v=crjt6Sth97Q), and
[Android/PC overview](https://sonic-fangames.com/sonic-p-06/).

The closest PC content equivalent is therefore **Project '06 — Sonic Release**
(the PC Sonic trial release), whose announcement describes the ten Sonic/Tails
stages. It is not safe to label the APK as PC Silver Release v1.45: that is the
later PC package containing Sonic, Shadow and Silver campaigns. The Android file's
internal `5.0` is its Android/Unity package version, not a PC release number.

Confidence: exact external APK source match — high; Android port name/content
line — high; exact PC commit/build hash — unproven. A byte-for-byte PC match would
still require the corresponding PC executable or a same-release asset/code hash.

## Newer Android build search

Checked 2026-09-22: no newer publicly documented Lowfriend APK was found. The
August 2025 “full mobile port” showcase is an earlier development build; the
September 10, 2025 Sonic Release APK is the later known public build. The same
MediaFire file remains linked by later community posts, and no Android patch,
version tag or replacement APK with a later upload date was located. This is a
research result, not proof that a private Discord test build cannot exist.

Exact method RVAs, prologues, caller counts, camera/canvas findings and API
audits are kept under `evidence/`. Those files are generated investigation data;
large APK, ELF, dump and Unity asset files remain ignored/local.

Future gesture and IK work must begin with probes and logs. Do not write a theory
into a binding document before a runtime observation supports it.
