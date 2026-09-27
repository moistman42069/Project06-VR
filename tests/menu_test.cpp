#include "vr_options.h"
#include "view_math.h"
#include "gesture_test.h"
#include "hand_model.h"
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <vector>
#include <iostream>

int main(){
    testGestures();
    for(int hand=0;hand<2;++hand){float zero[3]{};UnityXRPose glove{};glove.rotation.w=1;glove.position={hand==0?1.f:-1.f,0,0};P06ApplyFirstPersonAnchor(glove,zero,P06GloveFromGrip[hand],1);
        assert(std::fabs(glove.position.x)<.0001f&&std::fabs(glove.position.y)<.0001f&&std::fabs(glove.position.z-1)<.0001f);}
    XrView views[2]={{XR_TYPE_VIEW},{XR_TYPE_VIEW}};views[0].pose.orientation.w=views[1].pose.orientation.w=1;
    views[0].pose.position={.968f,2.f,-3.f};views[1].pose.position={1.032f,2.f,-3.f};
    VROptions settings;auto left=P06EyePose(views,0,settings),right=P06EyePose(views,1,settings);
    assert(std::fabs((right.position.x-left.position.x)-.064f)<.00001f);assert(left.position.z==3.f);
    settings.worldScale=2;left=P06EyePose(views,0,settings);assert(std::fabs(left.position.x-.484f)<.00001f);
    settings.positionalTracking=false;left=P06EyePose(views,0,settings);assert(std::fabs(left.position.x+.016f)<.00001f);assert(left.position.y==0&&left.position.z==0);
    settings.mode=ViewMode::StereoScreen;left=P06EyePose(views,0,settings);assert(std::fabs(left.position.x+.032f)<.00001f);assert(left.rotation.w==1);
    settings.mode=ViewMode::Theatre;left=P06EyePose(views,0,settings);right=P06EyePose(views,1,settings);assert(left.position.x==0&&right.position.x==0);
    XrFovf fov{-.8f,.9f,.7f,-.6f};settings.mode=ViewMode::Immersive;auto projection=P06Projection(fov,settings);
    assert(projection.data.halfAngles.left<0&&projection.data.halfAngles.right>0&&projection.data.halfAngles.top>0&&projection.data.halfAngles.bottom<0);
    UnityXRPose fpPose{};fpPose.rotation.w=1;fpPose.position={.1f,-.2f,.3f};float anchorPos[3]={2.f,4.f,6.f},anchorRot[4]={0,0,0,1};P06ApplyFirstPersonAnchor(fpPose,anchorPos,anchorRot,2.f);
    assert(std::fabs(fpPose.position.x-2.1f)<.00001f&&std::fabs(fpPose.position.y-3.8f)<.00001f&&std::fabs(fpPose.position.z-6.3f)<.00001f);assert(fpPose.rotation.w==1.f);
    fpPose={};fpPose.rotation.w=1;fpPose.position={1.f,0.f,0.f};anchorRot[1]=.70710678f;anchorRot[3]=.70710678f;P06ApplyFirstPersonAnchor(fpPose,anchorPos,anchorRot,1.f);
    assert(std::fabs(fpPose.position.z-5.f)<.0001f&&std::fabs(fpPose.rotation.y-anchorRot[1])<.0001f);
    std::filesystem::create_directories("out/menu-test-state");
    std::filesystem::remove("out/menu-test-state/vr-settings.txt");
    InitOptions("out/menu-test-state");
    Controls c;c.focused=true;
    assert(GetOptions().mode==ViewMode::Immersive);assert(!VRMenuOpen());
    UpdateVRMenu(c,1);
    c.buttons=LClick;auto out=UpdateVRMenu(c,2);assert(!(out.buttons&LClick));
    c.buttons=LClick|RClick;out=UpdateVRMenu(c,2.04);assert(VRMenuOpen());assert(out.buttons==0);
    UpdateVRMenu(c,3);assert(VRMenuOpen()); // Held chord does not retrigger.
    c.buttons=0;UpdateVRMenu(c,3.1);
    c.buttons=Y;out=UpdateVRMenu(c,3.2);assert(out.blocked);assert(GetOptions().mode==ViewMode::Immersive);
    c.buttons=0;UpdateVRMenu(c,3.25);c.lx=1;out=UpdateVRMenu(c,3.3);assert(out.lx==0);assert(GetOptions().mode==ViewMode::StereoScreen);
    c.lx=0;UpdateVRMenu(c,3.35);c.buttons=A;UpdateVRMenu(c,3.4);assert(GetOptions().mode==ViewMode::Theatre);
    c.buttons=0;UpdateVRMenu(c,3.5);c.buttons=B;out=UpdateVRMenu(c,3.6);assert(!VRMenuOpen());assert(out.buttons==0);
    out=UpdateVRMenu(c,3.7);assert(out.buttons==0); // Closing input never reaches gameplay.
    c.buttons=0;UpdateVRMenu(c,3.8);c.buttons=A;out=UpdateVRMenu(c,3.9);assert(out.buttons==A);
    c.buttons=0;UpdateVRMenu(c,4);c.buttons=LClick|RClick;UpdateVRMenu(c,4.1);assert(VRMenuOpen());
    c.focused=false;UpdateVRMenu(c,4.2);c.focused=true;UpdateVRMenu(c,4.3);assert(VRMenuOpen()); // No phantom chord on refocus.
    c.buttons=0;UpdateVRMenu(c,4.4);c.buttons=LClick|RClick;UpdateVRMenu(c,4.5);assert(!VRMenuOpen());
    c.buttons=0;UpdateVRMenu(c,4.6);
    c.buttons=LClick;out=UpdateVRMenu(c,5);assert(!(out.buttons&LClick));
    c.buttons=0;out=UpdateVRMenu(c,5.05);assert(out.buttons&LClick); // Short Back tap survives chord delay.
    out=UpdateVRMenu(c,5.07);assert(!(out.buttons&LClick));
    c.buttons=LClick;UpdateVRMenu(c,6);out=UpdateVRMenu(c,6.2);assert(out.buttons&LClick);
    c.buttons=0;out=UpdateVRMenu(c,6.3);assert(!(out.buttons&LClick));
    assert(ConsumeRecenter());assert(!ConsumeRecenter());
    c.buttons=0;UpdateVRMenu(c,6.35);c.buttons=LClick|RClick;UpdateVRMenu(c,6.4);assert(VRMenuOpen());c.buttons=0;UpdateVRMenu(c,6.5);
    c.ly=-1;UpdateVRMenu(c,6.7);UpdateVRMenu(c,6.96);UpdateVRMenu(c,7.22);c.ly=0;UpdateVRMenu(c,7.23);c.buttons=A;UpdateVRMenu(c,7.24);assert(GetOptions().firstPerson);
    c.buttons=0;UpdateVRMenu(c,7.3);c.buttons=B;UpdateVRMenu(c,7.4);assert(!VRMenuOpen());
    InitOptions("out/menu-test-state");assert(GetOptions().mode==ViewMode::Theatre&&GetOptions().firstPerson); // Versioned settings survive reload.
    std::vector<uint32_t> pixels(1024*1024);RasterMenu(pixels.data(),1024,1024);
    FILE* file=fopen("out/vr-menu.ppm","wb");assert(file);fprintf(file,"P6\n1024 1024\n255\n");
    for(auto px:pixels){unsigned char rgb[]={static_cast<unsigned char>(px),static_cast<unsigned char>(px>>8),static_cast<unsigned char>(px>>16)};fwrite(rgb,1,3,file);}fclose(file);
    std::cout<<"PASS: eye handedness/IPD, world scale, rotation-only tracking, stereo/mono screen poses, FOV signs, chord debounce, category navigation, staggered clicks, mode selection, input capture, focus loss, persistence, menu raster\n";
}
