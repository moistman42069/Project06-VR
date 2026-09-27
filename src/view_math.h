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
inline void P06ApplyFirstPersonAnchor(UnityXRPose& pose,const float position[3],const float rotation[4],float /*worldScale*/){
    const float x=pose.position.x,y=pose.position.y,z=pose.position.z;
    const float tx=2.f*(rotation[1]*z-rotation[2]*y),ty=2.f*(rotation[2]*x-rotation[0]*z),tz=2.f*(rotation[0]*y-rotation[1]*x);
    pose.position={position[0]+x+rotation[3]*tx+(rotation[1]*tz-rotation[2]*ty),
        position[1]+y+rotation[3]*ty+(rotation[2]*tx-rotation[0]*tz),
        position[2]+z+rotation[3]*tz+(rotation[0]*ty-rotation[1]*tx)};
    const auto q=pose.rotation;const float ax=rotation[0],ay=rotation[1],az=rotation[2],aw=rotation[3];
    pose.rotation={aw*q.x+ax*q.w+ay*q.z-az*q.y,aw*q.y-ax*q.z+ay*q.w+az*q.x,aw*q.z+ax*q.y-ay*q.x+az*q.w,aw*q.w-ax*q.x-ay*q.y-az*q.z};
}
inline UnityXRProjection P06Projection(const XrFovf& fov,const VROptions& options){
    UnityXRProjection result{};result.type=kUnityXRProjectionTypeHalfAngles;
    if(options.mode==ViewMode::Immersive)result.data.halfAngles={std::tan(fov.angleLeft),std::tan(fov.angleRight),std::tan(fov.angleUp),std::tan(fov.angleDown)};
    else {constexpr float vertical=.52056705f;result.data.halfAngles={-vertical*16.f/9.f,vertical*16.f/9.f,vertical,-vertical};}
    return result;
}
