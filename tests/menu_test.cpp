#include "vr_options.h"
#include "view_math.h"
#include "gesture_test.h"
#include "hand_model.h"
#include "locomotion_math.h"
#include "save_schema.h"
#include "visual_adjustments.h"
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <vector>
#include <iostream>

int main(){
    assert(std::strcmp(kSaveSchemaVersion,"0.1.10-vr-candidate")==0);
    for(int patch=0;patch<=10;++patch){char version[64];snprintf(version,sizeof(version),"0.1.%d-vr-candidate",patch);assert(CompatibleSaveVersion(version));}
    const char* invalidSaveVersions[]={nullptr,"","1.0","0.1.11-vr-candidate","0.1.999-vr-candidate","0.1.9","0.1.9-vr-candidate-extra"};
    for(const char* bad:invalidSaveVersions)assert(!CompatibleSaveVersion(bad));
    testGestures();
    {float angles[3]{},q[4];P06HandAdjustment(angles,q);assert(q[0]==0&&q[1]==0&&q[2]==0&&q[3]==1);
     for(int axis=0;axis<3;++axis){float a[3]{};a[axis]=90;P06HandAdjustment(a,q);assert(std::fabs(q[axis]-.70710678f)<.00001f&&std::fabs(q[3]-.70710678f)<.00001f);}
     for(int pitch=-180;pitch<=180;pitch+=45)for(int yaw=-180;yaw<=180;yaw+=45)for(int roll=-180;roll<=180;roll+=45){float a[3]={float(pitch),float(yaw),float(roll)};P06HandAdjustment(a,q);float norm=0;for(float v:q)norm+=v*v;assert(std::fabs(norm-1)<.00001f);}}
    {float zero[3]{};UnityXRPose glove{};glove.rotation.w=1;glove.position={0,0,1};P06ApplyFirstPersonAnchor(glove,zero,P06GloveFromAim,1);
        assert(glove.position.x==0&&glove.position.y==0&&glove.position.z==1);}
    for(int camera=-180;camera<=180;camera+=45)for(int head=-180;head<=180;head+=45){
        float x=.3f,y=.8f,cy=camera*.017453293f,hy=head*.017453293f;P06RemapStick(x,y,hy,cy);
        assert(std::fabs(x*x+y*y-.73f)<.0001f);
        assert(std::fabs(std::cos(cy)*x+std::sin(cy)*y-(std::cos(hy)*.3f+std::sin(hy)*.8f))<.0001f);
        assert(std::fabs(std::cos(cy)*y-std::sin(cy)*x-(std::cos(hy)*.8f-std::sin(hy)*.3f))<.0001f);
    }
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
    c.ly=-1;out=UpdateVRMenu(c,3.15);assert(out.blocked);c.ly=0;c.buttons=A;UpdateVRMenu(c,3.2);assert(GetOptions().mode==ViewMode::Immersive);
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
    c.buttons=A;UpdateVRMenu(c,6.55);c.buttons=0;UpdateVRMenu(c,6.6); // Enter VR category from root.
    c.ly=-1;UpdateVRMenu(c,6.7);UpdateVRMenu(c,6.96);UpdateVRMenu(c,7.22);c.ly=0;UpdateVRMenu(c,7.23);c.buttons=A;UpdateVRMenu(c,7.24);assert(GetOptions().firstPerson);
    c.buttons=0;UpdateVRMenu(c,7.3);c.buttons=B;UpdateVRMenu(c,7.4);assert(!VRMenuOpen());
    InitOptions("out/menu-test-state");assert(GetOptions().mode==ViewMode::Theatre&&GetOptions().firstPerson); // Versioned settings survive reload.
    std::vector<uint32_t> pixels(1024*1024);RasterMenu(pixels.data(),1024,1024);
    FILE* file=fopen("out/vr-menu.ppm","wb");assert(file);fprintf(file,"P6\n1024 1024\n255\n");
    for(auto px:pixels){unsigned char rgb[]={static_cast<unsigned char>(px),static_cast<unsigned char>(px>>8),static_cast<unsigned char>(px>>16)};fwrite(rgb,1,3,file);}fclose(file);
    c.buttons=0;UpdateVRMenu(c,8);c.buttons=LClick|RClick;UpdateVRMenu(c,8.1);c.buttons=0;UpdateVRMenu(c,8.2);
    c.buttons=A;UpdateVRMenu(c,8.3);c.buttons=0;UpdateVRMenu(c,8.4);c.buttons=X;out=UpdateVRMenu(c,8.5);assert(out.blocked&&VRMenuOpen());
    RasterMenu(pixels.data(),1024,1024);file=fopen("out/vr-menu-root.ppm","wb");assert(file);fprintf(file,"P6\n1024 1024\n255\n");
    for(auto px:pixels){unsigned char rgb[]={static_cast<unsigned char>(px),static_cast<unsigned char>(px>>8),static_cast<unsigned char>(px>>16)};fwrite(rgb,1,3,file);}fclose(file);
    // VR contains all physical controls, including rows below the visible page.
    double now=9;auto update=[&](){now+=.3;return UpdateVRMenu(c,now);};
    auto press=[&](uint32_t button){c.buttons=0;update();c.buttons=button;update();c.buttons=0;update();};
    auto down=[&](int count){c.ly=-1;for(int i=0;i<count;++i)update();c.ly=0;update();};
    press(A);down(6);press(A);assert(GetOptions().motionRun);
    down(3);press(A);assert(GetOptions().gestureHoming);
    down(2);press(A);assert(GetOptions().crouchSpin);
    down(2);press(A);assert(ConsumeRecenter());
    RasterMenu(pixels.data(),1024,1024);file=fopen("out/vr-menu-scrolled.ppm","wb");assert(file);fprintf(file,"P6\n1024 1024\n255\n");
    for(auto px:pixels){unsigned char rgb[]={static_cast<unsigned char>(px),static_cast<unsigned char>(px>>8),static_cast<unsigned char>(px>>16)};fwrite(rgb,1,3,file);}fclose(file);
    press(X);assert(VRMenuOpen());down(1);press(A); // Graphics remains a separate category.
    c.lx=-1;update();c.lx=0;update();assert(GetOptions().renderScale<.7f);
    press(B);assert(!VRMenuOpen());InitOptions("out/menu-test-state");
    assert(GetOptions().motionRun&&GetOptions().gestureHoming&&GetOptions().crouchSpin);
    // Upgrade preserves accepted preferences but replaces obsolete pull distance.
    file=fopen("out/menu-test-state/vr-settings.txt","w");assert(file);
    fprintf(file,"4 0 0.8 1.2 2.5 3 1 2 1 1 0.9 1 1 1 2.3 2.5 0.35 0.4 1 0.6\n");fclose(file);
    InitOptions("out/menu-test-state");auto migrated=GetOptions();
    assert(migrated.firstPerson&&migrated.motionRun&&migrated.gestureHoming&&migrated.crouchSpin);
    assert(migrated.homingTravel==.10f&&migrated.crouchDepth==.4f&&migrated.runSensitivity==2.3f&&migrated.hapticStrength==.6f);
    assert(migrated.hudX==0&&migrated.hudY==0&&migrated.hudSize==1);
    for(const auto& hand:migrated.handAngles)for(float angle:hand)assert(angle==0);
    // Current accepted candidate uses schema 5; retain its tuned homing threshold.
    file=fopen("out/menu-test-state/vr-settings.txt","w");assert(file);
    fprintf(file,"5 0 0.8 1.2 2.5 3 1 2 1 1 0.9 1 1 1 2.3 2.5 0.16 0.4 1 0.6\n");fclose(file);
    InitOptions("out/menu-test-state");migrated=GetOptions();
    assert(migrated.homingTravel==.16f&&migrated.hudX==0&&migrated.hudY==0&&migrated.hudSize==1);
    for(const auto& hand:migrated.handAngles)for(float angle:hand)assert(angle==0);
    // Adjust both HUD axes, depth/size and each hand independently through actual menu input.
    press(LClick|RClick);down(3);press(A); // Root was Graphics; wrap to UI.
    c.lx=1;update();c.lx=0;update();assert(GetOptions().hudDistance==2.25f);
    down(1);press(A);assert(GetOptions().hudX==.05f);
    down(1);press(A);assert(GetOptions().hudY==.05f);
    down(1);press(A);assert(GetOptions().hudSize==1.05f);
    RasterMenu(pixels.data(),1024,1024);file=fopen("out/vr-menu-hud.ppm","wb");assert(file);fprintf(file,"P6\n1024 1024\n255\n");
    for(auto px:pixels){unsigned char rgb[]={static_cast<unsigned char>(px),static_cast<unsigned char>(px>>8),static_cast<unsigned char>(px>>16)};fwrite(rgb,1,3,file);}fclose(file);
    press(X);down(1);press(A);down(14);
    for(int i=0;i<6;++i){press(A);assert(GetOptions().handAngles[i/3][i%3]==5);if(i<5)down(1);}
    RasterMenu(pixels.data(),1024,1024);file=fopen("out/vr-menu-hands.ppm","wb");assert(file);fprintf(file,"P6\n1024 1024\n255\n");
    for(auto px:pixels){unsigned char rgb[]={static_cast<unsigned char>(px),static_cast<unsigned char>(px>>8),static_cast<unsigned char>(px>>16)};fwrite(rgb,1,3,file);}fclose(file);
    InitOptions("out/menu-test-state");auto saved=GetOptions();
    assert(saved.hudX==.05f&&saved.hudY==.05f&&saved.hudSize==1.05f&&saved.hudDistance==2.25f);
    for(auto& hand:saved.handAngles)for(float angle:hand)assert(angle==5);
    down(1);press(A);for(auto& hand:GetOptions().handAngles)for(float angle:hand)assert(angle==0);
    press(X);down(4);press(A);down(4);press(A);assert(GetOptions().hudX==0&&GetOptions().hudY==0&&GetOptions().hudSize==1&&GetOptions().hudDistance==2);
    assert(GetOptions().crouchSpin&&GetOptions().gestureHoming&&GetOptions().firstPerson);press(B);
    std::cout<<"PASS: eye handedness/IPD, world scale, rotation-only tracking, stereo/mono screen poses, FOV signs, chord debounce, category navigation, staggered clicks, mode selection, input capture, focus loss, persistence, menu raster\n";
}
