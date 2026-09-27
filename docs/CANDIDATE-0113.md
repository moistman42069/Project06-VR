# Candidate 0.1.13: smaller UI and anchored settings panel

The user calls 0.1.12 a fantastic build. Its log is preserved locally, with hash
and findings in evidence/candidate-0112-run.json. Slots and authored gloves load;
one existing Settings.SetLocalSettings null reference remains. No unrelated game
settings or speculative water changes are included.

## Changes

- HUD size now spans **5-200%**, depth 0.25-20, horizontal/vertical offsets -10
  to +10. Screen width spans 0.1-20, screen depth 0.25-20, stereo strength 0-4.
- Title/main-menu UI has separate size, depth and X/Y controls plus reset.
  Defaults: **65% size**, depth 3, centered. Size spans 5-200%; depth/offset ranges
  match HUD controls. This affects the UI canvas, not the underlying 3D scene.
- VR panel size spans 0.1-3 meters; distance 0.25-10 meters. Defaults retain
  1.05-meter size and 1.15-meter distance. Its opening pose is anchored, so looking
  around no longer moves the panel. Close/reopen or recenter to bring it in front
  again. Optional follow-view and a reset are in UI.
- **Immersive Mode (All)** is the first VR row. Enable selects immersive display,
  positional tracking, first person, physical running, outward homing and crouch
  spindash. Disable turns the four optional interactions off and retains VR.
  The indicator derives from individual settings; sensitivities and haptics are
  preserved. Unsupported character/state guards still apply.
- Default left glove pitch/yaw/roll is (-10,-5,-10) degrees; right is (-10,+5,+10).
  These are modest ergonomic adjustments, not a claim of measured headset fit.
  The user confirmed no hand calibration was performed in the accepted run.
  Existing nonzero custom angles survive migration; untouched hands adopt the
  new defaults. Reset restores the new defaults. Gesture poses are unchanged.

Scale/depth remain positive: negative size mirrors content and negative depth
places it behind the viewer. Position offsets support both directions. Each UI
group has a reset; no game save or progress is rewritten.

## Implementation and review

The panel captures a valid headset pose in the existing render reference space
when opened. It retains that pose during head movement, invalidates it after
reference-origin changes, and retries if valid tracking is unavailable. A panel
failure never gates the game projection layer. This follows the OpenXR quad's
[space-relative pose definition](https://registry.khronos.org/OpenXR/specs/1.1/man/html/XrCompositionLayerQuad.html)
and [world-stable layer guidance](https://github.com/KhronosGroup/OpenXR-Guide/blob/main/chapters/frame_submission.md).
The eye swapchains, projection submission and session lifecycle are unchanged.

Title/menu ownership is obtained from the existing verified Start hooks. The
managed component is rooted and checked for native object liveness; its root is
released on replacement/destruction. Only bridge-owned canvases are resized,
using the existing world-space transform path. Title camera compatibility and
model render-texture fixes are retained. Unity's [Canvas render modes](https://docs.unity3d.com/2022.3/Documentation/Manual/UICanvas.html)
explain why explicit placement uses world-space canvas transforms.

VR settings schema 7 accepts schemas 1-6. Game save identity stays fixed at
0.1.10-vr-candidate. No new engine binding, native hook or managed field write.

## Checks and limits

Native ARM64 build and pinned ABI audit pass. Host tests cover panel pose capture,
head motion, reopening, recenter, follow-view transitions and invalid poses;
expanded UI bounds, reset/persistence, settings migration, preserved custom hand
angles and the all-interactions switch. Existing eye/view/gesture/menu checks
also pass. New menu pages were rasterized and inspected for legibility.

The final package is checked for signing, alignment, ARM64 libraries, exports,
and unchanged original game payload. Matching source is verified against the APK
receipt. 0.1.12 remains the tested fallback. New title canvas behavior, ergonomic
fit, panel anchoring and scene transitions need Quest testing; there is no public
release and no headset guarantee from host validation.
