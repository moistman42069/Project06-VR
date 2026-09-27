# Candidate 0.1.8: startup, first person, gestures and haptics

Status: built for user testing, not headset accepted. No public release.

## Startup regression

All six `193132`–`193201` logs from September 26 stop after the IL2CPP load-base
message, before input-hook installation. Every crash is SIGSEGV at address
`0x135`, with PC `libil2cpp.so+0x1073d3c` and LR `+0x10d19b4`. Disassembly at the
PC reads `ldrb w8, [x0, #0x135]`; the object/class argument is null. The crashing
thread is the Android activity thread, before Unity's managed startup.

The 0.1.7 regression called `klass(PlayerBase)`, `klass(SonicFast)` and method
lookup directly from `InstallGameHooks`, before IL2CPP class initialization.
0.1.8 removes that block. Startup only binds exported API addresses and installs
prologue-verified native detours. Managed discovery begins from an existing game
callback, requires `il2cpp_thread_current`, and stays on that same Unity thread.
`tools/audit-candidate.py` rejects early reflection and missing pinned icall names.
Log hashes and relative addresses are in `evidence/startup-017-crashes.json`.

This fixes the identified early-lookup path. It does not establish that every
later runtime path is correct without running the candidate on Quest.

## First-person and rendering audit

- Corrected GameObject use of `Component.get_transform` to `GameObject.get_transform`.
- Corrected nonexistent `Renderer.get_material` icall to `Renderer.GetMaterial`.
- Removed ambiguous `Material.SetColor` overload invocation; use `SetColorImpl(int, Color)`.
- Validate shader support before showing gloves; use available unlit/diffuse/standard shader.
- Use pinned `Object.m_CachedPtr` for native lifetime checks instead of a nonexistent
  `Object.op_Implicit` icall. Own/free managed handles for retained player, camera,
  renderer and trigger references. Glove objects persist independently of game-camera scenes.
- Update the eye anchor every rendered frame. `CameraTarget` contains camera
  lookahead and is not a head bone: use player position plus adjustable eye height.
- Keep first-person base yaw independent of player rotation and the following
  camera; integrate right-stick yaw, with head tracking applied once.
- Preserve anchor coordinates in game units; apply world scale only to tracked
  offsets, IPD and glove dimensions. Hand loss hides that hand, not the camera.
- Restore hidden renderer state when leaving first person or changing players.
- Temporarily reduce first-person near clipping. Move bridge-owned HUD panels to
  the first-person anchor in world space, restoring camera-space mode on exit.
- Keep the two-pass stereo provider and configless EGL startup path from the
  user-confirmed 0.1.4 baseline. Overlay-to-stereo redirection remains enabled.

Gloves are seven intersecting primitives per hand: palm, four fingers, thumb and
white cuff. They are render-only, with disabled colliders; no full-body IK or
authored Sonic skeleton is claimed. Quest must confirm orientation/proportions,
near clipping, HUD layout and the reported right-eye menu flicker.

## Gesture input

Separate OpenXR grip and aim actions are queried per hand with action-active,
position/orientation-valid and tracked checks. Quaternions and numbers are
validated. Coherent, timestamped head/hand samples expire after 150 ms; reference
space changes reset detectors. The right Meta/system button stays reserved.

All three toggles default off and currently require SonicNew in immersive mode.
Menu/focus/tracking loss, dead/locked states and invalid samples block gestures.

- Running requires opposing, alternating two-hand sagittal swings. Head
  translation is removed. Adjustable sensitivity and ramp feed a bounded run
  amount. The native game treats analog magnitude mostly as a walk/run switch,
  so ground-only `AccelerationSystem` temporarily scales native acceleration and
  maximum speed while the gesture owns input. Original maximum is restored
  immediately. Manual left stick overrides the gesture. Air, water, slide and
  charge states never receive a synthetic speed/collision modification.
- Homing requires the game's existing selected target and a native eligible
  jump/air state. Aim within 25 degrees, extend beyond 35 cm, dwell at least
  120 ms, then retract the adjustable distance at over 0.35 m/s. Target changes,
  1.5-second expiry, focus loss and 600-ms cooldown prevent repeat pulses. The
  bridge presses native A; it never invents an enemy or forces homing movement.
- Crouch depth is measured from standing calibration in tracking metres.
  A 150-ms dwell begins native X charge from grounded Sonic. Standing past the
  release hysteresis releases once, toward head yaw. Native charge/speed logic
  remains intact. Cancelling an owned charge uses the native StateAir transition
  (which stops charge sounds/state) before clearing X, preventing an accidental
  release launch. It settles through normal native grounding afterward.

## Haptics

