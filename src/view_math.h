#pragma once
#include "vr_options.h"
#include <openxr/openxr.h>
#include "IUnityXRDisplay.h"
#include <cmath>

inline UnityXRPose P06EyePose(const XrView* views,int eye,const VROptions& options){
    const auto& p=views[eye].pose;const auto& other=views[1-eye].pose;
    UnityXRPose result{};
    if(options.mode==ViewMode::Immersive){
        result.rotation={-p.orientation.x,-p.orientation.y,p.orientation.z,p.orientation.w};
        if(options.positionalTracking)result.position={p.position.x/options.worldScale,p.position.y/options.worldScale,-p.position.z/options.worldScale};
        else result.position={(p.position.x-other.position.x)*.5f/options.worldScale,(p.position.y-other.position.y)*.5f/options.worldScale,-(p.position.z-other.position.z)*.5f/options.worldScale};
    }else{
        result.rotation.w=1;
        const float x=p.position.x-other.position.x,y=p.position.y-other.position.y,z=p.position.z-other.position.z;
        const float halfIPD=.5f*std::sqrt(x*x+y*y+z*z);
        result.position.x=options.mode==ViewMode::Theatre?0.f:(eye==0?-halfIPD:halfIPD)*options.stereoStrength;
    }
    return result;
}
inline UnityXRProjection P06Projection(const XrFovf& fov,const VROptions& options){
    UnityXRProjection result{};result.type=kUnityXRProjectionTypeHalfAngles;
    if(options.mode==ViewMode::Immersive)result.data.halfAngles={std::tan(fov.angleLeft),std::tan(fov.angleRight),std::tan(fov.angleUp),std::tan(fov.angleDown)};
    else {constexpr float vertical=.52056705f;result.data.halfAngles={-vertical*16.f/9.f,vertical*16.f/9.f,vertical,-vertical};}
    return result;
}
