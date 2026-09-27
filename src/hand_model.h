#pragma once
// Model: fingers +Z, palm normal +Y, thumb +X left / -X right.
// Orient the stylized pointing gloves with OpenXR aim (+Z after Unity conversion),
// while retaining grip position. Grip -Z is NOT the user's fingertip direction.
// Model alignment is identity; VROptions adds mirrored ergonomic pitch/yaw/roll
// calibration without changing the controller poses used for gestures.
constexpr float P06GloveFromAim[4]={0,0,0,1};
