#include "vr_options.h"
#include "view_math.h"
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <vector>
#include <iostream>

int main(){
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
    c.lx=1;out=UpdateVRMenu(c,3.2);assert(out.lx==0);assert(GetOptions().mode==ViewMode::StereoScreen);
    c.lx=0;UpdateVRMenu(c,3.3);c.buttons=A;UpdateVRMenu(c,3.4);assert(GetOptions().mode==ViewMode::Theatre);
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
    InitOptions("out/menu-test-state");assert(GetOptions().mode==ViewMode::Theatre); // Persistent settings.
    std::vector<uint32_t> pixels(1024*1024);RasterMenu(pixels.data(),1024,1024);
    FILE* file=fopen("out/vr-menu.ppm","wb");assert(file);fprintf(file,"P6\n1024 1024\n255\n");
    for(auto px:pixels){unsigned char rgb[]={static_cast<unsigned char>(px),static_cast<unsigned char>(px>>8),static_cast<unsigned char>(px>>16)};fwrite(rgb,1,3,file);}fclose(file);
    std::cout<<"PASS: eye handedness/IPD, world scale, rotation-only tracking, stereo/mono screen poses, FOV signs, chord debounce, staggered clicks, mode selection, input capture, focus loss, persistence, menu raster\n";
}
