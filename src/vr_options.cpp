#include "vr_options.h"
#include "font8x8_basic.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>

namespace {
std::mutex mutex;
VROptions options;
bool open=false,recenterRequested=false,chordHeld=false,swallow=false,chordArmed=true;
uint32_t oldButtons=0;
int category=-1,rootSelected=0,selected=0,oldVertical=0,oldHorizontal=0;
double nextRepeat=0;
double leftClickSince=0;
bool leftClickPending=false;
uint64_t revision=1;
std::string filename;
constexpr int categoryCount=5;
const char* categoryNames[]={"UI","VR","GRAPHICS","HAPTICS","SYSTEM"};
struct MenuItem { const char* label; int option; };
constexpr MenuItem menuItems[][22]={
    {{"HUD DISTANCE",7},{"HUD LEFT / RIGHT",24},{"HUD DOWN / UP",25},{"HUD SIZE",26},{"RESET HUD POSITION / SIZE",27},{"SCREEN DISTANCE",4},{"SCREEN WIDTH",5},{"SCREEN STEREO DEPTH",6},{"TITLE / MENU SIZE",36},{"TITLE / MENU DISTANCE",37},{"TITLE / MENU LEFT / RIGHT",38},{"TITLE / MENU DOWN / UP",39},{"RESET TITLE / MENU",40},{"VR PANEL SIZE",41},{"VR PANEL DISTANCE",42},{"VR PANEL FOLLOWS VIEW",43},{"RESET VR PANEL",44},{"IN-GAME HUD FOLLOWS VIEW",45}},
    {{"IMMERSIVE MODE (ALL)",35},{"DISPLAY MODE",0},{"WORLD SCALE",2},{"HEAD POSITION",3},{"FIRST PERSON",13},{"RECENTER VIEW",11},{"FIRST PERSON EYE HEIGHT",14},
     {"PHYSICAL RUNNING",15},{"RUN SWING SENSITIVITY",16},{"RUN SPEED RAMP",17},{"OUTWARD SWING HOMING",18},{"HOMING SWING DISTANCE",19},
     {"CROUCH SPINDASH",20},{"CROUCH DEPTH",21},{"CALIBRATE STANDING",11},
     {"LEFT HAND PITCH / TILT",28},{"LEFT HAND YAW / ANGLE",29},{"LEFT HAND ROLL / TWIST",30},
     {"RIGHT HAND PITCH / TILT",31},{"RIGHT HAND YAW / ANGLE",32},{"RIGHT HAND ROLL / TWIST",33},{"RESET HAND ROTATIONS",34}},
    {{"RENDER SCALE (RESTART)",1},{"REFRESH RATE",8},{"SHADOWS",9},{"POST EFFECTS",10}},
    {{"GAME HAPTICS",22},{"HAPTIC STRENGTH",23}},
    {{"RESUME GAME",12}}
};
constexpr int categoryRows[]={18,22,4,2,1};
const char* modeName(){switch(options.mode){case ViewMode::StereoScreen:return "3D STEREO SCREEN";case ViewMode::Theatre:return "2D THEATRE";default:return options.firstPerson?"IMMERSIVE FIRST PERSON":"IMMERSIVE THIRD PERSON";}}
void save(){if(filename.empty())return;if(auto f=fopen(filename.c_str(),"w")){
    fprintf(f,"8 %d %.3f %.3f %.3f %.3f %.3f %.3f %d %d %f %d %d %d %f %f %f %f %d %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %d %d\n",int(options.mode),options.renderScale,options.worldScale,options.screenDistance,options.screenWidth,options.stereoStrength,options.hudDistance,int(options.positionalTracking),int(options.firstPerson),options.eyeHeight,int(options.motionRun),int(options.gestureHoming),int(options.crouchSpin),options.runSensitivity,options.runAcceleration,options.homingTravel,options.crouchDepth,int(options.haptics),options.hapticStrength,options.hudX,options.hudY,options.hudSize,options.handAngles[0][0],options.handAngles[0][1],options.handAngles[0][2],options.handAngles[1][0],options.handAngles[1][1],options.handAngles[1][2],options.titleSize,options.titleDistance,options.titleX,options.titleY,options.vrMenuSize,options.vrMenuDistance,int(options.menuFollowView),int(options.hudFollowView));fclose(f);}}
float valid(float v,float lo,float hi,float fallback){return std::isfinite(v)?std::clamp(v,lo,hi):fallback;}
void change(int direction){
    if(category<0){category=rootSelected;selected=0;++revision;return;}
    const int option=menuItems[category][selected].option;
    switch(option){
        case 0:options.mode=ViewMode((int(options.mode)+direction+3)%3);recenterRequested=true;break;
        case 1:options.renderScale=std::clamp(options.renderScale+direction*.1f,.4f,1.f);break;
        case 2:options.worldScale=std::clamp(options.worldScale+direction*.1f,.25f,3.f);break;
        case 3:options.positionalTracking=!options.positionalTracking;break;
        case 4:options.screenDistance=std::clamp(options.screenDistance+direction*.25f,.25f,20.f);break;
        case 5:options.screenWidth=std::clamp(options.screenWidth+direction*.1f,.1f,20.f);break;
        case 6:options.stereoStrength=std::clamp(options.stereoStrength+direction*.1f,0.f,4.f);break;
        case 7:options.hudDistance=std::clamp(options.hudDistance+direction*.25f,.25f,20.f);break;
        case 11:recenterRequested=true;break;
        case 12:open=false;swallow=true;break;
        case 13:options.firstPerson=!options.firstPerson;break;
        case 14:options.eyeHeight=std::clamp(options.eyeHeight+direction*.05f,.4f,1.5f);break;
        case 15:options.motionRun=!options.motionRun;break;
        case 16:options.runSensitivity=std::clamp(options.runSensitivity+direction*.1f,.2f,3.f);break;
        case 17:options.runAcceleration=std::clamp(options.runAcceleration+direction*.25f,.5f,5.f);break;
        case 18:options.gestureHoming=!options.gestureHoming;break;
        case 19:options.homingTravel=std::clamp(options.homingTravel+direction*.02f,.04f,.3f);break;
        case 20:options.crouchSpin=!options.crouchSpin;break;
        case 21:options.crouchDepth=std::clamp(options.crouchDepth+direction*.025f,.15f,.65f);break;
        case 22:options.haptics=!options.haptics;break;
        case 23:options.hapticStrength=std::clamp(options.hapticStrength+direction*.1f,0.f,1.f);break;
        case 24:options.hudX=std::clamp(options.hudX+direction*.05f,-10.f,10.f);break;
        case 25:options.hudY=std::clamp(options.hudY+direction*.05f,-10.f,10.f);break;
        case 26:options.hudSize=std::clamp(options.hudSize+direction*.05f,.05f,2.f);break;
        case 27:options.hudX=options.hudY=0;options.hudSize=1;options.hudDistance=2;break;
        case 28:case 29:case 30:case 31:case 32:case 33:{int index=option-28;auto& angle=options.handAngles[index/3][index%3];angle=std::clamp(angle+direction*5.f,-180.f,180.f);break;}
        case 34:{VROptions defaults;for(int h=0;h<2;++h)std::copy(defaults.handAngles[h],defaults.handAngles[h]+3,options.handAngles[h]);break;}
        case 35:{bool enable=!ImmersiveInteractions(options);options.firstPerson=options.motionRun=options.gestureHoming=options.crouchSpin=options.hudFollowView=enable;if(enable){options.mode=ViewMode::Immersive;options.positionalTracking=true;recenterRequested=true;}break;}
        case 36:options.titleSize=std::clamp(options.titleSize+direction*.05f,.05f,2.f);break;
        case 37:options.titleDistance=std::clamp(options.titleDistance+direction*.25f,.25f,20.f);break;
        case 38:options.titleX=std::clamp(options.titleX+direction*.05f,-10.f,10.f);break;
        case 39:options.titleY=std::clamp(options.titleY+direction*.05f,-10.f,10.f);break;
        case 40:options.titleSize=.65f;options.titleDistance=3;options.titleX=options.titleY=0;break;
        case 41:options.vrMenuSize=std::clamp(options.vrMenuSize+direction*.05f,.1f,3.f);break;
        case 42:options.vrMenuDistance=std::clamp(options.vrMenuDistance+direction*.05f,.25f,10.f);break;
        case 43:options.menuFollowView=!options.menuFollowView;break;
        case 45:options.hudFollowView=!options.hudFollowView;break;
        case 44:options.vrMenuSize=1.05f;options.vrMenuDistance=1.15f;options.menuFollowView=false;break;
        default:return; // Clearly labelled read-only placeholders.
    }++revision;save();
}
uint32_t color(int r,int g,int b){return 0xff000000u|uint32_t(b<<16)|uint32_t(g<<8)|uint32_t(r);}
void rect(uint32_t* p,int w,int h,int x,int y,int rw,int rh,uint32_t c){for(int yy=std::max(0,y);yy<std::min(h,y+rh);++yy)for(int xx=std::max(0,x);xx<std::min(w,x+rw);++xx)p[yy*w+xx]=c;}
void text(uint32_t* p,int w,int h,int x,int y,const char* s,int scale,uint32_t c,bool italic=false){
    for(;*s;++s,x+=8*scale){unsigned ch=static_cast<unsigned char>(*s);if(ch>=128)ch='?';for(int row=0;row<8;++row)for(int col=0;col<8;++col)if(font8x8_basic[ch][row]&(1<<col))rect(p,w,h,x+col*scale+(italic?(7-row)*scale/2:0),y+row*scale,scale,scale,c);}}
}
void InitOptions(const char* directory){std::lock_guard<std::mutex> lock(mutex);filename=std::string(directory)+"/vr-settings.txt";if(auto f=fopen(filename.c_str(),"r")){
    VROptions read;int version=0,mode=0,pos=1,fp=0,mr=0,gh=0,cs=0,haptic=1,follow=0,hudFollow=0;int n=fscanf(f,"%d %d %f %f %f %f %f %f %d %d %f %d %d %d %f %f %f %f %d %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %d %d",&version,&mode,&read.renderScale,&read.worldScale,&read.screenDistance,&read.screenWidth,&read.stereoStrength,&read.hudDistance,&pos,&fp,&read.eyeHeight,&mr,&gh,&cs,&read.runSensitivity,&read.runAcceleration,&read.homingTravel,&read.crouchDepth,&haptic,&read.hapticStrength,&read.hudX,&read.hudY,&read.hudSize,&read.handAngles[0][0],&read.handAngles[0][1],&read.handAngles[0][2],&read.handAngles[1][0],&read.handAngles[1][1],&read.handAngles[1][2],&read.titleSize,&read.titleDistance,&read.titleX,&read.titleY,&read.vrMenuSize,&read.vrMenuDistance,&follow,&hudFollow);fclose(f);
    if((n==9&&version==1)||(n==10&&version==2)||(n==18&&version==3)||(n==20&&(version==4||version==5))||(n==29&&version==6)||(n==36&&version==7)||(n==37&&version==8)){read.mode=ViewMode(std::clamp(mode,0,2));read.renderScale=valid(read.renderScale,.4f,1.f,.7f);read.worldScale=valid(read.worldScale,.25f,3.f,1.f);read.screenDistance=valid(read.screenDistance,.25f,20.f,2.5f);read.screenWidth=valid(read.screenWidth,.1f,20.f,3.f);read.stereoStrength=valid(read.stereoStrength,0.f,4.f,1.f);read.hudDistance=valid(read.hudDistance,.25f,20.f,2.f);read.positionalTracking=pos!=0;read.firstPerson=version>=2&&fp!=0;read.eyeHeight=valid(read.eyeHeight,.4f,1.5f,.85f);read.motionRun=mr!=0;read.gestureHoming=gh!=0;read.crouchSpin=cs!=0;read.runSensitivity=valid(read.runSensitivity,.2f,3.f,1.f);read.runAcceleration=valid(read.runAcceleration,.5f,5.f,2.f);read.homingTravel=version<5?.10f:valid(read.homingTravel,.04f,.3f,.10f);read.crouchDepth=valid(read.crouchDepth,.15f,.65f,.3f);read.haptics=haptic!=0;read.hapticStrength=valid(read.hapticStrength,0.f,1.f,.7f);read.hudX=valid(read.hudX,-10.f,10.f,0.f);read.hudY=valid(read.hudY,-10.f,10.f,0.f);read.hudSize=valid(read.hudSize,.05f,2.f,1.f);for(auto& hand:read.handAngles)for(auto& angle:hand)angle=valid(angle,-180.f,180.f,0.f);read.titleSize=valid(read.titleSize,.05f,2.f,.65f);read.titleDistance=valid(read.titleDistance,.25f,20.f,3.f);read.titleX=valid(read.titleX,-10.f,10.f,0.f);read.titleY=valid(read.titleY,-10.f,10.f,0.f);read.vrMenuSize=valid(read.vrMenuSize,.1f,3.f,1.05f);read.vrMenuDistance=valid(read.vrMenuDistance,.25f,10.f,1.15f);read.menuFollowView=follow!=0;read.hudFollowView=hudFollow!=0;
        if(version==6){VROptions defaults;for(int h=0;h<2;++h)if(read.handAngles[h][0]==0&&read.handAngles[h][1]==0&&read.handAngles[h][2]==0)std::copy(defaults.handAngles[h],defaults.handAngles[h]+3,read.handAngles[h]);}
        options=read;}}
}
VROptions GetOptions(){std::lock_guard<std::mutex> lock(mutex);return options;}
bool VRMenuOpen(){std::lock_guard<std::mutex> lock(mutex);return open;}
bool ConsumeRecenter(){std::lock_guard<std::mutex> lock(mutex);bool r=recenterRequested;recenterRequested=false;return r;}
uint64_t MenuRevision(){std::lock_guard<std::mutex> lock(mutex);return revision;}
Controls UpdateVRMenu(const Controls& c,double now){
    std::lock_guard<std::mutex> lock(mutex);
    if(!c.focused){oldButtons=0;oldVertical=oldHorizontal=0;chordHeld=false;chordArmed=false;swallow=true;leftClickPending=false;return c;}
    const bool chord=(c.buttons&(LClick|RClick))==(LClick|RClick);
    const uint32_t pressed=c.buttons&~oldButtons;
    if(!(c.buttons&(LClick|RClick)))chordArmed=true;
    if(chord&&!chordHeld&&chordArmed){open=!open;if(open){category=-1;selected=0;}swallow=true;++revision;nextRepeat=now+.3;}
    chordHeld=chord;oldButtons=c.buttons;
    if(open){
        if((pressed&B)&&!chord){open=false;swallow=true;++revision;}
        if(open&&!chord){
            int vertical=c.ly>.55f?-1:c.ly<-.55f?1:0;int horizontal=c.lx>.55f?1:c.lx<-.55f?-1:0;
            if(pressed&X){if(category>=0){rootSelected=category;category=-1;selected=0;++revision;}nextRepeat=now+.25;}
            else{
                if(vertical&&(vertical!=oldVertical||now>=nextRepeat)){
                    if(category<0)rootSelected=(rootSelected+vertical+categoryCount)%categoryCount;
                    else selected=(selected+vertical+categoryRows[category])%categoryRows[category];
                    nextRepeat=now+.25;++revision;
                }
                if(category>=0&&horizontal&&(horizontal!=oldHorizontal||now>=nextRepeat)){change(horizontal);nextRepeat=now+.25;}
                if(pressed&A)change(1);
            }
            oldVertical=vertical;oldHorizontal=horizontal;
        }
    }
    const bool neutral=c.buttons==0&&std::fabs(c.lx)<.2f&&std::fabs(c.ly)<.2f&&std::fabs(c.rx)<.2f&&std::fabs(c.ry)<.2f;
    if(!open&&!chord&&neutral)swallow=false;
    if(open||swallow||chord){leftClickPending=false;Controls blank;blank.focused=true;blank.blocked=true;return blank;}
    Controls result=c;
    if(pressed&LClick){leftClickSince=now;leftClickPending=true;}
    if(c.buttons&LClick){
        if(now-leftClickSince<.15)result.buttons&=~LClick;
        else leftClickPending=false;
    }else if(leftClickPending){result.buttons|=LClick;leftClickPending=false;}
    return result;
}
void RasterMenu(uint32_t* p,int w,int h){
    std::lock_guard<std::mutex> lock(mutex);
    std::fill(p,p+w*h,color(5,18,54));
    for(int y=0;y<130;++y)rect(p,w,h,0,y,w,1,color(8,55-y/5,150-y/3));
    rect(p,w,h,0,0,w,8,color(38,191,255));rect(p,w,h,0,127,w,4,color(255,205,55));
    text(p,w,h,52,46,"PROJECT 06 / VR",4,color(2,12,45),true);
    text(p,w,h,48,40,"PROJECT 06 / VR",4,color(242,249,255),true);
    text(p,w,h,48,94,"QUEST 3   -   CANDIDATE 0.1.14",2,color(137,214,255));
    text(p,w,h,48,158,category<0?"VR SETTINGS":categoryNames[category],3,color(255,217,85));
    const int count=category<0?categoryCount:categoryRows[category];
    int first=category>=0?std::max(0,selected-5):0;
    for(int i=first;i<std::min(count,first+6);++i){int y=222+(i-first)*(category<0?76:102);const int option=category<0?-1:menuItems[category][i].option;if(i==(category<0?rootSelected:selected)){rect(p,w,h,30,y-12,w-60,74,color(12,72,156));rect(p,w,h,30,y-12,5,74,color(255,205,55));}
        if(category<0){text(p,w,h,60,y+6,categoryNames[i],3,color(231,245,255));text(p,w,h,w-104,y+6,">",3,color(255,217,85));continue;}
        char value[80]{};switch(option){case 0:snprintf(value,sizeof(value),"%s",modeName());break;case 1:snprintf(value,sizeof(value),"%d%%",int(std::round(options.renderScale*100)));break;case 2:snprintf(value,sizeof(value),"%.2fx",options.worldScale);break;case 3:snprintf(value,sizeof(value),"%s",options.positionalTracking?"ON":"ROTATION ONLY");break;case 4:snprintf(value,sizeof(value),"%.2fm",options.screenDistance);break;case 5:snprintf(value,sizeof(value),"%.2fm",options.screenWidth);break;case 6:snprintf(value,sizeof(value),"%.1fx",options.stereoStrength);break;case 7:snprintf(value,sizeof(value),"%.2fm",options.hudDistance);break;case 8:strcpy(value,"72 HZ REQUESTED");break;case 9:case 10:strcpy(value,"GAME DEFAULT / WIP");break;case 14:snprintf(value,sizeof(value),"%.2fm",options.eyeHeight);break;
        case 15:strcpy(value,options.motionRun?"ON":"OFF");break;
        case 16:snprintf(value,sizeof(value),"%.1fx",options.runSensitivity);break;
        case 17:snprintf(value,sizeof(value),"%.2f / SECOND",options.runAcceleration);break;
        case 18:strcpy(value,options.gestureHoming?"ON / NATIVE TARGET":"OFF");break;
        case 19:snprintf(value,sizeof(value),"%.2fm",options.homingTravel);break;
        case 20:strcpy(value,options.crouchSpin?"ON":"OFF");break;
        case 21:snprintf(value,sizeof(value),"%.3fm",options.crouchDepth);break;
        case 22:strcpy(value,options.haptics?"ON":"OFF");break;
        case 23:snprintf(value,sizeof(value),"%d%%",int(options.hapticStrength*100));break;
        case 35:strcpy(value,ImmersiveInteractions(options)?"ON / ALL INTERACTIONS":"OFF / INDIVIDUAL SETTINGS");break;
        case 36:snprintf(value,sizeof(value),"%d%%",int(std::round(options.titleSize*100)));break;
        case 37:snprintf(value,sizeof(value),"%.2fm",options.titleDistance);break;
        case 38:snprintf(value,sizeof(value),"%+.2fm (+ RIGHT)",options.titleX);break;
        case 39:snprintf(value,sizeof(value),"%+.2fm (+ UP)",options.titleY);break;
        case 41:snprintf(value,sizeof(value),"%.2fm",options.vrMenuSize);break;
        case 42:snprintf(value,sizeof(value),"%.2fm",options.vrMenuDistance);break;
        case 45:strcpy(value,options.hudFollowView?"ON / HEAD-RELATIVE":"OFF / GAME-CAMERA RELATIVE");break;
        case 43:strcpy(value,options.menuFollowView?"ON":"OFF / ANCHORED ON OPEN");break;
        case 24:snprintf(value,sizeof(value),"%+.2fm  (+ RIGHT)",options.hudX);break;
        case 25:snprintf(value,sizeof(value),"%+.2fm  (+ UP)",options.hudY);break;
        case 26:snprintf(value,sizeof(value),"%d%%",int(std::round(options.hudSize*100)));break;
        case 28:case 29:case 30:case 31:case 32:case 33:snprintf(value,sizeof(value),"%+.0f DEGREES",options.handAngles[(option-28)/3][(option-28)%3]);break;
        case 13:strcpy(value,options.firstPerson?"ON / HEAD-ANCHORED":"OFF / THIRD PERSON");break;default:strcpy(value,"PRESS A");break;}
        text(p,w,h,52,y,menuItems[category][i].label,2,color(231,245,255));text(p,w,h,52,y+30,value,2,(option==8||option==9||option==10)?color(159,181,209):color(106,224,255));
    }
    if(category>=0&&count>6){char hint[100];snprintf(hint,sizeof(hint),"ROWS %d-%d / %d   STICK UP/DOWN TO SCROLL",first+1,std::min(count,first+6),count);text(p,w,h,48,805,hint,2,color(255,217,85));}
    text(p,w,h,48,862,category<0?"STICK: SCROLL   A: OPEN CATEGORY   B: CLOSE":"STICK: SELECT/CHANGE   A: APPLY   X: BACK   B: CLOSE",2,color(209,228,250));
    text(p,w,h,48,899,"BOTH STICK CLICKS: TOGGLE THIS MENU",2,color(182,196,218));
    text(p,w,h,48,936,"HOLD RIGHT META BUTTON: SYSTEM RECENTER",2,color(182,196,218));
    text(p,w,h,48,980,"REFRESH, SHADOWS AND POST EFFECTS ARE DISPLAY-ONLY",2,color(142,153,171));
}
