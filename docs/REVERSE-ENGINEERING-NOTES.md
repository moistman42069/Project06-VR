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

Exact method RVAs, prologues, caller counts, camera/canvas findings and API
audits are kept under `evidence/`. Those files are generated investigation data;
large APK, ELF, dump and Unity asset files remain ignored/local.

Future gesture and IK work must begin with probes and logs. Do not write a theory
into a binding document before a runtime observation supports it.
