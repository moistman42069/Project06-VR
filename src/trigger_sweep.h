#pragma once
#include <algorithm>
#include <cmath>
inline bool SegmentSphere(const float a[3],const float b[3],const float c[3],float radius){
    if(!std::isfinite(radius)||radius<=0)return false;
    float length=0,dot=0,start=0,end=0;
    for(int i=0;i<3;++i){if(!std::isfinite(a[i])||!std::isfinite(b[i])||!std::isfinite(c[i]))return false;float d=b[i]-a[i],v=c[i]-a[i];length+=d*d;dot+=v*d;start+=v*v;end+=(b[i]-c[i])*(b[i]-c[i]);}
    // Recovery is only for a complete crossing missed by discrete overlap detection.
    if(length<1e-8f||start<=radius*radius||end<=radius*radius)return false;
    float t=std::clamp(dot/length,0.f,1.f),distance=0;
    for(int i=0;i<3;++i){float d=a[i]+t*(b[i]-a[i])-c[i];distance+=d*d;}
    return distance<radius*radius;
}
inline bool SegmentBox(const float a[3],const float b[3],const float c[3],const float size[3]){
    float lo=0,hi=1;bool insideA=true,insideB=true;
    for(int i=0;i<3;++i){if(!std::isfinite(a[i])||!std::isfinite(b[i])||!std::isfinite(c[i])||!std::isfinite(size[i])||size[i]<=0)return false;
        float min=c[i]-size[i]*.5f,max=c[i]+size[i]*.5f,d=b[i]-a[i];insideA&=a[i]>=min&&a[i]<=max;insideB&=b[i]>=min&&b[i]<=max;
        if(std::fabs(d)<1e-7f){if(a[i]<min||a[i]>max)return false;continue;}
        float t0=(min-a[i])/d,t1=(max-a[i])/d;if(t0>t1)std::swap(t0,t1);lo=std::max(lo,t0);hi=std::min(hi,t1);if(lo>=hi)return false;
    }
    return !insideA&&!insideB&&lo<hi;
}
