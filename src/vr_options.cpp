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
int category=0,selected=0,oldVertical=0,oldHorizontal=0;
double nextRepeat=0;
double leftClickSince=0;
bool leftClickPending=false;
uint64_t revision=1;
std::string filename;
constexpr int categoryCount=8;
const char* categoryNames[]={"UI","VR","GRAPHICS","RUNNING","HOMING","SPINDASH","HAPTICS","SYSTEM"};
struct MenuItem { const char* label; int option; };
constexpr MenuItem menuItems[][6]={
    {{"HUD DISTANCE",7},{"SCREEN DISTANCE",4},{"SCREEN WIDTH",5},{"SCREEN STEREO DEPTH",6},{"",-1}},
    {{"DISPLAY MODE",0},{"WORLD SCALE",2},{"HEAD POSITION",3},{"EXPERIMENTAL FIRST PERSON",13},{"RECENTER VIEW",11},{"FIRST PERSON EYE HEIGHT",14}},
    {{"RENDER SCALE (RESTART)",1},{"REFRESH RATE",8},{"SHADOWS",9},{"POST EFFECTS",10},{"",-1}},
    {{"EXPERIMENTAL MOTION RUN",15},{"SWING SENSITIVITY",16},{"SPEED RAMP",17}},
    {{"EXPERIMENTAL HOMING",18},{"PULL DISTANCE",19}},
    {{"EXPERIMENTAL CROUCH SPIN",20},{"CROUCH DEPTH",21},{"CALIBRATE STANDING",11}},
    {{"GAME HAPTICS",22},{"HAPTIC STRENGTH",23}},
    {{"RESUME GAME",12},{"",-1},{"",-1},{"",-1},{"",-1}}
};
constexpr int categoryRows[]={4,6,4,3,2,3,2,1};
const char* modeName(){switch(options.mode){case ViewMode::StereoScreen:return "3D STEREO SCREEN";case ViewMode::Theatre:return "2D THEATRE";default:return options.firstPerson?"IMMERSIVE FIRST PERSON":"IMMERSIVE THIRD PERSON";}}
void save(){if(filename.empty())return;if(auto f=fopen(filename.c_str(),"w")){
    fprintf(f,"4 %d %.3f %.3f %.3f %.3f %.3f %.3f %d %d %f %d %d %d %f %f %f %f %d %f\n",int(options.mode),options.renderScale,options.worldScale,options.screenDistance,options.screenWidth,options.stereoStrength,options.hudDistance,int(options.positionalTracking),int(options.firstPerson),options.eyeHeight,int(options.motionRun),int(options.gestureHoming),int(options.crouchSpin),options.runSensitivity,options.runAcceleration,options.homingPull,options.crouchDepth,int(options.haptics),options.hapticStrength);fclose(f);}}
