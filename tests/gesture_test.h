#pragma once
#include "gestures.h"
#include "trigger_sweep.h"
#include "haptics.h"
#include <cassert>
inline void testGestures(){
    // Replay a smooth running cycle, including zero-velocity reversals and a
    // modest phase mismatch. Full sensitivity must sustain native top speed.
    for(int hz:{72,90,120}){
        GestureEngine gait;VROptions opt;opt.motionRun=true;opt.runSensitivity=3;
        GestureContext ctx;ctx.allowed=ctx.ground=true;ctx.player=7;
        MotionFrame motion;motion.focused=motion.head.valid=true;motion.head.p[1]=1.6f;
        for(auto& hand:motion.hand)hand.valid=true;
        float minimum=1;GestureOutput result;
        for(int i=0;i<hz*5;++i){double t=double(i)/hz;motion.time=10+t;
            for(int h=0;h<2;++h){motion.hand[h].p[0]=h?.2f:-.2f;motion.hand[h].p[1]=1.2f;
                motion.hand[h].p[2]=.15f*std::sin(float(t*1.6*6.2831853)+(h?3.35f:0));}
            result=gait.step(motion,ctx,opt,motion.time);
            if(i>hz*3)minimum=std::min(minimum,result.run);
        }
        assert(minimum>.9f);
        for(int i=0;i<hz;++i){motion.time+=1.0/hz;result=gait.step(motion,ctx,opt,motion.time);}
        assert(result.run==0); // Releasing both arms stops input; no stuck cruise.
    }
    VROptions o;o.motionRun=o.gestureHoming=o.crouchSpin=true;
    MotionFrame f;f.focused=true;f.head.valid=true;f.origin=1;f.time=1;
    f.head.p[1]=1.6f;for(int h=0;h<2;++h){f.hand[h].valid=f.aim[h].valid=true;f.hand[h].p[0]=h?.2f:-.2f;f.hand[h].p[1]=1.3f;f.hand[h].p[2]=.1f;}
    GestureContext c;c.allowed=c.ground=true;c.player=1;GestureEngine engine;
    auto step=[&](double dt=.02){f.time+=dt;return engine.step(f,c,o,f.time);};
    step();step();
    // Equal whole-body translation must not count as a running swing.
    for(int i=0;i<10;++i){f.head.p[2]+=.02f;for(auto& h:f.hand)h.p[2]+=.02f;assert(step().run==0);}
    // One hand only cannot trigger two-hand running.
    for(int i=0;i<10;++i){f.hand[0].p[1]+=.015f;assert(step().run==0);}
    for(int i=0;i<30;++i){float delta=(i/8)%2?.02f:-.02f;f.hand[0].p[1]+=delta;f.hand[1].p[1]-=delta;step();}
    auto r=step();assert(r.run>0&&r.run<=1);
    f.focused=false;r=step();assert(r.run==0&&!r.homing&&!r.charge);f.focused=true;step();step();
    // Crouch dwell, native charge, repeated render samples, stand/release.
    f.head.p[1]-=.4f;for(int i=0;i<15;++i)r=step();assert(r.charge);
    c.ground=false;c.charge=true;r=step();assert(r.charge);
    r=engine.step(f,c,o,f.time+.001);assert(r.charge&&!r.cancel);
    for(int i=0;i<15;++i)step();f.head.p[1]+=.4f;for(int i=0;i<8;++i){r=step();if(r.release)break;}assert(r.release&&!r.charge);
    r=step();assert(!r.release);
    // Turning off a gesture or losing focus cancels a held charge; never launches.
    c.ground=true;c.charge=false;for(int i=0;i<40;++i)step();f.head.p[1]-=.4f;for(int i=0;i<15;++i)r=step();assert(r.charge);
    c.charge=true;c.ground=false;f.focused=false;r=step();assert(r.cancel&&!r.release&&!r.charge);
    f.focused=true;f.head.p[1]=1.6f;step();step();c.charge=false;
    ++f.origin;r=step();assert(r.run==0&&!r.homing&&!r.release);
    r=engine.step(f,c,o,f.time+1);assert(r.run==0&&!r.homing); // Stale tracking rejected.
    // Both hands independently support outward strokes. Replay at headset rates;
    // reject inward pull, whole-body translation, head-only motion and pose jumps.
    for(int hz:{72,90,120})for(int hand=0;hand<2;++hand)for(int scenario=0;scenario<9;++scenario){
        GestureEngine swing;VROptions opt;opt.gestureHoming=true;
        GestureContext ctx;ctx.allowed=ctx.homing=true;ctx.player=4;ctx.target=123;
        if(scenario==4)ctx.target=0;if(scenario==5)ctx.manual=true;if(scenario==6)ctx.homing=false;
        MotionFrame motion;motion.time=20;motion.focused=motion.head.valid=true;motion.head.p[1]=1.6f;
        for(int h=0;h<2;++h){motion.hand[h].valid=true;motion.hand[h].p[0]=h?.15f:-.15f;motion.hand[h].p[1]=1.45f;motion.hand[h].p[2]=scenario==1?.7f:.25f;}
        swing.step(motion,ctx,opt,motion.time);int pulses=0;
        for(int i=0;i<hz/5;++i){motion.time+=1.0/hz;float d=1.6f/hz;
            if(scenario==0||scenario>=4)motion.hand[hand].p[2]+=d;
            if(scenario==1)motion.hand[hand].p[2]-=d;
            if(scenario==2){motion.head.p[2]+=d;for(auto& h:motion.hand)h.p[2]+=d;}
            if(scenario==3)motion.head.p[2]-=d;
            if(scenario==7)motion.hand[hand].valid=i<3;
            if(scenario==8)motion.hand[hand].p[2]+=1; // Discontinuous tracking.
            auto result=swing.step(motion,ctx,opt,motion.time);pulses+=result.homing;
            auto duplicate=swing.step(motion,ctx,opt,motion.time+.0001);assert(!duplicate.homing);
        }
        assert(pulses==(scenario==0?1:0));
        // Pose reacquisition/origin replacement cannot complete an old stroke.
        ++motion.origin;motion.hand[hand].valid=true;motion.time+=1.0/hz;
        assert(!swing.step(motion,ctx,opt,motion.time).homing);
    }
    // An unsupported character must never receive a synthetic spin button.
    GestureEngine noSpin;c={};c.allowed=c.ground=true;c.player=4;c.spinSupported=false;
    f.time=50;f.head.p[1]=1.6f;noSpin.step(f,c,o,f.time);f.time+=.02;noSpin.step(f,c,o,f.time);
    f.head.p[1]=1.1f;for(int i=0;i<40;++i){f.time+=.02;auto v=noSpin.step(f,c,o,f.time);assert(!v.charge&&!v.release);}
    float a[3]={-2,0,0},b[3]={2,0,0},center[3]{},size[3]={1,1,1};
    assert(SegmentSphere(a,b,center,.5f));assert(SegmentBox(a,b,center,size));
    b[0]=0;assert(!SegmentSphere(a,b,center,.5f));assert(!SegmentBox(a,b,center,size)); // Native endpoint overlap owns this.
    b[0]=2;a[1]=b[1]=2;assert(!SegmentSphere(a,b,center,.5f));assert(!SegmentBox(a,b,center,size));
    a[1]=b[1]=0;a[0]=NAN;assert(!SegmentSphere(a,b,center,.5f));assert(!SegmentBox(a,b,center,size));
    assert(ActionHaptic("Hurt").amplitude>ActionHaptic("Jump").amplitude);
    assert(ActionHaptic("Air").amplitude==0);assert(ClampHaptic(NAN,1,1).amplitude==0);
    assert(ClampHaptic(2,1,1).amplitude==1&&ClampHaptic(2,1,1).seconds==.3f);
    assert(ClampHaptic(.8f,.1f,0).amplitude==0);
}
