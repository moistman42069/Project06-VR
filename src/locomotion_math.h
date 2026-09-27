#pragma once
#include <cmath>
// Convert a head-relative stick into the yaw basis consumed by RotatePlayer.
inline void P06RemapStick(float& x,float& y,float headYaw,float nativeYaw){
    const float delta=headYaw-nativeYaw,c=std::cos(delta),s=std::sin(delta);
    const float oldX=x;x=c*x+s*y;y=c*y-s*oldX;
}
