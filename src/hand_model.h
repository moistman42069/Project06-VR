#pragma once
// Model: fingers +Z, palm normal +Y, thumb +X left / -X right.
// OpenXR grip converted to Unity: palm normal +X left / -X right,
// little-finger-to-thumb direction +Z. Both wrists extend along -Y.
constexpr float P06GloveFromGrip[2][4]={{-.5f,-.5f,-.5f,.5f},{-.5f,.5f,.5f,.5f}};
