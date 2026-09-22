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

There is currently no evidence that maps this APK to an exact PC executable,
PC tag or Project06OSP commit. The public Project06OSP repository is an
AssetRipper/decompilation source snapshot whose current `main` commit is dated
2023-11-13; it does not provide a matching Android build manifest or a reliable
cross-platform version table. The safest description is therefore: **the APK is
an Android P-06 5.0 build, with no verified one-to-one PC equivalent**. A precise
equivalence would require a PC executable/build manifest or matching gameplay
asset and code hashes from the same release.

Exact method RVAs, prologues, caller counts, camera/canvas findings and API
audits are kept under `evidence/`. Those files are generated investigation data;
large APK, ELF, dump and Unity asset files remain ignored/local.

Future gesture and IK work must begin with probes and logs. Do not write a theory
into a binding document before a runtime observation supports it.
