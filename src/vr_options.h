#pragma once
#include "controls.h"
#include <cstdint>
enum class ViewMode { Immersive=0,StereoScreen=1,Theatre=2 };
struct VROptions {
    ViewMode mode=ViewMode::Immersive;
    float renderScale=.7f,worldScale=1.f,screenDistance=2.5f,screenWidth=3.f,stereoStrength=1.f,hudDistance=2.f;
    bool positionalTracking=true,firstPerson=false;
    bool motionRun=false,gestureHoming=false,crouchSpin=false;
    float eyeHeight=.85f,runSensitivity=1.f,runAcceleration=2.f,homingTravel=.10f,crouchDepth=.3f;
    bool haptics=true;
    float hapticStrength=.7f;
    float hudX=0,hudY=0,hudSize=1;
    float titleSize=.65f,titleDistance=3.f,titleX=0,titleY=0;
    float vrMenuSize=1.05f,vrMenuDistance=1.15f;
    bool menuFollowView=false;
    float handAngles[2][3]{{-10,-5,-10},{-10,5,10}}; // Local visual pitch, yaw, roll in degrees; tracking stays unchanged.
};
inline bool ImmersiveInteractions(const VROptions& o){return o.mode==ViewMode::Immersive&&o.firstPerson&&o.motionRun&&o.gestureHoming&&o.crouchSpin&&o.positionalTracking;}
void InitOptions(const char* directory);
VROptions GetOptions();
Controls UpdateVRMenu(const Controls& input,double now);
bool VRMenuOpen();
bool ConsumeRecenter();
uint64_t MenuRevision();
void RasterMenu(uint32_t* rgba,int width,int height);
