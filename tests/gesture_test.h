#pragma once
#include "gestures.h"
#include "trigger_sweep.h"
#include "haptics.h"
#include <cassert>
inline void testGestures(){
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
    f.head.p[1]-=.4f;for(int i=0;i<10;++i)r=step();assert(r.charge);
    c.ground=false;c.charge=true;r=step();assert(r.charge);
    r=engine.step(f,c,o,f.time+.001);assert(r.charge&&!r.cancel);
    for(int i=0;i<15;++i)step();f.head.p[1]+=.4f;r=step();assert(r.release&&!r.charge);
    r=step();assert(!r.release);
    // Turning off a gesture or losing focus cancels a held charge; never launches.
    c.ground=true;c.charge=false;for(int i=0;i<40;++i)step();f.head.p[1]-=.4f;for(int i=0;i<10;++i)r=step();assert(r.charge);
    c.charge=true;c.ground=false;f.focused=false;r=step();assert(r.cancel&&!r.release&&!r.charge);
    f.focused=true;f.head.p[1]=1.6f;step();step();c.charge=false;c.homing=true;c.target=123;c.pointing[0]=true;
    f.hand[0].p[0]=-.15f;f.hand[0].p[1]=1.45f;f.hand[0].p[2]=f.head.p[2]+.65f;
    for(int i=0;i<10;++i){r=step();assert(!r.homing);} // Arm extension alone is not an attack.
    for(int i=0;i<6;++i){f.hand[0].p[2]-=.04f;r=step();if(r.homing)break;}assert(r.homing);
    r=step();assert(!r.homing); // One pulse and cooldown.
    c.target=0;for(int i=0;i<60;++i){r=step();assert(!r.homing);} // Never air-dash without native target.
    ++f.origin;r=step();assert(r.run==0&&!r.homing&&!r.release);
    r=engine.step(f,c,o,f.time+1);assert(r.run==0&&!r.homing); // Stale tracking rejected.
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
