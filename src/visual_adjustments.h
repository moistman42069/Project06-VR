#pragma once
#include <cmath>
// Local yaw * pitch * roll; zero offsets preserve the accepted aim orientation.
inline void P06HandAdjustment(const float degrees[3],float q[4]){
    constexpr float halfRadians=.008726646259971648f;
    float sx=std::sin(degrees[0]*halfRadians),cx=std::cos(degrees[0]*halfRadians);
    float sy=std::sin(degrees[1]*halfRadians),cy=std::cos(degrees[1]*halfRadians);
    float sz=std::sin(degrees[2]*halfRadians),cz=std::cos(degrees[2]*halfRadians);
    q[0]=cy*sx*cz+sy*cx*sz;q[1]=sy*cx*cz-cy*sx*sz;
    q[2]=cy*cx*sz-sy*sx*cz;q[3]=cy*cx*cz+sy*sx*sz;
}

// Unity-handed tracked head pose to camera-local HUD center. Match eye translation
// scaling and rotation-only tracking; UI distance/offsets retain their existing units.
inline void P06HUDFromHead(const float offset[3],const float position[3],const float q[4],float worldScale,bool positional,float out[3]){
    const float tx=2*(q[1]*offset[2]-q[2]*offset[1]),ty=2*(q[2]*offset[0]-q[0]*offset[2]),tz=2*(q[0]*offset[1]-q[1]*offset[0]);
    out[0]=offset[0]+q[3]*tx+q[1]*tz-q[2]*ty;
    out[1]=offset[1]+q[3]*ty+q[2]*tx-q[0]*tz;
    out[2]=offset[2]+q[3]*tz+q[0]*ty-q[1]*tx;
    if(positional)for(int i=0;i<3;++i)out[i]+=position[i]/worldScale;
}

struct P06HUDVisibility {
    bool hidden=false,savedEnabled=true;
    bool update(bool hide,bool current){
        if(hide){if(!hidden)savedEnabled=current;hidden=true;return false;}
        if(hidden){hidden=false;return savedEnabled;}
        return current;
    }
};
