#pragma once
// Model: fingers +Z, palm normal +Y, thumb +X left / -X right.
// Orient the stylized pointing gloves with OpenXR aim (+Z after Unity conversion),
// while retaining grip position. Grip -Z is NOT the user's fingertip direction.
// Both hands therefore share identity aim rotation; thumb offsets are mirrored.
constexpr float P06GloveFromAim[4]={0,0,0,1};