Touch left/right haptic output paths use an optional OpenXR vibration action.
State/action transitions produce differentiated pulses; generic character states
receive a light default. Extra paths cover ring collection, movement/rail/water
cadence, spindash charge and release. Toggle and intensity persist in settings.
Finite amplitude/duration guards, pulse merging, 300-ms maximum durations,
120-ms queue expiry and explicit stopping on focus/menu/pause loss prevent stale
or sustained vibration. Optional binding failure does not disable base controls.
Character-specific timing and exhaustive event coverage still require playtests.

## Water and level compatibility

The 0.1.7 SonicFast ground-grace hypothesis was withdrawn. Returning true after
a failed ground ray leaves the associated RaycastHit invalid; it is not a safe
general collision fix. It also does not implement SonicNew's WaterSlide state.
The unused helper remains marked rejected for historical reference.

Pinned metadata and the reference reconstruction agree on WaterSlider entry,
WaterslideBooster speed changes and SonicNew's WaterSlide fields. Normal native
exits include spline progress over 1, grounded progress after 0.25, and speed
at or below 4. Those conditions are not overridden.

The new fallback records live WaterSlider/WaterslideBooster sphere and box
colliders, their active state and observed native trigger callbacks. On Sonic's
fixed-step callback it checks the previous-to-current position segment in each
collider's local space. Only a complete outside-to-outside crossing, unobserved
by native callbacks, can invoke that same trigger's native handler with Sonic's
actual collider. It excludes teleports over 12 units, stale steps over 150 ms,
menus, death, inactive colliders and already-consumed boosters. Endpoint overlaps
remain entirely owned by native collision handling. Other trigger types are
untouched. The segment routines are reusable, but are not enabled indiscriminately
for other level objects.

Logs report entry/booster speed, missed-crossing recovery and slide exit state,
speed/progress/position. **The filmed fall's exact cause is not proven by the
available evidence.** The fallback addresses missed water triggers; a low-speed
exit, intended route boundary or another defect would still need its own evidence.

Earlier gameplay logs also repeatedly show `AnimatedUV.Update` array-index
exceptions and a missing `_NormalMap` property. Disassembly verifies a 24-byte
TexScroll row (material index, string, speed) and `speed * Time.time` behavior.
The guarded implementation preserves valid rows and skips invalid material
indices, destroyed materials, missing shader properties and nonfinite offsets.
This removes that specific exception path without replacing the water shader.
It is not asserted to be the cause of the fall or the right-eye flicker.

## Validation and limits

- ARM64 native build; host menu/view/gesture/sweep/haptic tests.
- Pinned export/icall audit and native hook-prologue generation.
- Gesture tests include focus loss, recenter, stale/duplicate samples, no-target
  homing, cooldown, crouch cancellation and rejecting single-hand/body movement.
- Source archive integrity, signed APK verification, 16-KiB plugin/ZIP alignment,
  manifest/resource identity and unchanged original payload checks.
- Logs stay in Downloads/P06Quest. Original APK, earlier source archives,
  receipts and diagnostic evidence are retained. Intermediate APKs are disposable.

No USB headset is available through the user's Shadow PC. A successful build is
not a guarantee of visible Quest frames, visual quality or level completion.
The full headset checklist remains the acceptance gate.

## Primary references

- [Unity IL2CPP generated-code internals](https://unity.com/blog/engine-platform/il2cpp-internals-a-tour-of-generated-code)
- [Unity primitive creation and stripping requirements](https://docs.unity3d.com/2022.3/Documentation/ScriptReference/GameObject.CreatePrimitive.html)
- [Unity event execution order](https://docs.unity3d.com/2022.3/Documentation/Manual/ExecutionOrder.html)
- [Unity discrete collision limitations](https://docs.unity.cn/Manual/discrete-collision-detection.html)
- [Meta OpenXR input and pose actions](https://developers.meta.com/horizon/documentation/native/android/mobile-openxr-input/)
- [OpenXR action, reference-space and haptic specification](https://registry.khronos.org/OpenXR/specs/1.0-khr/html/xrspec.html)
- [OpenXR haptic feedback reference](https://registry.khronos.org/OpenXR/specs/1.1/man/html/xrApplyHapticFeedback.html)
- [Unity Canvas behavior](https://docs.unity3d.com/cn/2018.3/Manual/class-Canvas.html)
- [Unity right-eye overlay issue; different reproduction, not proof of this defect](https://issuetracker.unity3d.com/issues/xr-screenspace-overlay-canvas-is-rendered-to-hmd-when-xrsettings-dot-showdeviceview-is-set-to-false-and-shareddepth-buffer-enabled)
