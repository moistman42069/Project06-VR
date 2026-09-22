#include "bridge.h"
#include "vr_options.h"
#include "view_math.h"
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include "IUnityXRDisplay.h"
#include "IUnityGraphics.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <mutex>
#include <vector>
#include <chrono>

namespace {
IUnityXRDisplayInterface* display=nullptr;
IUnityGraphics* graphics=nullptr;
UnitySubsystemHandle displayHandle{};
XrInstance instance=XR_NULL_HANDLE;
XrSystemId systemId=XR_NULL_SYSTEM_ID;
XrSession session=XR_NULL_HANDLE;
XrSpace localSpace=XR_NULL_HANDLE,renderSpace=XR_NULL_HANDLE,viewSpace=XR_NULL_HANDLE;
XrSessionState sessionState=XR_SESSION_STATE_UNKNOWN;
bool running=false,frameBegun=false,renderFrame=false,recentered=false,shouldRender=false;
XrTime predictedTime=0;
XrTime pendingRecenterTime=0;
VROptions frameOptions;
std::mutex xrMutex;
struct Eye { XrSwapchain swapchain=XR_NULL_HANDLE; uint32_t width=0,height=0,index=0;bool acquired=false;std::vector<XrSwapchainImageOpenGLESKHR> images;std::vector<UnityXRRenderTextureId> textures; } eyes[2];
XrView views[2]={{XR_TYPE_VIEW},{XR_TYPE_VIEW}};
XrActionSet actionSet=XR_NULL_HANDLE;
XrPath hands[2]{};
XrAction stick=XR_NULL_HANDLE,trigger=XR_NULL_HANDLE,grip=XR_NULL_HANDLE,click=XR_NULL_HANDLE;
XrAction buttonA=XR_NULL_HANDLE,buttonB=XR_NULL_HANDLE,buttonX=XR_NULL_HANDLE,buttonY=XR_NULL_HANDLE,menu=XR_NULL_HANDLE;
Controls lastControls;
XrSwapchain menuSwapchain=XR_NULL_HANDLE;
std::vector<XrSwapchainImageOpenGLESKHR> menuImages;
std::vector<uint64_t> menuImageRevision;
std::vector<uint32_t> menuPixels,menuUpload;
bool menuReady=false;
constexpr int menuSize=1024;
bool check(XrResult r,const char* what){
    if(XR_SUCCEEDED(r))return true;
    // Failures remain observable without flooding logs when a session is lost.
    static unsigned failures=0;if(failures++<30 || failures%300==0)LOG("OpenXR %s failed: %d (failure %u)",what,int(r),failures);return false;
}
#define XR_OK(expr) check((expr),#expr)
XrPath path(const char* s){XrPath p=0;XR_OK(xrStringToPath(instance,s,&p));return p;}
bool makeAction(XrAction& out,const char* name,XrActionType type,bool both){
    XrActionCreateInfo a{XR_TYPE_ACTION_CREATE_INFO};strncpy(a.actionName,name,sizeof(a.actionName)-1);strncpy(a.localizedActionName,name,sizeof(a.localizedActionName)-1);
    a.actionType=type;if(both){a.countSubactionPaths=2;a.subactionPaths=hands;}return XR_OK(xrCreateAction(actionSet,&a,&out));
}
bool setupActions(){
    hands[0]=path("/user/hand/left");hands[1]=path("/user/hand/right");
    XrActionSetCreateInfo ci{XR_TYPE_ACTION_SET_CREATE_INFO};strcpy(ci.actionSetName,"p06_gameplay");strcpy(ci.localizedActionSetName,"Project 06 controls");
    if(!XR_OK(xrCreateActionSet(instance,&ci,&actionSet)))return false;
    if(!makeAction(stick,"stick",XR_ACTION_TYPE_VECTOR2F_INPUT,true) || !makeAction(trigger,"trigger",XR_ACTION_TYPE_FLOAT_INPUT,true) ||
       !makeAction(grip,"grip",XR_ACTION_TYPE_FLOAT_INPUT,true) || !makeAction(click,"stick_click",XR_ACTION_TYPE_BOOLEAN_INPUT,true) ||
       !makeAction(buttonA,"button_a",XR_ACTION_TYPE_BOOLEAN_INPUT,false) || !makeAction(buttonB,"button_b",XR_ACTION_TYPE_BOOLEAN_INPUT,false) ||
       !makeAction(buttonX,"button_x",XR_ACTION_TYPE_BOOLEAN_INPUT,false) || !makeAction(buttonY,"button_y",XR_ACTION_TYPE_BOOLEAN_INPUT,false) ||
       !makeAction(menu,"menu",XR_ACTION_TYPE_BOOLEAN_INPUT,false))return false;
    std::vector<XrActionSuggestedBinding> b;
    auto bind=[&](XrAction a,const char* s){b.push_back({a,path(s)});};
    bind(stick,"/user/hand/left/input/thumbstick");bind(stick,"/user/hand/right/input/thumbstick");
    bind(trigger,"/user/hand/left/input/trigger/value");bind(trigger,"/user/hand/right/input/trigger/value");
    bind(grip,"/user/hand/left/input/squeeze/value");bind(grip,"/user/hand/right/input/squeeze/value");
    bind(click,"/user/hand/left/input/thumbstick/click");bind(click,"/user/hand/right/input/thumbstick/click");
    bind(buttonA,"/user/hand/right/input/a/click");bind(buttonB,"/user/hand/right/input/b/click");
    bind(buttonX,"/user/hand/left/input/x/click");bind(buttonY,"/user/hand/left/input/y/click");bind(menu,"/user/hand/left/input/menu/click");
    XrInteractionProfileSuggestedBinding suggested{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
    suggested.interactionProfile=path("/interaction_profiles/oculus/touch_controller");suggested.countSuggestedBindings=uint32_t(b.size());suggested.suggestedBindings=b.data();
    if(!XR_OK(xrSuggestInteractionProfileBindings(instance,&suggested)))return false;
    XrSessionActionSetsAttachInfo attach{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};attach.countActionSets=1;attach.actionSets=&actionSet;
    return XR_OK(xrAttachSessionActionSets(session,&attach));
}
void deadzone(float& x,float& y){
    if(!std::isfinite(x)||!std::isfinite(y)){x=y=0;return;}float len=std::sqrt(x*x+y*y);
    if(len<0.18f){x=y=0;return;}float scale=std::min(1.f,(len-0.18f)/0.82f)/len;x*=scale;y*=scale;
}
Controls readControls(){
    Controls c;
    if(!running || sessionState!=XR_SESSION_STATE_FOCUSED || !actionSet)return c;
    XrActiveActionSet active{actionSet,XR_NULL_PATH};XrActionsSyncInfo sync{XR_TYPE_ACTIONS_SYNC_INFO};sync.countActiveActionSets=1;sync.activeActionSets=&active;
    if(!XR_OK(xrSyncActions(session,&sync)))return c;
    c.focused=true;
    auto getFloat=[&](XrAction a,int hand){XrActionStateGetInfo g{XR_TYPE_ACTION_STATE_GET_INFO};g.action=a;g.subactionPath=hands[hand];XrActionStateFloat s{XR_TYPE_ACTION_STATE_FLOAT};return XR_OK(xrGetActionStateFloat(session,&g,&s))&&s.isActive&&std::isfinite(s.currentState)?std::clamp(s.currentState,0.f,1.f):0.f;};
    auto getBool=[&](XrAction a,XrPath hand=XR_NULL_PATH){XrActionStateGetInfo g{XR_TYPE_ACTION_STATE_GET_INFO};g.action=a;g.subactionPath=hand;XrActionStateBoolean s{XR_TYPE_ACTION_STATE_BOOLEAN};return XR_OK(xrGetActionStateBoolean(session,&g,&s))&&s.isActive&&s.currentState;};
    for(int i=0;i<2;++i){XrActionStateGetInfo g{XR_TYPE_ACTION_STATE_GET_INFO};g.action=stick;g.subactionPath=hands[i];XrActionStateVector2f s{XR_TYPE_ACTION_STATE_VECTOR2F};
        if(XR_OK(xrGetActionStateVector2f(session,&g,&s))&&s.isActive){auto x=s.currentState.x,y=s.currentState.y;deadzone(x,y);if(i==0){c.lx=x;c.ly=y;}else{c.rx=x;c.ry=y;}}}
    c.lt=getFloat(trigger,0);c.rt=getFloat(trigger,1);c.lg=getFloat(grip,0);c.rg=getFloat(grip,1);
    if(getBool(buttonA))c.buttons|=A;if(getBool(buttonB))c.buttons|=B;if(getBool(buttonX))c.buttons|=X;if(getBool(buttonY))c.buttons|=Y;
    if(getBool(menu))c.buttons|=Menu;if(getBool(click,hands[0]))c.buttons|=LClick;if(getBool(click,hands[1]))c.buttons|=RClick;
    // Hysteresis prevents analog trigger/grip noise from producing button edges.
    auto analog=[&](float value,uint32_t bit){if(value>((lastControls.buttons&bit)?0.4f:0.55f))c.buttons|=bit;};
    analog(c.lt,LT);analog(c.rt,RT);analog(c.lg,LB);analog(c.rg,RB);return c;
}
void pollEvents(){
    if(!instance)return;
    for(;;){XrEventDataBuffer ev{XR_TYPE_EVENT_DATA_BUFFER};XrResult r=xrPollEvent(instance,&ev);if(r==XR_EVENT_UNAVAILABLE)break;if(!XR_OK(r))break;
        if(ev.type==XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED){auto& s=*reinterpret_cast<XrEventDataSessionStateChanged*>(&ev);sessionState=s.state;LOG("OpenXR session state=%d",int(sessionState));
            if(s.state==XR_SESSION_STATE_READY&&!running){XrSessionBeginInfo bi{XR_TYPE_SESSION_BEGIN_INFO};bi.primaryViewConfigurationType=XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;running=XR_OK(xrBeginSession(session,&bi));}
            if(s.state==XR_SESSION_STATE_STOPPING&&running){XR_OK(xrEndSession(session));running=false;}
            if(s.state==XR_SESSION_STATE_EXITING||s.state==XR_SESSION_STATE_LOSS_PENDING){running=false;}
        }else if(ev.type==XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING){
            auto& change=*reinterpret_cast<XrEventDataReferenceSpaceChangePending*>(&ev);
            if(change.referenceSpaceType==XR_REFERENCE_SPACE_TYPE_LOCAL){pendingRecenterTime=change.changeTime;LOG("System recenter queued for runtime changeTime");}
        }
    }
}
bool createInstance(){
    JNIEnv* env=nullptr;if(!g_vm||!g_activity){LOG("Java activity unavailable");return false;}
    if(g_vm->GetEnv((void**)&env,JNI_VERSION_1_6)!=JNI_OK && g_vm->AttachCurrentThread(&env,nullptr)!=JNI_OK)return false;
    PFN_xrInitializeLoaderKHR initialize=nullptr;xrGetInstanceProcAddr(XR_NULL_HANDLE,"xrInitializeLoaderKHR",reinterpret_cast<PFN_xrVoidFunction*>(&initialize));
    if(!initialize){LOG("OpenXR Android loader initialization missing");return false;}
    XrLoaderInitInfoAndroidKHR li{XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};li.applicationVM=g_vm;li.applicationContext=g_activity;
    if(!XR_OK(initialize(reinterpret_cast<XrLoaderInitInfoBaseHeaderKHR*>(&li))))return false;
    uint32_t n=0;if(!XR_OK(xrEnumerateInstanceExtensionProperties(nullptr,0,&n,nullptr))||n>2048)return false;
    std::vector<XrExtensionProperties> props(n,{XR_TYPE_EXTENSION_PROPERTIES});if(!XR_OK(xrEnumerateInstanceExtensionProperties(nullptr,n,&n,props.data())))return false;
    auto has=[&](const char* name){return std::any_of(props.begin(),props.end(),[&](auto& p){return !strcmp(p.extensionName,name);});};
    std::vector<const char*> ext={XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME,XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME};
    for(auto name:ext)if(!has(name)){LOG("Required extension missing: %s",name);return false;}
    if(has(XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME))ext.push_back(XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME);
    XrInstanceCreateInfoAndroidKHR ai{XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};ai.applicationVM=g_vm;ai.applicationActivity=g_activity;
    XrInstanceCreateInfo ci{XR_TYPE_INSTANCE_CREATE_INFO};ci.next=&ai;strcpy(ci.applicationInfo.applicationName,"P06 Quest");ci.applicationInfo.applicationVersion=1;
    strcpy(ci.applicationInfo.engineName,"Unity native XR provider");ci.applicationInfo.apiVersion=XR_API_VERSION_1_0;ci.enabledExtensionCount=uint32_t(ext.size());ci.enabledExtensionNames=ext.data();
    if(!XR_OK(xrCreateInstance(&ci,&instance)))return false;
    XrInstanceProperties ip{XR_TYPE_INSTANCE_PROPERTIES};xrGetInstanceProperties(instance,&ip);LOG("OpenXR runtime: %s",ip.runtimeName);
    XrSystemGetInfo si{XR_TYPE_SYSTEM_GET_INFO};si.formFactor=XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    return XR_OK(xrGetSystem(instance,&si,&systemId));
}
bool createSession(){
    if(graphics->GetRenderer()!=kUnityGfxRendererOpenGLES30){LOG("Unsupported renderer %d: GLES3 required",int(graphics->GetRenderer()));return false;}
    auto eglDisplay=eglGetCurrentDisplay();auto context=eglGetCurrentContext();if(eglDisplay==EGL_NO_DISPLAY||context==EGL_NO_CONTEXT){LOG("Unity GLES context unavailable");return false;}
    PFN_xrGetOpenGLESGraphicsRequirementsKHR requirements=nullptr;
    if(!XR_OK(xrGetInstanceProcAddr(instance,"xrGetOpenGLESGraphicsRequirementsKHR",reinterpret_cast<PFN_xrVoidFunction*>(&requirements)))||!requirements)return false;
    XrGraphicsRequirementsOpenGLESKHR req{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR};if(!XR_OK(requirements(instance,systemId,&req)))return false;
    EGLint configId=0;eglQueryContext(eglDisplay,context,EGL_CONFIG_ID,&configId);EGLint attrs[]={EGL_CONFIG_ID,configId,EGL_NONE},count=0;EGLConfig config=nullptr;
    if(!eglChooseConfig(eglDisplay,attrs,&config,1,&count)||count!=1){LOG("Unity EGLConfig unavailable");return false;}
    XrGraphicsBindingOpenGLESAndroidKHR gb{XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR};gb.display=eglDisplay;gb.context=context;gb.config=config;
    XrSessionCreateInfo ci{XR_TYPE_SESSION_CREATE_INFO};ci.next=&gb;ci.systemId=systemId;if(!XR_OK(xrCreateSession(instance,&ci,&session)))return false;
    XrReferenceSpaceCreateInfo sp{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};sp.referenceSpaceType=XR_REFERENCE_SPACE_TYPE_LOCAL;sp.poseInReferenceSpace.orientation.w=1;
    if(!XR_OK(xrCreateReferenceSpace(session,&sp,&localSpace))||!XR_OK(xrCreateReferenceSpace(session,&sp,&renderSpace))||!setupActions())return false;
    sp.referenceSpaceType=XR_REFERENCE_SPACE_TYPE_VIEW;if(!XR_OK(xrCreateReferenceSpace(session,&sp,&viewSpace)))return false;
    PFN_xrRequestDisplayRefreshRateFB refresh=nullptr;
    if(XR_SUCCEEDED(xrGetInstanceProcAddr(instance,"xrRequestDisplayRefreshRateFB",reinterpret_cast<PFN_xrVoidFunction*>(&refresh)))&&refresh)XR_OK(refresh(session,72.f));
    return true;
}
bool createTextures(){
    uint32_t count=0;if(!XR_OK(xrEnumerateViewConfigurationViews(instance,systemId,XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,0,&count,nullptr))||count!=2)return false;
    XrViewConfigurationView configs[2]={{XR_TYPE_VIEW_CONFIGURATION_VIEW},{XR_TYPE_VIEW_CONFIGURATION_VIEW}};
    if(!XR_OK(xrEnumerateViewConfigurationViews(instance,systemId,XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,2,&count,configs)))return false;
    uint32_t nf=0;if(!XR_OK(xrEnumerateSwapchainFormats(session,0,&nf,nullptr))||nf>256)return false;
    std::vector<int64_t> formats(nf);if(!XR_OK(xrEnumerateSwapchainFormats(session,nf,&nf,formats.data())))return false;
    // Linear RGBA8 avoids assuming that the original game uses linear lighting.
    int64_t format=GL_RGBA8;if(std::find(formats.begin(),formats.end(),format)==formats.end())return false;
    const float scale=GetOptions().renderScale;
    for(int eye=0;eye<2;++eye){auto& e=eyes[eye];e.width=std::min(2048u,uint32_t(configs[eye].recommendedImageRectWidth*scale));e.height=std::min(2048u,uint32_t(configs[eye].recommendedImageRectHeight*scale));
        e.width=std::max(16u,e.width&~3u);e.height=std::max(16u,e.height&~3u);
        XrSwapchainCreateInfo ci{XR_TYPE_SWAPCHAIN_CREATE_INFO};ci.usageFlags=XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT|XR_SWAPCHAIN_USAGE_SAMPLED_BIT;ci.format=format;ci.sampleCount=1;ci.width=e.width;ci.height=e.height;ci.faceCount=1;ci.arraySize=1;ci.mipCount=1;
        if(!XR_OK(xrCreateSwapchain(session,&ci,&e.swapchain)))return false;
        uint32_t n=0;if(!XR_OK(xrEnumerateSwapchainImages(e.swapchain,0,&n,nullptr))||n==0||n>16)return false;
        e.images.assign(n,{XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR});e.textures.resize(n);
        if(!XR_OK(xrEnumerateSwapchainImages(e.swapchain,n,&n,reinterpret_cast<XrSwapchainImageBaseHeader*>(e.images.data()))))return false;
        for(uint32_t i=0;i<n;++i){UnityXRRenderTextureDesc td{};td.colorFormat=kUnityXRRenderTextureFormatRGBA32;td.color.nativePtr=reinterpret_cast<void*>(uintptr_t(e.images[i].image));td.depthFormat=kUnityXRDepthTextureFormat24bitOrGreater;td.width=e.width;td.height=e.height;td.textureArrayLength=0;td.flags=kUnityXRRenderTextureFlagsLockedWidthHeight;
            auto r=display->CreateTexture(displayHandle,&td,&e.textures[i]);if(r!=kUnitySubsystemErrorCodeSuccess){LOG("Unity CreateTexture failed: %d",int(r));return false;}}
        LOG("Eye %d: %ux%u, %u images, native stereo multipass",eye,e.width,e.height,n);
    }return true;
}
bool createMenu(){
    XrSwapchainCreateInfo ci{XR_TYPE_SWAPCHAIN_CREATE_INFO};ci.usageFlags=XR_SWAPCHAIN_USAGE_SAMPLED_BIT|XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;ci.format=GL_RGBA8;ci.sampleCount=1;ci.width=ci.height=menuSize;ci.faceCount=ci.arraySize=ci.mipCount=1;
    if(!XR_OK(xrCreateSwapchain(session,&ci,&menuSwapchain)))return false;
    uint32_t n=0;if(!XR_OK(xrEnumerateSwapchainImages(menuSwapchain,0,&n,nullptr))||n<1||n>16)return false;
    menuImages.assign(n,{XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR});menuImageRevision.assign(n,0);menuPixels.resize(menuSize*menuSize);menuUpload.resize(menuSize*menuSize);
    return XR_OK(xrEnumerateSwapchainImages(menuSwapchain,n,&n,reinterpret_cast<XrSwapchainImageBaseHeader*>(menuImages.data())));
}
void updateMenuTexture(){
    menuReady=false;if(!VRMenuOpen()||!menuSwapchain)return;
    uint32_t index=0;XrSwapchainImageAcquireInfo ai{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    if(!XR_OK(xrAcquireSwapchainImage(menuSwapchain,&ai,&index)))return;
    XrSwapchainImageWaitInfo wi{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};wi.timeout=XR_INFINITE_DURATION;
    bool valid=XR_OK(xrWaitSwapchainImage(menuSwapchain,&wi))&&index<menuImages.size();
    if(valid && menuImageRevision[index]!=MenuRevision()){
        RasterMenu(menuPixels.data(),menuSize,menuSize);
        for(int y=0;y<menuSize;++y)std::copy_n(menuPixels.data()+y*menuSize,menuSize,menuUpload.data()+(menuSize-y-1)*menuSize);
        GLint binding=0,buffer=0,alignment=0,row=0,skipRows=0,skipPixels=0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D,&binding);glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING,&buffer);glGetIntegerv(GL_UNPACK_ALIGNMENT,&alignment);
        glGetIntegerv(GL_UNPACK_ROW_LENGTH,&row);glGetIntegerv(GL_UNPACK_SKIP_ROWS,&skipRows);glGetIntegerv(GL_UNPACK_SKIP_PIXELS,&skipPixels);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);glPixelStorei(GL_UNPACK_ALIGNMENT,4);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glPixelStorei(GL_UNPACK_SKIP_ROWS,0);glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
        glBindTexture(GL_TEXTURE_2D,menuImages[index].image);glTexSubImage2D(GL_TEXTURE_2D,0,0,0,menuSize,menuSize,GL_RGBA,GL_UNSIGNED_BYTE,menuUpload.data());
        glBindTexture(GL_TEXTURE_2D,binding);glBindBuffer(GL_PIXEL_UNPACK_BUFFER,buffer);glPixelStorei(GL_UNPACK_ALIGNMENT,alignment);glPixelStorei(GL_UNPACK_ROW_LENGTH,row);glPixelStorei(GL_UNPACK_SKIP_ROWS,skipRows);glPixelStorei(GL_UNPACK_SKIP_PIXELS,skipPixels);
        menuImageRevision[index]=MenuRevision();
    }
    XrSwapchainImageReleaseInfo ri{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};menuReady=XR_OK(xrReleaseSwapchainImage(menuSwapchain,&ri))&&valid;
}
void finishFrame(){
    bool ready=renderFrame&&eyes[0].acquired&&(frameOptions.mode==ViewMode::Theatre||eyes[1].acquired);
    for(auto& e:eyes)if(e.acquired){XrSwapchainImageReleaseInfo ri{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};if(!XR_OK(xrReleaseSwapchainImage(e.swapchain,&ri)))ready=false;e.acquired=false;}
    if(frameBegun){
        XrCompositionLayerProjectionView pv[2]={{XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW},{XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW}};
        for(int i=0;i<2;++i){pv[i].pose=views[i].pose;pv[i].fov=views[i].fov;pv[i].subImage.swapchain=eyes[i].swapchain;pv[i].subImage.imageRect.extent={int32_t(eyes[i].width),int32_t(eyes[i].height)};}
        XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};layer.space=renderSpace;layer.viewCount=2;layer.views=pv;
        const XrCompositionLayerBaseHeader* layers[3]{};uint32_t layerCount=0;
        XrCompositionLayerQuad screens[2]={{XR_TYPE_COMPOSITION_LAYER_QUAD},{XR_TYPE_COMPOSITION_LAYER_QUAD}};
        if(ready){
            if(frameOptions.mode==ViewMode::Immersive)layers[layerCount++]=reinterpret_cast<const XrCompositionLayerBaseHeader*>(&layer);
            else for(int i=0;i<(frameOptions.mode==ViewMode::Theatre?1:2);++i){
                auto& q=screens[i];q.space=renderSpace;q.eyeVisibility=frameOptions.mode==ViewMode::Theatre?XR_EYE_VISIBILITY_BOTH:(i==0?XR_EYE_VISIBILITY_LEFT:XR_EYE_VISIBILITY_RIGHT);
                q.subImage=pv[i].subImage;q.pose.orientation.w=1;q.pose.position.z=-frameOptions.screenDistance;q.size={frameOptions.screenWidth,frameOptions.screenWidth*9.f/16.f};
                layers[layerCount++]=reinterpret_cast<const XrCompositionLayerBaseHeader*>(&q);
            }
        }
        if(shouldRender)updateMenuTexture();else menuReady=false;
        XrCompositionLayerQuad menuLayer{XR_TYPE_COMPOSITION_LAYER_QUAD};
        if(menuReady){menuLayer.space=viewSpace;menuLayer.eyeVisibility=XR_EYE_VISIBILITY_BOTH;menuLayer.subImage.swapchain=menuSwapchain;menuLayer.subImage.imageRect.extent={menuSize,menuSize};menuLayer.pose.orientation.w=1;menuLayer.pose.position.z=-1.15f;menuLayer.size={1.05f,1.05f};layers[layerCount++]=reinterpret_cast<const XrCompositionLayerBaseHeader*>(&menuLayer);}
        XrFrameEndInfo end{XR_TYPE_FRAME_END_INFO};end.displayTime=predictedTime;end.environmentBlendMode=XR_ENVIRONMENT_BLEND_MODE_OPAQUE;end.layerCount=layerCount;end.layers=layerCount?layers:nullptr;
        XR_OK(xrEndFrame(session,&end));frameBegun=false;
    }renderFrame=false;
}
void shutdown(){
    if(frameBegun)finishFrame();
    for(auto& e:eyes){for(auto t:e.textures)if(t)display->DestroyTexture(displayHandle,t);e.textures.clear();e.images.clear();if(e.swapchain)xrDestroySwapchain(e.swapchain);e=Eye{};}
    if(menuSwapchain)xrDestroySwapchain(menuSwapchain);menuSwapchain=XR_NULL_HANDLE;menuImages.clear();menuImageRevision.clear();menuPixels.clear();menuUpload.clear();menuReady=false;
    if(viewSpace)xrDestroySpace(viewSpace);if(renderSpace)xrDestroySpace(renderSpace);if(localSpace)xrDestroySpace(localSpace);renderSpace=localSpace=viewSpace=XR_NULL_HANDLE;
    if(session)xrDestroySession(session);session=XR_NULL_HANDLE;
    if(actionSet)xrDestroyActionSet(actionSet);actionSet=XR_NULL_HANDLE;
    if(instance)xrDestroyInstance(instance);instance=XR_NULL_HANDLE;
    running=false;recentered=false;sessionState=XR_SESSION_STATE_UNKNOWN;PublishControls({});
}
bool locate(XrSpace space){
    XrViewLocateInfo li{XR_TYPE_VIEW_LOCATE_INFO};li.viewConfigurationType=XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;li.displayTime=predictedTime;li.space=space;
    XrViewState state{XR_TYPE_VIEW_STATE};uint32_t n=0;
    if(!XR_OK(xrLocateViews(session,&li,&state,2,&n,views))||n!=2)return false;
    constexpr auto valid=XR_VIEW_STATE_ORIENTATION_VALID_BIT|XR_VIEW_STATE_POSITION_VALID_BIT;
    if((state.viewStateFlags&valid)!=valid)return false;
    for(auto& v:views){auto& p=v.pose;auto& q=p.orientation;
        if(!std::isfinite(p.position.x)||!std::isfinite(p.position.y)||!std::isfinite(p.position.z)||!std::isfinite(q.x)||!std::isfinite(q.y)||!std::isfinite(q.z)||!std::isfinite(q.w))return false;}
    return true;
}
void recenter(){
    if(!locate(localSpace))return;
    auto q=views[0].pose.orientation;
    float yaw=std::atan2(2.f*(q.w*q.y+q.x*q.z),1.f-2.f*(q.y*q.y+q.x*q.x));
    XrReferenceSpaceCreateInfo ci{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};ci.referenceSpaceType=XR_REFERENCE_SPACE_TYPE_LOCAL;ci.poseInReferenceSpace.orientation={0,std::sin(yaw/2),0,std::cos(yaw/2)};
    ci.poseInReferenceSpace.position={(views[0].pose.position.x+views[1].pose.position.x)*.5f,(views[0].pose.position.y+views[1].pose.position.y)*.5f,(views[0].pose.position.z+views[1].pose.position.z)*.5f};
    XrSpace next=XR_NULL_HANDLE;if(XR_OK(xrCreateReferenceSpace(session,&ci,&next))){if(renderSpace)xrDestroySpace(renderSpace);renderSpace=next;recentered=true;LOG("Headset origin recentered; game third-person camera retained");}
}
UnitySubsystemErrorCode UNITY_INTERFACE_API gfxStart(UnitySubsystemHandle h,void*,UnityXRRenderingCapabilities* caps){
    std::lock_guard<std::mutex> guard(xrMutex);displayHandle=h;
    caps->noSinglePassRenderingSupport=true;caps->invalidateRenderStateAfterEachCallback=true;caps->skipPresentToMainScreen=true;
    if(!createInstance()||!createSession()||!createTextures()||!createMenu()){LOG("XR graphics startup FAILED");shutdown();return kUnitySubsystemErrorCodeFailure;}
    LOG("XR graphics initialized");return kUnitySubsystemErrorCodeSuccess;
}
UnitySubsystemErrorCode UNITY_INTERFACE_API populate(UnitySubsystemHandle,void*,const UnityXRFrameSetupHints*,UnityXRNextFrameDesc* frame){
    std::lock_guard<std::mutex> guard(xrMutex);*frame={};frame->mirrorBlitMode=kUnityXRMirrorBlitNone;
    // The pinned APK disables the graphics worker. Sync after game camera updates;
    // the helper refuses Unity object access if a separate worker is ever enabled.
    SyncHudCamera();
    if(frameBegun)finishFrame();pollEvents();if(!running)return kUnitySubsystemErrorCodeSuccess;
    XrFrameWaitInfo wi{XR_TYPE_FRAME_WAIT_INFO};XrFrameState fs{XR_TYPE_FRAME_STATE};if(!XR_OK(xrWaitFrame(session,&wi,&fs)))return kUnitySubsystemErrorCodeSuccess;
    XrFrameBeginInfo bi{XR_TYPE_FRAME_BEGIN_INFO};if(!XR_OK(xrBeginFrame(session,&bi)))return kUnitySubsystemErrorCodeSuccess;frameBegun=true;predictedTime=fs.predictedDisplayTime;
    if(pendingRecenterTime && predictedTime>=pendingRecenterTime){recentered=false;pendingRecenterTime=0;}
    bool recenterRequested=ConsumeRecenter();if(!recentered||recenterRequested)recenter();
    frameOptions=GetOptions();
    shouldRender=fs.shouldRender;
    if(!shouldRender||!locate(renderSpace))return kUnitySubsystemErrorCodeSuccess;
    int passCount=frameOptions.mode==ViewMode::Theatre?1:2;
    for(int i=0;i<passCount;++i){auto& e=eyes[i];XrSwapchainImageAcquireInfo ai{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
        if(!XR_OK(xrAcquireSwapchainImage(e.swapchain,&ai,&e.index)))return kUnitySubsystemErrorCodeSuccess;
        e.acquired=true;XrSwapchainImageWaitInfo wait{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};wait.timeout=XR_INFINITE_DURATION;
        if(!XR_OK(xrWaitSwapchainImage(e.swapchain,&wait))||e.index>=e.textures.size())return kUnitySubsystemErrorCodeSuccess;
        auto& pass=frame->renderPasses[i];pass.textureId=e.textures[e.index];pass.cullingPassIndex=i;pass.renderParamsCount=1;
        auto& params=pass.renderParams[0];
        // Entire tracked eye pose is relative to the untouched third-person anchor.
        // No XR input provider applies a duplicate head transform.
        params.deviceAnchorToEyePose=P06EyePose(views,i,frameOptions);
        params.projection=P06Projection(views[i].fov,frameOptions);
        params.viewportRect={0,0,1,1};params.textureArraySlice=0;
        frame->cullingPasses[i].deviceAnchorToCullingPose=params.deviceAnchorToEyePose;frame->cullingPasses[i].projection=params.projection;frame->cullingPasses[i].separation=0;
    }
    frame->renderPassesCount=passCount;renderFrame=true;
    static uint64_t frames=0;if((frames++%600)==0){LOG("Frame %llu; mode=%d passes=%d views valid; focused=%d",(unsigned long long)frames,int(frameOptions.mode),passCount,sessionState==XR_SESSION_STATE_FOCUSED);LogBridgeStats();}
    return kUnitySubsystemErrorCodeSuccess;
}
UnitySubsystemErrorCode UNITY_INTERFACE_API submit(UnitySubsystemHandle,void*){std::lock_guard<std::mutex> guard(xrMutex);finishFrame();return kUnitySubsystemErrorCodeSuccess;}
UnitySubsystemErrorCode UNITY_INTERFACE_API gfxStop(UnitySubsystemHandle,void*){std::lock_guard<std::mutex> guard(xrMutex);shutdown();return kUnitySubsystemErrorCodeSuccess;}
UnitySubsystemErrorCode UNITY_INTERFACE_API update(UnitySubsystemHandle,void*,UnityXRDisplayState* state){
    {std::lock_guard<std::mutex> guard(xrMutex);pollEvents();auto next=readControls();
        if(next.focused!=lastControls.focused||next.buttons!=lastControls.buttons)LOG("Touch input: focused=%d buttons=0x%x left=(%.2f,%.2f) right=(%.2f,%.2f)",int(next.focused),next.buttons,next.lx,next.ly,next.rx,next.ry);
        lastControls=next;
        double now=std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
        PublishControls(UpdateVRMenu(lastControls,now));state->focusLost=sessionState!=XR_SESSION_STATE_FOCUSED;state->displayIsTransparent=false;state->reprojectionMode=kUnityXRReprojectionModePositionAndOrientation;}
    GameMainTick();return kUnitySubsystemErrorCodeSuccess;
}
UnitySubsystemErrorCode UNITY_INTERFACE_API initialize(UnitySubsystemHandle h,void*){
    UnityXRDisplayGraphicsThreadProvider gp{};gp.Start=gfxStart;gp.SubmitCurrentFrame=submit;gp.PopulateNextFrameDesc=populate;gp.Stop=gfxStop;
    auto r=display->RegisterProviderForGraphicsThread(h,&gp);if(r!=kUnitySubsystemErrorCodeSuccess)return r;
    UnityXRDisplayProvider p{};p.UpdateDisplayState=update;return display->RegisterProvider(h,&p);
}
UnitySubsystemErrorCode UNITY_INTERFACE_API start(UnitySubsystemHandle,void*){return kUnitySubsystemErrorCodeSuccess;}
void UNITY_INTERFACE_API stop(UnitySubsystemHandle,void*){LOG("Unity XR lifecycle stop");}
void UNITY_INTERFACE_API destroy(UnitySubsystemHandle,void*){LOG("Unity XR lifecycle shutdown");}
}
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* interfaces){
    display=interfaces->Get<IUnityXRDisplayInterface>();graphics=interfaces->Get<IUnityGraphics>();
    if(!display||!graphics){LOG("Unity XR provider interfaces missing");return;}
    UnityLifecycleProvider life{};life.Initialize=initialize;life.Start=start;life.Stop=stop;life.Shutdown=destroy;
    auto r=display->RegisterLifecycleProvider("P06Quest","P06 Quest Display",&life);LOG("Unity XR provider registration=%d",int(r));
}
