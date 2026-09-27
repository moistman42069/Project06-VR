# Historical 0.1.7 diagnosis — superseded

The water-ground interpretation and proposed grace hook below were incorrect.
0.1.7 crashes during its early managed lookup. See [CANDIDATE-018.md](CANDIDATE-018.md)
for the corrected diagnosis; the following text is retained as investigation history.

Date: 2026-09-26. Evidence: candidate 0.1.4 run logs supplied by the user,
the supplied Quest recording, the pinned APK's Unity metadata, and the matching
Project06OSP C# reconstruction. Candidate 0.1.7 contains a targeted menu conversion, an experimental
first-person path and bounded water-ground seam continuity; none has been
retested on the headset.

## Right-eye menu flicker

The 0.1.4 logs report a live `ScreenSpaceOverlay` canvas while VR is enabled.
Unity says that mode is not visible through the XR camera. The bridge did
convert overlay canvases to the dedicated stereo HUD camera, but it only scanned
once every 60 display updates. The same logs show canvases being converted again
after scene/UI changes, which establishes that the game recreates or resets
overlay canvases after the initial scan.

This is consistent with Unity's documented XR canvas modes: Screen Space Camera
is rendered by a camera and Screen Space Overlay is rendered without reference
to a camera ([Unity Canvas manual](https://docs.unity3d.com/cn/2018.3/Manual/class-Canvas.html)).
Unity's issue tracker also records an XR defect in which a Screen Space Overlay
canvas appears in the right eye only ([UUM-1557](https://issuetracker.unity3d.com/issues/xr-screenspace-overlay-canvas-is-rendered-to-hmd-when-xrsettings-dot-showdeviceview-is-set-to-false-and-shareddepth-buffer-enabled)).
That issue is not an exact reproduction of this Unity 2022 custom provider, but
it supports the failure class. The custom provider currently uses Unity's
documented two-pass arrangement (two render passes, one parameter/texture each)
([Unity XR display provider manual](https://docs.unity3d.com/es/2021.1/Manual/xrsdk-display.html)).

Candidate 0.1.7 hooks Unity's Canvas render-mode setter after the stereo HUD
camera is ready. Requests to switch a canvas into Screen Space Overlay are
redirected to Screen Space Camera and bound to that HUD camera immediately. The
existing periodic conversion remains as a fallback for canvases created before
the hook becomes active. This removes the observed one-scan delay. Only Quest
testing can confirm it removes the reported flicker.

The logs also show `GL_INVALID_OPERATION` reports and repeated
`AnimatedUV.Update()` `IndexOutOfRangeException`s. They have not been tied to the
right-eye symptom: the logs do not identify a GL call site or eye, and the
recording does not include isolated left/right eye captures. No unrelated shader
or texture code was changed based on correlation alone.

## Water / falling report

The recording shows Sonic maintaining contact with the water-running route
through multiple frames, then losing support beside the rock face and falling.
The earlier assessment incorrectly described the clip as lacking water-surface
contact. The source's `SonicFast.StateGround()` updates velocity from the game's
forward direction and switches to Air when the grounding ray fails.
`PlayerBase.IsGrounded()` casts down up to `MaxRayLenght - 0.25` against a mask
that includes the `Water` layer, and updates `PlayerEvents.GroundTag` from a hit.
The VR bridge does not write player position, velocity, rigidbody state,
collision masks or `GroundTag`.

The clip establishes a loss of support but cannot distinguish a one/few-frame
ground-ray miss at a water collider seam from a true route exit. Candidate 0.1.7
adds a conservative 80 ms continuity rule only for `SonicFast` immediately
after a confirmed `Water`/`ShoreWater` ground hit. It does not extend ray length,
change masks, teleport the player or suppress a persistent fall. Each use is
counted and logged outside the physics callback so the next Quest run can confirm
whether the route encountered transient ground-ray misses. This tag-limited
fallback is reusable for similar water seam defects without changing land, rail,
slope, spring or other character grounding. Quest retest is required to establish
whether it resolves this exact fall.


## Experimental first person

Candidate 0.1.7 derives a camera-local head anchor from
`PlayerCamera.PlayerBase.CameraTarget`, composes that with the OpenXR eye pose,
hides/restores `PlayerRenderers`, and draws simple white glove-shaped meshes at
left/right Touch grip poses. The option is disabled by default. The visual
changes fail open to third person if the player anchor, renderer array, managed
Unity objects or either controller pose is missing. The gloves are primitive
placeholders, not an authored Sonic model or animated IK. Quest testing must
confirm anchor alignment, pose handedness, scaling, body visibility and that the
render callbacks run on the Unity thread as expected.
