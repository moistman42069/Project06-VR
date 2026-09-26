# Quest loading-screen failure: EGL binding

## Evidence from the 0.1.3 Quest run

The supplied 2026-09-26 log records successful native plugin registration and
`xrCreateInstance` on the Oculus runtime. The next provider event is
`Unity EGLConfig unavailable`, followed by `XR graphics startup FAILED`. The
Unity integrated subsystem still reported `IsRunning = true`, so the previous
bridge mislabeled a failed display initialization as a running XR display.
There is no evidence in this run that an OpenXR session or first compositor
frame was created: graphics startup returned before either was possible.

That message was emitted by one compound branch in the old source:
`eglQueryContext(... EGL_CONFIG_ID ...)` had its return value ignored, and its
output was passed directly into a one-result `eglChooseConfig`. The existing
log cannot distinguish whether the query failed, returned config ID zero, or
the chooser failed. It therefore proves the failing stage, but not which
sub-call caused it.

## Corrected EGL binding

The replacement gets the context's `EGL_CONFIG_ID`, checks the return value,
and uses the current draw or read surface's config ID when the context has no
associated config. EGL's `EGL_KHR_no_config_context` permits a context config ID
of zero; EGL specifies that `eglQuerySurface(EGL_CONFIG_ID)` reports the config
used to create the surface. If Unity has a surfaceless context, the provider
selects an enumerated RGBA8 GLES3-compatible EGL config. It resolves IDs against
the display's actual configs with `eglGetConfigs`/`eglGetConfigAttrib`, then
passes the resolved handle with Unity's existing display and context to OpenXR.

The new diagnostics log both query results, IDs, EGL errors, the selected
config, and every later startup boundary. `XR graphics initialized` is now
distinct from Unity's integrated-subsystem running bit. The game bridge marks
XR active only when both are true and does not repeatedly create/start the
same subsystem if graphics startup failed.

## Cross-checks and limits

This path was cross-checked against the Khronos EGL/OpenXR specifications,
Meta's Android OpenXR instance/session guide and hello_xr flow, and Unity's
native XR display-provider lifecycle. Unity's provider contract requires
`PopulateNextFrameDesc` to give Unity its target textures and
`SubmitCurrentFrame` to hand completed rendering to the compositor. OpenXR
requires the running frame loop to call `xrWaitFrame`, `xrBeginFrame`, and
`xrEndFrame`; a successful `xrEndFrame` with a nonempty layer list is the first
reliable provider-side evidence that a rendered layer reached the runtime.

The 0.1.4 source logs that first successful layer submission. It cannot prove
the pixels are visible or that Quest's system loading view cleared without a
Quest 3 runtime test. This workspace runs under Shadow PC and has no Quest
transport. The candidate is not accepted or released until the user's headset
test confirms visibility, head tracking, controller input, menu behavior, and
recenter. If the loading view remains, the new stage-specific log indicates
whether failure occurred during EGL resolution, OpenXR session/swapchain
creation, session-state transition, frame rendering, or layer submission.

## References

- [Khronos OpenXR Android GLES binding](https://registry.khronos.org/OpenXR/specs/1.0/man/html/XrGraphicsBindingOpenGLESAndroidKHR.html)
- [Khronos EGL specification 1.2: querying EGL surface configuration](https://registry.khronos.org/EGL/specs/eglspec.1.2.pdf)
- [Khronos EGL specification 1.0: config IDs](https://registry.khronos.org/EGL/specs/eglspec.1.0.pdf)
- [Meta: creating OpenXR instances and sessions on Android](https://developers.meta.com/horizon/documentation/native/android/mobile-openxr-instance-session/)
- [Unity: XR SDK display subsystem and frame lifecycle](https://docs.unity.cn/Manual/xrsdk-display.html)
- [Google Cardboard native Unity XR display-provider sample](https://github.com/googlevr/cardboard/blob/master/sdk/unity/xr_provider/display.cc)
- [Khronos OpenXR frame-submission guide](https://github.com/KhronosGroup/OpenXR-Guide/blob/main/chapters/frame_submission.md)
