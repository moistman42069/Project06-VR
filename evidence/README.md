# APK evidence

These small, reviewable files record facts measured from the pinned Android APK.
They support the native bindings and camera/input decisions in `src/`.

The large IL2CPP dump, ELF files, Unity bundles, APK payload and generated
reports remain local and ignored. `RInput.Awake.asm` and the related Rewired
lead are retained as a superseded investigation record; the shipped Android
input path is the ControlFreak2 `CF2Input` path documented in
`docs/REVERSE-ENGINEERING-NOTES.md`.

Do not treat an evidence lead as a binding until it is checked against the exact
APK identity in `apk-identity.json`.
