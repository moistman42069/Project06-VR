# Candidate 0.1.9: movement, hands and menu refinement

Status: candidate for Quest testing, not a public release or headset acceptance.
The user confirmed 0.1.8 launch, immersive rendering and good haptics. The supplied
203905 run and 06bug.webp are preserved in ignored out/headset-logs. Log identity
and water events are recorded in evidence/candidate-019-run.json.

## Movement and gestures

First-person eyes used a stable VR heading but physical sticks retained the
chase-camera basis. Gesture steering also used PlayerCamera.Camera rotation,
where native RotatePlayer uses PlayerCamera.transform.localEulerAngles.y.
Remap first-person free-movement sticks into that native yaw basis, retaining
analog magnitude and native slope handling. Near-vertical head direction retains
the last usable heading. Third-person sticks and rail/water/vehicle lane controls
remain stock. Mach-speed and snowboard motors retain their specialized controls.

Swing running previously lost its demand at every arm reversal and reduced
native acceleration a second time. A decaying stroke-speed envelope now bridges
reversals, requiring alternating two-hand cadence and recent motion. The selected
speed cap changes while native acceleration remains intact. Physical stick wins;
focus/tracking/menu loss cancels input. Sensitivity and ramp remain adjustable.

Homing pointing can arm before attack eligibility, including preparing a jump.
A pull fires only when the native state accepts it, the same native target remains
selected and physical action buttons are released. Include native AfterHoming
chains and unlocked spring/rope launches. Reject implausible pull velocity and
preserve range/occlusion/state restrictions. This does not add ground homing or
arbitrary point-selected targets. Crouch now filters height before its existing
dwell/hysteresis, preserving calibration, native charge and safe cancellation.

## First person and menu

The old grip-axis permutation points fingers sideways, matching the screenshot.
Gloves now use grip position and aim orientation from the same tracking sample,
with mirrored thumbs and subtle palm/cuff shading. They remain procedural gloves,
not rigged character hands or IK. Pose loss hides only that glove.

Body hiding includes player renderer arrays, Metal Sonic's singular renderer,
Upgrades.Renderers and renderer children of the visual Mesh. Saved enabled states
and managed handles are restored/released. This removes accessory cuffs rather
than attaching floating feet; no collider or physics object is changed.
Eye height remains adjustable, without anatomical head-bone anchoring.

The blue menu has one vertical category list: stick scroll, A enter/apply, X back,
B close. Both-stick chord and reserved Meta recenter remain. Navy/cobalt panels,
cyan values, gold highlights and an italic pixel title provide a Sonic-inspired
theme. Values occupy separate lines. Existing settings remain compatible.

## Main-menu right-eye flicker

The log has 20 GL_INVALID_OPERATION mentions without shader identity or a failing
per-eye draw. It cannot prove the exact GPU cause. Earlier canvas redirection did
not fix the symptom and is not claimed as its cause.

Pinned level2 extraction shows a DeferredShading Main Camera with forced
intermediate rendering, a DeferredShading Model Camera targeting a shared UI
texture while configured for both eyes, and a VideoPlayer background texture.
Neither serialized menu camera contains PostProcessLayer. Records are preserved
in evidence/menu-camera-components.json.

The candidate uses forward rendering only for the named main-menu cameras,
explicit mono for the flat model-preview texture, and removes forced intermediate
rendering on those cameras and the bridge HUD camera. Where BackgroundVideo has
its authored static fallback, use it and stop the animated background. Videos
without that fallback remain stock; game settings on disk are not rewritten.
Animated menu backgrounds are intentionally unavailable in this candidate.
Gameplay cameras/shaders and original game assets remain unchanged.

New hooks come from pinned metadata and verify 16 prologue bytes. Managed work
retains the working startup/thread guard. OpenXR/EGL/frame-submission code and
haptics are unchanged. This bounded compatibility change still needs Quest
verification; it is not proof that the flicker has disappeared.

Primary references reviewed:

- [OpenXR pose semantics](https://registry.khronos.org/OpenXR/specs/1.0-khr/html/xrspec.html#semantic-paths-standard-pose-identifiers): grip and aim have different purposes; grip -Z is not fingertip-forward.
- [Unity stereoTargetEye](https://docs.unity.com/en-us/engine/6000.3/script-reference/unityengine/camera/stereotargeteye) and [RenderingPath](https://docs.unity.cn/2022.2/Documentation/ScriptReference/RenderingPath.html): explicit eye selection and path meanings.
- [Unity PostProcessLayer source](https://github.com/Unity-Technologies/PostProcessing/blob/v2/PostProcessing/Runtime/PostProcessLayer.cs): reviewed stereo blit handling, but no global postprocessing patch is justified by this scene's components.

## Character and level scope

Pinned metadata contains 13 PlayerBase subclasses: SonicNew, SonicFast, Princess,
SnowBoard, Shadow, Silver, Tails, Amy, Knuckles, Blaze, Rouge, Omega and MetalSonic.
Code presence does not prove all are selectable in this Android port. Eleven use
RotatePlayer/AccelerationSystem for normal ground locomotion; Mach-speed Sonic
and snowboarding have specialized controls. Visual hiding is shared. Native jump
homing is supported for SonicNew, Shadow, MetalSonic and Princess; crouch spin for
SonicNew, Shadow and MetalSonic. Other characters keep their physical-button
abilities. No per-character headset acceptance is claimed.

The latest water log has entry, two boosters and a normal exit at progress
1.00047. Preserve bounded swept-trigger recovery and native exit conditions.
This does not prove all routes or the older filmed fall are fixed. Never invent
grounding. Full IK, authored character hands/feet and world interaction remain
future work.

## Validation and acceptance

- ARM64 build, pinned API/export/lifecycle audit and character metadata checks.
- Realistic two-hand waveform replay at 72/90/120 Hz sustains over 90% of requested
  speed at maximum sensitivity after ramp-up; a second stationary returns zero.
- Whole-body/single-hand rejection, stale/focus/tracking/recenter behavior,
  crouch charge/release/cancel and unsupported-character spin rejection.
- Head/camera yaw remapping, analog magnitude, view/IPD math, category navigation,
  input capture and settings persistence. Root and category raster visual review.
- Final signed-APK integrity, payload preservation, ABI/alignment and archive checks.

These checks cannot certify visible pixels, hand ergonomics, every character,
collision route or runtime frame timing. New five-second locomotion telemetry
records prefab, native/head yaw, run demand, speed and attack eligibility. Menu
compatibility milestones are logged. Use the headset checklist before release.