float valid(float v,float lo,float hi,float fallback){return std::isfinite(v)?std::clamp(v,lo,hi):fallback;}
void change(int direction){
    const int option=menuItems[category][selected].option;
    switch(option){
        case 0:options.mode=ViewMode((int(options.mode)+direction+3)%3);recenterRequested=true;break;
        case 1:options.renderScale=std::clamp(options.renderScale+direction*.1f,.4f,1.f);break;
        case 2:options.worldScale=std::clamp(options.worldScale+direction*.1f,.25f,3.f);break;
        case 3:options.positionalTracking=!options.positionalTracking;break;
        case 4:options.screenDistance=std::clamp(options.screenDistance+direction*.25f,1.f,6.f);break;
        case 5:options.screenWidth=std::clamp(options.screenWidth+direction*.25f,1.f,6.f);break;
        case 6:options.stereoStrength=std::clamp(options.stereoStrength+direction*.1f,0.f,2.f);break;
        case 7:options.hudDistance=std::clamp(options.hudDistance+direction*.25f,.75f,5.f);break;
        case 11:recenterRequested=true;break;
        case 12:open=false;swallow=true;break;
        case 13:options.firstPerson=!options.firstPerson;break;
        case 14:options.eyeHeight=std::clamp(options.eyeHeight+direction*.05f,.4f,1.5f);break;
        case 15:options.motionRun=!options.motionRun;break;
        case 16:options.runSensitivity=std::clamp(options.runSensitivity+direction*.1f,.2f,3.f);break;
        case 17:options.runAcceleration=std::clamp(options.runAcceleration+direction*.25f,.5f,5.f);break;
        case 18:options.gestureHoming=!options.gestureHoming;break;
        case 19:options.homingPull=std::clamp(options.homingPull+direction*.02f,.08f,.4f);break;
        case 20:options.crouchSpin=!options.crouchSpin;break;
        case 21:options.crouchDepth=std::clamp(options.crouchDepth+direction*.025f,.15f,.65f);break;
        case 22:options.haptics=!options.haptics;break;
        case 23:options.hapticStrength=std::clamp(options.hapticStrength+direction*.1f,0.f,1.f);break;
        default:return; // Clearly labelled read-only placeholders.
    }++revision;save();
}
uint32_t color(int r,int g,int b){return 0xff000000u|uint32_t(b<<16)|uint32_t(g<<8)|uint32_t(r);}
void rect(uint32_t* p,int w,int h,int x,int y,int rw,int rh,uint32_t c){for(int yy=std::max(0,y);yy<std::min(h,y+rh);++yy)for(int xx=std::max(0,x);xx<std::min(w,x+rw);++xx)p[yy*w+xx]=c;}
void text(uint32_t* p,int w,int h,int x,int y,const char* s,int scale,uint32_t c){
    for(;*s;++s,x+=8*scale){unsigned ch=static_cast<unsigned char>(*s);if(ch>=128)ch='?';for(int row=0;row<8;++row)for(int col=0;col<8;++col)if(font8x8_basic[ch][row]&(1<<col))rect(p,w,h,x+col*scale,y+row*scale,scale,scale,c);}}
}
void InitOptions(const char* directory){std::lock_guard<std::mutex> lock(mutex);filename=std::string(directory)+"/vr-settings.txt";if(auto f=fopen(filename.c_str(),"r")){
    VROptions read;int version=0,mode=0,pos=1,fp=0,mr=0,gh=0,cs=0,haptic=1;int n=fscanf(f,"%d %d %f %f %f %f %f %f %d %d %f %d %d %d %f %f %f %f %d %f",&version,&mode,&read.renderScale,&read.worldScale,&read.screenDistance,&read.screenWidth,&read.stereoStrength,&read.hudDistance,&pos,&fp,&read.eyeHeight,&mr,&gh,&cs,&read.runSensitivity,&read.runAcceleration,&read.homingPull,&read.crouchDepth,&haptic,&read.hapticStrength);fclose(f);
    if((n==9&&version==1)||(n==10&&version==2)||(n==18&&version==3)||(n==20&&version==4)){read.mode=ViewMode(std::clamp(mode,0,2));read.renderScale=valid(read.renderScale,.4f,1.f,.7f);read.worldScale=valid(read.worldScale,.25f,3.f,1.f);read.screenDistance=valid(read.screenDistance,1.f,6.f,2.5f);read.screenWidth=valid(read.screenWidth,1.f,6.f,3.f);read.stereoStrength=valid(read.stereoStrength,0.f,2.f,1.f);read.hudDistance=valid(read.hudDistance,.75f,5.f,2.f);read.positionalTracking=pos!=0;read.firstPerson=version>=2&&fp!=0;read.eyeHeight=valid(read.eyeHeight,.4f,1.5f,.85f);read.motionRun=mr!=0;read.gestureHoming=gh!=0;read.crouchSpin=cs!=0;read.runSensitivity=valid(read.runSensitivity,.2f,3.f,1.f);read.runAcceleration=valid(read.runAcceleration,.5f,5.f,2.f);read.homingPull=valid(read.homingPull,.08f,.4f,.18f);read.crouchDepth=valid(read.crouchDepth,.15f,.65f,.3f);read.haptics=haptic!=0;read.hapticStrength=valid(read.hapticStrength,0.f,1.f,.7f);options=read;}}
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
    if(chord&&!chordHeld&&chordArmed){open=!open;swallow=true;++revision;nextRepeat=now+.3;}
    chordHeld=chord;oldButtons=c.buttons;
    if(open){
        if((pressed&B)&&!chord){open=false;swallow=true;++revision;}
        if(open&&!chord){
            int vertical=c.ly>.55f?-1:c.ly<-.55f?1:0;int horizontal=c.lx>.55f?1:c.lx<-.55f?-1:0;
            if((pressed&X)||(pressed&Y)){category=(category+((pressed&Y)?1:categoryCount-1))%categoryCount;selected=std::min(selected,categoryRows[category]-1);nextRepeat=now+.25;++revision;}
            if(vertical&&(vertical!=oldVertical||now>=nextRepeat)){selected=(selected+vertical+categoryRows[category])%categoryRows[category];nextRepeat=now+.25;++revision;}
            if(horizontal&&(horizontal!=oldHorizontal||now>=nextRepeat)){change(horizontal);nextRepeat=now+.25;}
            if(pressed&A)change(1);oldVertical=vertical;oldHorizontal=horizontal;
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
    std::fill(p,p+w*h,color(12,18,31));rect(p,w,h,0,0,w,10,color(55,206,244));
    text(p,w,h,48,44,"PROJECT 06 / VR",4,color(235,244,255));text(p,w,h,48,94,"QUEST 3   -   CANDIDATE 0.1.8",2,color(122,155,190));
    char heading[100];snprintf(heading,sizeof(heading),"< X   %s  (%d/%d)   Y >",categoryNames[category],category+1,categoryCount);text(p,w,h,48,138,heading,3,color(98,219,248));
    const int count=categoryRows[category];
    for(int i=0;i<count;++i){int y=218+i*102;const int option=menuItems[category][i].option;if(i==selected){rect(p,w,h,30,y-12,w-60,74,color(27,60,86));rect(p,w,h,30,y-12,5,74,color(55,206,244));}
        char value[80]{};switch(option){case 0:snprintf(value,sizeof(value),"%s",modeName());break;case 1:snprintf(value,sizeof(value),"%d%%",int(std::round(options.renderScale*100)));break;case 2:snprintf(value,sizeof(value),"%.2fx",options.worldScale);break;case 3:snprintf(value,sizeof(value),"%s",options.positionalTracking?"ON":"ROTATION ONLY");break;case 4:snprintf(value,sizeof(value),"%.2fm",options.screenDistance);break;case 5:snprintf(value,sizeof(value),"%.2fm",options.screenWidth);break;case 6:snprintf(value,sizeof(value),"%.1fx",options.stereoStrength);break;case 7:snprintf(value,sizeof(value),"%.2fm",options.hudDistance);break;case 8:strcpy(value,"72 HZ REQUESTED");break;case 9:case 10:strcpy(value,"GAME DEFAULT / WIP");break;case 14:snprintf(value,sizeof(value),"%.2fm",options.eyeHeight);break;
        case 15:strcpy(value,options.motionRun?"ON":"OFF");break;
        case 16:snprintf(value,sizeof(value),"%.1fx",options.runSensitivity);break;
        case 17:snprintf(value,sizeof(value),"%.2f / SECOND",options.runAcceleration);break;
        case 18:strcpy(value,options.gestureHoming?"ON / NATIVE TARGET":"OFF");break;
        case 19:snprintf(value,sizeof(value),"%.2fm",options.homingPull);break;
        case 20:strcpy(value,options.crouchSpin?"ON":"OFF");break;
        case 21:snprintf(value,sizeof(value),"%.3fm",options.crouchDepth);break;
        case 22:strcpy(value,options.haptics?"ON":"OFF");break;
        case 23:snprintf(value,sizeof(value),"%d%%",int(options.hapticStrength*100));break;
        case 13:strcpy(value,options.firstPerson?"ON / HEAD-ANCHORED":"OFF / THIRD PERSON");break;default:strcpy(value,"PRESS A");break;}
        text(p,w,h,52,y,menuItems[category][i].label,2,color(211,222,237));text(p,w,h,555,y,value,2,(option==8||option==9||option==10)?color(142,153,171):color(98,219,248));
    }
    text(p,w,h,48,862,"STICK: SELECT/CHANGE   X/Y: CATEGORY   A: APPLY   B: CLOSE",2,color(182,196,218));
    text(p,w,h,48,899,"BOTH STICK CLICKS: TOGGLE THIS MENU",2,color(182,196,218));
    text(p,w,h,48,936,"HOLD RIGHT META BUTTON: SYSTEM RECENTER",2,color(182,196,218));
    text(p,w,h,48,980,"REFRESH, SHADOWS AND POST EFFECTS ARE DISPLAY-ONLY",2,color(142,153,171));
}
