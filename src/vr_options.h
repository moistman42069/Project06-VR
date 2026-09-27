#pragma once
#include "controls.h"
#include <cstdint>
#include <string>
enum class ViewMode { Immersive=0,StereoScreen=1,Theatre=2 };
struct VROptions {
    ViewMode mode=ViewMode::Immersive;
    float renderScale=.7f,worldScale=1.f,screenDistance=1.f,screenWidth=3.f,stereoStrength=1.f,hudDistance=2.f;
    bool positionalTracking=true,firstPerson=false;
    bool motionRun=false,gestureHoming=false,crouchSpin=false;
    float eyeHeight=.85f,runSensitivity=1.f,runAcceleration=2.f,homingTravel=.10f,crouchDepth=.3f;
    bool haptics=true;
    float hapticStrength=.7f;
    float hudX=-.4f,hudY=-.3f,hudSize=.2f;
    float hudWidth=1.f,titleWidth=1.f;
    bool hideHUD=false;
    float titleSize=.15f,titleDistance=3.f,titleX=0,titleY=0;
    float vrMenuSize=1.05f,vrMenuDistance=1.15f;
    bool menuFollowView=false,hudFollowView=false;
    float handAngles[2][3]{{-10,-5,30},{-10,5,-30}}; // Local visual pitch, yaw, roll in degrees; tracking stays unchanged.
};
inline bool ImmersiveInteractions(const VROptions& o){return o.mode==ViewMode::Immersive&&o.firstPerson&&o.motionRun&&o.gestureHoming&&o.crouchSpin&&o.positionalTracking&&o.hudFollowView;}
void InitOptions(const char* directory);
VROptions GetOptions();
std::string OptionsSnapshot();
Controls UpdateVRMenu(const Controls& input,double now);
bool VRMenuOpen();
bool ConsumeRecenter();
uint64_t MenuRevision();
void RasterMenu(uint32_t* rgba,int width,int height);
