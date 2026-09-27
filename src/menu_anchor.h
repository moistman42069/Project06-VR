#pragma once
#include <openxr/openxr.h>
#include <cmath>
#include <cstdint>

struct P06MenuAnchor {
    XrPosef head{{0,0,0,1},{0,0,0}};
    bool valid=false,wasOpen=false,wasFollowing=false;
    uint64_t origin=0;
    bool update(bool open,bool follow,uint64_t nextOrigin,const XrPosef* located){
        if(!open){valid=wasOpen=false;return false;}
        if(!wasOpen||origin!=nextOrigin||(wasFollowing&&!follow))valid=false;
        wasOpen=true;wasFollowing=follow;origin=nextOrigin;
        if((!valid||follow)&&located){
            const auto& p=located->position;const auto& q=located->orientation;
            float norm=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
            if(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)&&std::isfinite(norm)&&norm>.5f&&norm<1.5f){
                head=*located;float scale=1.f/std::sqrt(norm);head.orientation={q.x*scale,q.y*scale,q.z*scale,q.w*scale};valid=true;
            }
        }
        return valid;
    }
    XrPosef pose(float distance)const{
        auto result=head;const auto& q=head.orientation;
        // Rotate the OpenXR forward direction (0,0,-distance) into the captured space.
        result.position.x-=distance*2.f*(q.x*q.z+q.w*q.y);
        result.position.y-=distance*2.f*(q.y*q.z-q.w*q.x);
        result.position.z-=distance*(1.f-2.f*(q.x*q.x+q.y*q.y));
        return result;
    }
};
