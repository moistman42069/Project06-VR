#pragma once
#include <algorithm>
#include <cmath>
#include <cstring>
struct HapticPulse { float amplitude=0,seconds=0; };
inline HapticPulse ActionHaptic(const char* state){
    if(!state||!*state)return {};
    if(std::strstr(state,"Death")||!std::strcmp(state,"Hurt"))return {.8f,.22f};
    if(std::strstr(state,"Homing")||std::strstr(state,"Attack")||std::strstr(state,"Kick"))return {.55f,.09f};
    if(std::strstr(state,"Jump")||std::strstr(state,"Spring")||std::strstr(state,"Dash"))return {.35f,.065f};
    if(!std::strcmp(state,"Ground"))return {.25f,.045f};
    if(!std::strcmp(state,"Air")||!std::strcmp(state,"Cutscene")||!std::strcmp(state,"Talk"))return {};
    return {.18f,.04f}; // Character-specific action transitions have a light default.
}
inline HapticPulse ClampHaptic(float amplitude,float seconds,float strength){
    if(!std::isfinite(amplitude)||!std::isfinite(seconds)||!std::isfinite(strength)||seconds<=0)return {};
    return {std::clamp(amplitude*strength,0.f,1.f),std::clamp(seconds,.005f,.3f)};
}
