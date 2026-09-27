# Candidate 0.1.10 — title rendering and movement refinement

Date: 2026-09-26. Version code 110. Local test candidate; no public release.

## Accepted baseline

The user tested 0.1.9 and confirmed crouch spindash, hand orientation,
first-person head-follow view and the blue category menu. Startup, immersive
stereo and haptics also work. Preserve the managed-startup guard, EGL/OpenXR
submission, first-person anchor and crouch/haptic behavior. The new candidate
has not been tested on a headset.

## Evidence and changes

1. **Title screen:** the previous correction only ran at MainMenu.Start.
   The supplied level0 title camera is named Camera, uses DeferredShading (3),
   HDR and ForceIntoRT. It has Transform, Camera and AudioListener components;
   there is no camera-attached post-process script to disable. TitleScreen.Start
   now sets Forward, LDR, both eyes and no forced intermediate on every entry.
   The accepted MainMenu path remains unchanged. This removes a concrete
   unhandled configuration; the log does not identify the exact failing GPU
   draw, so pink-free rendering still needs headset verification.
2. **Airborne steering:** the previous state whitelist remapped input before
   native movement consumed it, missing launch states and later camera changes.
   Pinned PlayerBase.RotatePlayer and SlopePhysics hooks now remap only their
   axis reads using the current native camera basis and tracked head heading.
   Ground/air rotation locks and acceleration remain native. Rail, water, vehicle,
   mach-speed and snowboard lane reads are not globally rotated. Failure to
   install both hooks keeps the previous state-gated behavior and logs it.
3. **Homing:** either hand can trigger one native jump/homing pulse with an
   outward stroke. Default travel 10 cm, adjustable 4–30 cm; no pointing/pull
   sequence. Requires a live native target and an eligible native attack state.
   Radial velocity, actual hand velocity, tracking validity and a 400 ms cooldown
   reject head-only motion, inward pulls, discontinuities and repeats. Existing
   settings migrate to the new distance default while preserving other choices.
4. **Hands:** locally extract the authored ch_sonic_cloth glove submesh from
   sonic_Root in this exact APK. Wrist/finger bind poses establish handed axes;
   one Loop subdivision and smooth normals soften the original silhouette.
   Each hand has 1,909 vertices and 3,792 triangles. Keep the accepted grip-position
   and aim-orientation mapping. Create two render-only, collider-disabled meshes
   through checked managed methods; failed construction restores the body/view
   fallback without stopping XR. No feet or arm IK are added. These shared Sonic
   gloves follow the existing supported character camera path; character-specific
   glove skins and finger articulation are future work. Generated game geometry
   stays ignored and is excluded from the public repository/source archive.
5. **Visibility:** the provider already supplies matching eye/culling poses per
   eye. Bypass baked occlusion on world-output cameras, preserving those stereo
   frustums, native layer masks, far clip and LOD. This is conservative compatibility
   for displaced viewpoints, not proof of the exact reported disappearing draw.
   More objects can reach rendering; Quest frame-time cost needs verification.
6. **Concrete null-camera faults:** the latest log contains 591 NullReference
   lines: 586 GaugeController.Update, one GaugeController.Start, two Settings and
   two SkyboxModel.Start. Native disassembly confirms Gauge/Skybox Start dereference
   Camera.main then assign just their camera field. Bind those fields to the
   existing active-camera fallback instead. Gauge Update retries the binding and
   requires its renderer before calling the original method. Camera tags are not
   changed globally: doing that would also activate unrelated Settings/post-FX.
   Settings exceptions are recorded but not broadly patched in this candidate.
7. **Menu:** five root entries: UI, VR, Graphics, Haptics, System. VR contains
   physical running, homing and crouch controls and their sliders; scroll reveals
   later rows, X returns to the root. Preserve blue styling and input capture.

## Water investigation: no new physics change

The user still reports the filmed fall. Latest telemetry has multiple native
water entries, boosters and three spline exits with progress just over 1.0.
One intervening entry has no matching exit before a later player/scene change.
That is insufficient to identify the falling segment or distinguish a missed
trigger from a stage/spline issue. Searches for this Android port, Lowfriend,
Wave Ocean and water-slide falls did not locate a corroborated matching report.
PC/original-2006 reports are not evidence of this Android bug. Per the user's
instruction, leave the swept-trigger recovery, native slide exits and physics
unchanged; do not add invented water grounding or an infinite slide.

## References and verification

- [Unity XR display provider](https://docs.unity3d.com/2022.3/Documentation/Manual/xrsdk-display.html): render/culling poses and intermediate-target conditions.
- [Unity camera occlusion API](https://docs.unity3d.com/2022.3/Documentation/ScriptReference/Camera-useOcclusionCulling.html): the camera-specific control used here.
- [Unity rendering path](https://docs.unity.cn/2022.2/Documentation/ScriptReference/RenderingPath.html): forward/deferred enum semantics.
- [Unity camera bindings](https://github.com/Unity-Technologies/UnityCsReference/blob/master/Runtime/Export/Camera/Camera.bindings.cs): camera property interfaces; every internal call is additionally checked against this APK's libunity strings.
- [ImmersiveFirstPerson](https://github.com/Gerominoes/ImmersiveFirstPerson): another Unity first-person mod explicitly disables occlusion to keep nearby props visible. This is a comparable approach, not validation of Project 06 or Quest performance; no code was copied.
- [OpenXR poses](https://registry.khronos.org/OpenXR/specs/1.0-khr/html/xrspec.html#semantic-paths-standard-pose-identifiers): grip and aim pose distinction, preserving the accepted controller mapping.

Evidence: candidate-0110-run.json, title-camera-components.json,
billboard-native-audit.txt, glove-extraction.json and bindings.json under evidence/.
The latest run is preserved locally under out/headset-logs and identified by hash.

Host tests cover gesture replays at 72/90/120 Hz, noise rejection, crouch
regressions, heading transforms, menu navigation/scroll/back, settings migration,
view transforms and input capture. Mesh checks cover finite data, valid indices,
unit normals and geometry previews. Native build and ABI audit check managed
mesh signatures, exported APIs, internal calls, pinned hook prologues and the
no-early-managed-lookup guard. Character audit covers 13 classes present in the
APK; that does not establish all are playable from its menus.

The signed APK must pass signature/alignment, original payload hashes, ARM64
library/export checks and receipt matching. Preserve 0.1.9 as the tested fallback.
The headset checklist remains the acceptance test; no launch/render guarantee
can be established on this Shadow PC without the user's Quest run.

## Packaged result

Native build, host tests, API/character audits and signed-payload verification
passed. APK: `out/p06-quest-0.1.10.apk`, 1,761,946,779 bytes.
SHA-256: `62fd49400579375d618f25b879dd07bfa45732f1474a7ad5b9a64ce63e3ef937`.
Matching source and external checksums are beside it. Install over 0.1.9 with the
same development signature to retain settings. No public release was created.
