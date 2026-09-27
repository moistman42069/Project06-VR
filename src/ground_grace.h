#pragma once
// REJECTED 0.1.7 EXPERIMENT. Retained as historical source only; never include
// in the active bridge. A true return after a failed raycast leaves RaycastHit
// invalid, and SonicFast grounding does not explain SonicNew's WaterSlide.
#include <cmath>

// Bridge only a momentary ground-ray miss after SonicFast was actually
// grounded on a Water/ShoreWater collider. Never synthesize contact on land,
// after a stale/negative clock interval, or after the short seam grace ends.
constexpr double P06_WATER_GROUND_GRACE_SECONDS = 0.08;
inline bool P06KeepWaterGrounded(bool lastContactWasWater, double secondsSinceContact) {
    return lastContactWasWater && std::isfinite(secondsSinceContact) &&
           secondsSinceContact >= 0.0 &&
           secondsSinceContact <= P06_WATER_GROUND_GRACE_SECONDS;
}
