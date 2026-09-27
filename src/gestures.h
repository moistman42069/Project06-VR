#pragma once
#include "vr_options.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

struct MotionPose { bool valid=false;float p[3]{},q[4]{0,0,0,1}; };
struct MotionFrame { MotionPose head,hand[2],aim[2];double time=0;uint64_t origin=0;bool focused=false; };
struct GestureContext {
    bool allowed=false,ground=false,charge=false,homing=false,manual=false;
    bool spinSupported=true;
    uintptr_t player=0,target=0;bool pointing[2]{};
};
struct GestureOutput { float run=0;bool homing=false,charge=false,release=false,cancel=false; };
// All distances are tracking metres and all derivatives use monotonic seconds.
// Game physics, collision, target selection and attack execution remain native.
class GestureEngine {
    MotionFrame last{};bool have=false,owned=false,armedStanding=false;
    float standing=0,height=0,run=0;double crouchedAt=0,cooldown=0,chargeAt=0;
    int pointed=-1;uintptr_t target=0,player=0;float extension=0;
    double pointAt=0;bool pointArmed=false;
    int swingSign[2]{};double lastSwing[2]{};
    float swingEnvelope[2]{};double lastMotion=0;
    static float distance(const MotionPose& a,const MotionPose& b){float v=0;for(int i=0;i<3;++i)v+=(a.p[i]-b.p[i])*(a.p[i]-b.p[i]);return std::sqrt(v);}
public:
    GestureOutput step(const MotionFrame& f,const GestureContext& c,const VROptions& o,double now){
        GestureOutput out;
        bool valid=f.focused&&f.head.valid&&c.allowed&&f.time>0&&now-f.time>=-.04&&now-f.time<.15;
        bool changed=!have||f.origin!=last.origin||c.player!=player;
        double dt=f.time-last.time;
        if(changed&&f.head.valid){standing=height=f.head.p[1];armedStanding=false;}
        if(valid&&!changed&&dt==0&&!c.manual&&(!owned||o.crouchSpin)){
            out.run=o.motionRun?run:0;out.charge=owned;return out;
        }
        if(!valid||changed||dt<=0||dt>.1){
            out.cancel=owned;owned=false;run=0;crouchedAt=0;pointed=-1;pointArmed=false;
            swingSign[0]=swingSign[1]=0;lastSwing[0]=lastSwing[1]=0;
            swingEnvelope[0]=swingEnvelope[1]=0;lastMotion=0;
            last=f;have=valid;player=c.player;return out;
        }
        height+=(f.head.p[1]-height)*(1.f-std::exp(-float(dt)*30.f));
        const float drop=standing-height;
        if(drop<o.crouchDepth*.4f)armedStanding=true;
        if(o.motionRun&&!c.manual&&!owned&&f.hand[0].valid&&f.hand[1].valid&&last.hand[0].valid&&last.hand[1].valid){
            float v[2][3]{};float speeds[2]{};float dot=0;
            for(int h=0;h<2;++h)for(int j=0;j<3;++j){v[h][j]=float(((f.hand[h].p[j]-f.head.p[j])-(last.hand[h].p[j]-last.head.p[j]))/dt);speeds[h]+=v[h][j]*v[h][j];}
            for(int j=0;j<3;++j)dot+=v[0][j]*v[1][j];
            float a=std::sqrt(speeds[0]),b=std::sqrt(speeds[1]);
            float forward[3]={2*(f.head.q[0]*f.head.q[2]+f.head.q[3]*f.head.q[1]),0,1-2*(f.head.q[0]*f.head.q[0]+f.head.q[1]*f.head.q[1])};
            float sagittal[2];for(int h=0;h<2;++h){float along=v[h][0]*forward[0]+v[h][2]*forward[2];float dominant=std::fabs(along)>std::fabs(v[h][1])?along:v[h][1];sagittal[h]=std::sqrt(along*along+v[h][1]*v[h][1]);
                if(std::fabs(dominant)>.2f){int sign=dominant>0?1:-1;if(swingSign[h]&&swingSign[h]!=sign)lastSwing[h]=now;swingSign[h]=sign;}}
            bool alternating=lastSwing[0]>0&&lastSwing[1]>0&&now-lastSwing[0]<.8&&now-lastSwing[1]<.8;
            bool stroke=a<8&&b<8&&sagittal[0]>.55f*a&&sagittal[1]>.55f*b&&dot<-.1f*a*b;
            for(int h=0;h<2;++h){
                // A peak envelope carries speed through natural zero-velocity turnarounds.
                swingEnvelope[h]*=std::exp(-float(dt)*1.5f);
                if(stroke)swingEnvelope[h]=std::max(swingEnvelope[h],sagittal[h]);
            }
            if(stroke&&std::min(sagittal[0],sagittal[1])>.2f)lastMotion=now;
            float wanted=alternating&&now-lastMotion<.3?std::clamp((std::min(swingEnvelope[0],swingEnvelope[1])-.12f)*o.runSensitivity,0.f,1.f):0;
            // Dampen speed selection, while leaving native ground acceleration intact.
            float rate=wanted>run?o.runAcceleration:2.5f;
            run+=std::clamp(wanted-run,-rate*float(dt),rate*float(dt));out.run=run;
        }else run=0;
        if(o.gestureHoming&&c.target&&now>=cooldown){
            if(pointed<0||target!=c.target){pointed=-1;pointArmed=false;for(int h=0;h<2;++h)if(f.hand[h].valid&&f.aim[h].valid&&c.pointing[h]&&distance(f.hand[h],f.head)>.35f){pointed=h;target=c.target;pointAt=now;extension=distance(f.hand[h],f.head);break;}}
            if(pointed>=0){int h=pointed;float reach=distance(f.hand[h],f.head);
                if(!f.hand[h].valid||!f.aim[h].valid||now-pointAt>1.5){pointed=-1;pointArmed=false;}
                else {if(c.pointing[h]&&now-pointAt>=.12){pointArmed=true;extension=std::max(extension,reach);}
                    float inward=float((distance(last.hand[h],last.head)-reach)/dt);
                    if(!c.manual&&c.homing&&pointArmed&&extension-reach>=o.homingPull&&inward>.2f&&inward<4.f){out.homing=true;cooldown=now+.6;pointed=-1;pointArmed=false;}}
            }
        }else{pointed=-1;pointArmed=false;}
        if(owned){
            if(!o.crouchSpin||c.manual||(!c.charge&&now-chargeAt>.3)){out.cancel=true;owned=false;}
            else if(drop<o.crouchDepth*.45f&&c.charge&&now-chargeAt>.2){out.release=true;owned=false;armedStanding=false;cooldown=now+.5;}
            else out.charge=true;
        }else if(o.crouchSpin&&c.spinSupported&&c.ground&&!c.manual&&armedStanding&&now>=cooldown&&drop>=o.crouchDepth){
            if(crouchedAt==0)crouchedAt=now;
            if(now-crouchedAt>=.15){owned=true;chargeAt=now;out.charge=true;run=0;out.run=0;}
        }else crouchedAt=0;
        last=f;player=c.player;return out;
    }
};
