#pragma once
#include "controls.h"
#include <cstdint>
enum class ViewMode { Immersive=0,StereoScreen=1,Theatre=2 };
struct VROptions {
    ViewMode mode=ViewMode::Immersive;
    float renderScale=.7f,worldScale=1.f,screenDistance=2.5f,screenWidth=3.f,stereoStrength=1.f,hudDistance=2.f;
    bool positionalTracking=true;
};
void InitOptions(const char* directory);
VROptions GetOptions();
Controls UpdateVRMenu(const Controls& input,double now);
bool VRMenuOpen();
bool ConsumeRecenter();
uint64_t MenuRevision();
void RasterMenu(uint32_t* rgba,int width,int height);
