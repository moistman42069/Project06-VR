# Candidate 0.1.11: scene-transition crash and save compatibility

Date: 2026-09-26. Version code 111. Test candidate; no public release.

## Headset result and diagnosis

The user confirms 0.1.10 removes title-screen pink flicker. Both supplied runs
then crash after slot selection, before MainMenu and before glove construction.
Preserve that title correction and the previously accepted first-person view,
hand orientation, crouch spindash and blue menu.

Both logs stop at `libil2cpp.so+0x109cb9c`, in Unity's managed-object liveness
traversal. The register presented as an object points into the UnityMain stack;
its alleged type-hierarchy pointer contains instruction bytes
`0xf94003e8a9007c2f`. The failing address is `0x4003e8a9007c27` in both runs.

The bridge assigned `m_SubsystemDescriptor` using
`il2cpp_field_set_value(sub, field, &desc)`. In this exact IL2CPP binary,
reference fields receive the third argument directly. This stores the address
of the temporary native variable, not the descriptor object. The invalid field
remains reachable through the managed subsystem list and is later traversed
during scene cleanup. This defect predates 0.1.10; surviving earlier runs did
not make the stored stack address valid. The new gauge/skybox camera helper used
the same incorrect convention, creating another potential corruption path.
Those camera callbacks had not been reached in the two supplied runs.

`tools/test-reference-abi.py` executes the actual pinned ARM64 setters in Unicorn.
It reproduces the incorrect stack-address write for string, class, object and
array reference types, and verifies that `il2cpp_field_set_value_object` stores
the intended managed object. The original scalar float setter remains correct.
This is executable ABI evidence, not a headset test.

## Correction

- All object-reference assignments use the explicit object setter, passing the
  managed object itself. Each assignment is read back and checked for identity.
- The XR descriptor assignment is verified before subsystem start. Its log now
  contains `XR descriptor managed reference verified by read-back`.
- Gauge/skybox bindings and save-version normalization share that corrected helper.
- Scalar writes are isolated in a template that only accepts arithmetic types;
  an audit rejects raw setter calls outside that wrapper. Ground-running cap
  behavior is unchanged.
- No XR submission, title-rendering, glove geometry, crouch, haptic or water
  behavior was reverted. New outward homing and the grouped VR menu are retained.

## Save compatibility

The original game writes `Application.version` into each save and demands exact
string equality in both slot display and selection. APK versionName changed
with every VR candidate despite identical save payloads. Native call-site
verification in `evidence/save-version-0111.json` confirms those comparisons and
the CreateGameData/SaveGameData version reads in this APK.

The game-facing version now uses the fixed schema-1 identity
`0.1.10-vr-candidate`, independent of Android versionName/versionCode, native
logging and the VR menu label. **Do not bump this identity for a mod update.**
If the pinned game payload/schema changes, design an explicit migration first.
The original game's version display may retain this internal compatibility
identity; Android package information, logs and the VR menu show 0.1.11.

Slot display accepts the exact prior candidate labels 0.1.0 through 0.1.10.
After native slot loading, only matching version strings in the in-memory slot
data are normalized. Progress, records, flags, gems, playtime and banner data
are untouched. Unknown versions remain incompatible. No file is rewritten just
to browse/select slots; the game's normal later save writes the stable identity.
This can recover compatibility for intact older files, not files already deleted
or overwritten by creating a new slot.

Install over the current candidate using the same package and signature. Do not
uninstall or clear application data to update. The original native serialization
and save paths remain in use.

## Verification and acceptance

- Two matching crash logs are preserved locally and hashed in crash-0110-runs.json.
- Actual ARM64 setter regression covers four reference kinds and a scalar float.
- Host tests cover all eleven prior save labels, malformed/unknown rejection,
  fixed schema identity, plus the existing gestures/menu/view regression suite.
- Native build, pinned hook prologues, IL2CPP export/internal-call audits and
  early-managed-lookup guards must pass before packaging.
- Signed APK, alignment, original payload hashes and matching source receipt
  must pass. Source archive excludes the original game, extracted meshes and keys.

Quest acceptance is still required: launch, select an existing previously
incompatible slot without deleting it, reach level select, enter a stage, return
to title, then quit/relaunch and reopen the same slot. Preserve the run log.
The scene-transition correction is backed by the crash and executable ABI test;
this machine cannot certify headset launch/rendering or inspect the user's saves.

Relevant API comparison: [IL2CPP API declarations](https://github.com/dreamanlan/il2cpp_ref/blob/master/libil2cpp/il2cpp-api-functions.h).
The decision relies on this APK's exported machine code and its executed behavior,
not an assumed ABI from another Unity release.

## Packaged result

Native build, host regression suite, ARM64 reference-setter execution, API audit
and signed-payload verification passed. Package: `out/p06-quest-0.1.11.apk`.
SHA-256: `919126d49896bf1fffd8a1c53e1b4212663d85b4b1d79c23ab9ad68130223d80`.
Matching source ZIP, receipt and external checksums are beside the APK.
The development certificate matches prior candidates. Headset acceptance remains
pending; no public release was created.
