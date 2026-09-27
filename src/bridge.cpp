#include "bridge.h"
#include "bindings.h"
#include "vr_options.h"
#include "gestures.h"
#include "haptics.h"
#include "trigger_sweep.h"
#include "hand_model.h"
#include "locomotion_math.h"
#include "visual_adjustments.h"
#include "sonic_gloves.inc"
#include "dobby.h"
#include <android/dlext.h>
#include <dlfcn.h>
#include <atomic>
#include <cmath>
#include <cstring>
#include <mutex>
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <csignal>
#include <unistd.h>
#include <ucontext.h>
#include <time.h>
#include <sys/syscall.h>
#include <vector>
#include <array>
#include <chrono>
#include <cstddef>
#include <type_traits>

JavaVM* g_vm=nullptr;
jobject g_activity=nullptr;
static int logFd=-1;
void Log(const char* format,...){
    char message[1800];va_list args;va_start(args,format);vsnprintf(message,sizeof(message),format,args);va_end(args);
    __android_log_write(ANDROID_LOG_INFO,"P06Quest",message);
    if(logFd>=0){char line[2048];timespec t{};clock_gettime(CLOCK_MONOTONIC,&t);int n=snprintf(line,sizeof(line),"[%lld.%03ld] P06Quest: %s\n",(long long)t.tv_sec,t.tv_nsec/1000000,message);if(n>0)write(logFd,line,std::min(size_t(n),sizeof(line)-1));}
}
namespace {
struct sigaction previousSignals[NSIG]{};
bool signalsInstalled=false;
char* hex(char* dst,uintptr_t number){*dst++='0';*dst++='x';bool started=false;for(int shift=int(sizeof(number)*8)-4;shift>=0;shift-=4){unsigned digit=(number>>shift)&15;if(digit||started||!shift){*dst++="0123456789abcdef"[digit];started=true;}}return dst;}
void crashSignal(int signal,siginfo_t* info,void* raw){
    char text[256];char* p=text;const char prefix[]="P06Quest NATIVE CRASH signal=";memcpy(p,prefix,sizeof(prefix)-1);p+=sizeof(prefix)-1;p=hex(p,signal);
    const char address[]=" fault=";memcpy(p,address,sizeof(address)-1);p+=sizeof(address)-1;p=hex(p,reinterpret_cast<uintptr_t>(info?info->si_addr:nullptr));
#if defined(__aarch64__)
    if(raw){auto context=static_cast<ucontext_t*>(raw);const char pc[]=" pc=";memcpy(p,pc,sizeof(pc)-1);p+=sizeof(pc)-1;p=hex(p,context->uc_mcontext.pc);const char lr[]=" lr=";memcpy(p,lr,sizeof(lr)-1);p+=sizeof(lr)-1;p=hex(p,context->uc_mcontext.regs[30]);const char sp[]=" sp=";memcpy(p,sp,sizeof(sp)-1);p+=sizeof(sp)-1;p=hex(p,context->uc_mcontext.sp);}
#endif
    *p++='\n';if(logFd>=0)write(logFd,text,p-text);
    auto old=previousSignals[signal];sigaction(signal,&old,nullptr);
    if(old.sa_flags&SA_SIGINFO){if(old.sa_handler!=SIG_DFL && old.sa_handler!=SIG_IGN && old.sa_sigaction)old.sa_sigaction(signal,info,raw);}
    else if(old.sa_handler!=SIG_DFL && old.sa_handler!=SIG_IGN && old.sa_handler)old.sa_handler(signal);
}
void installCrashRecorder(){
    if(signalsInstalled)return;signalsInstalled=true;struct sigaction a{};sigemptyset(&a.sa_mask);a.sa_sigaction=crashSignal;a.sa_flags=SA_SIGINFO|SA_ONSTACK;
    for(int s:{SIGSEGV,SIGBUS,SIGILL,SIGFPE,SIGABRT})sigaction(s,&a,&previousSignals[s]);LOG("Minimal native crash recorder installed; previous handlers preserved");
}
void* il=nullptr;
pid_t unityThread=0;
void* hudCamera=nullptr;
bool managedReady=false;
void* (*thread_current)();
void (*gc_free)(uint32_t);
uint32_t playerRoot=0;
struct TrackedPose { bool valid=false;float position[3]{},rotation[4]{0,0,0,1}; };
TrackedPose trackedHands[2];
std::mutex trackedHandsMutex;
struct FirstPersonAnchor { bool valid=false;float position[3]{},rotation[4]{0,0,0,1};void* cameraTransform=nullptr;void* player=nullptr; };
FirstPersonAnchor firstPersonAnchor;
void* rootedCameraTransform=nullptr;
uint32_t cameraTransformRoot=0;
MotionFrame motionFrame;
std::mutex motionMutex;
GestureEngine gestures;
void* gesturePlayer=nullptr;
uint32_t gesturePlayerRoot=0;
struct HiddenRenderer { void* renderer=nullptr;bool wasEnabled=true;uint32_t root=0; };
std::vector<HiddenRenderer> hiddenRenderers;
struct GlovePiece { void* gameObject=nullptr;void* transform=nullptr;float offset[3]{},scale[3]{},rotation[4]{0,0,0,1};bool active=false;uint32_t root=0,transformRoot=0; };
struct Glove { std::array<GlovePiece,1> pieces; };
Glove gloves[2];
bool glovesCreated=false,gloveBuildFailed=false,firstPersonWasActive=false;
void* rootedPlayerBase=nullptr;
uint32_t hudLayerMask=1u<<5;
std::atomic<bool> installed{false};
std::atomic<uint64_t> axisQueries{0},buttonQueries{0};
std::atomic<bool> gameHapticsAllowed{false};
std::mutex inputMutex;
Controls controls;
bool nativeSteeringInstalled=false,fpHeadStick=false,steeringScope=false;
float fpHeadYaw=0,steeringX=0,steeringY=0;
uint32_t previous=0;
bool haveXR=false,tryingXR=false,xrStartAttempted=false;
void* xrStartedSubsystem=nullptr;

using Method=void;
using Obj=void;
void* (*domain_get)();
const void** (*domain_assemblies)(void*,size_t*);
const void* (*assembly_image)(const void*);
void* (*class_from_name)(const void*,const char*,const char*);
void* (*object_class)(void*);
void* (*class_field)(void*,const char*);
void (*field_get)(void*,void*,void*);
void (*field_set)(void*,void*,void*);
void (*field_set_object)(void*,void*,void*);
void (*static_get)(void*,void*);
void (*class_init)(void*);
const Method* (*class_method)(void*,const char*,int);
void* (*invoke)(const Method*,void*,void**,void**);
const void* (*class_type)(void*);
void* (*type_object)(const void*);
uintptr_t (*array_length)(void*);
uint32_t (*array_header_size)();
char* array_address(void* a,int elementSize,uintptr_t index){return reinterpret_cast<char*>(a)+array_header_size()+elementSize*index;}
int32_t (*string_length)(void*);
const char16_t* (*string_chars)(void*);
void* (*resolve)(const char*);
void* (*object_new)(void*);
void* (*string_new)(const char*);
uint32_t (*gc_root)(void*,bool);
void* (*array_new)(void*,uintptr_t);
using CanvasModeSetter=void(*)(void*,int);
CanvasModeSetter oldCanvasModeSetter=nullptr;
void (*canvasSetCamera)(void*,void*)=nullptr;
void (*canvasSetDistance)(void*,float)=nullptr;
bool canvasModeHookInstalled=false,canvasModeHookAttempted=false;
bool eq(void* str,const char* text) {
    if(!str || !string_length || !string_chars) return false;
    auto n=string_length(str); if(n<0 || n>128 || size_t(n)!=strlen(text))return false;
    auto s=string_chars(str);for(int i=0;i<n;++i)if(s[i]!=uint8_t(text[i]))return false;return true;
}
void* klass(const char* ns,const char* name) {
    if(!managedReady||gettid()!=unityThread||!thread_current||!thread_current())return nullptr;
    auto domain=domain_get();if(!domain)return nullptr;
    size_t n=0;auto a=domain_assemblies(domain,&n);if(!a||n>1024)return nullptr;
    for(size_t i=0;i<n;++i)if(auto k=class_from_name(assembly_image(a[i]),ns,name))return k;
    return nullptr;
}
template<typename T>T field(void* obj,const char* name) {
    T value{};if(obj)if(auto f=class_field(object_class(obj),name))field_get(obj,f,&value);return value;
}
void* call(void* k,const char* name,void* self,void** args,int argc) {
    if(!k)return nullptr;auto m=class_method(k,name,argc);if(!m)return nullptr;
    void* ex=nullptr;auto r=invoke(m,self,args,&ex);if(ex){LOG("Managed call failed: %s",name);return nullptr;}return r;
}
bool alive(void* obj){return obj&&field<void*>(obj,"m_CachedPtr");}
bool setObjectField(void* obj,const char* name,void* value){
    if(!obj||!field_set_object)return false;
    auto f=class_field(object_class(obj),name);if(!f)return false;
    // Reference setters take the managed object itself, NEVER &value.
    field_set_object(obj,f,value);void* readback=nullptr;field_get(obj,f,&readback);
    if(readback!=value){LOG("Managed reference assignment failed read-back: %s",name);return false;}
    return true;
}
template<typename T>void setScalarField(void* obj,void* f,const T& value){
    static_assert(std::is_arithmetic<T>::value,"Managed references must use setObjectField");
    field_set(obj,f,const_cast<T*>(&value));
}
template<typename T>T icall(const char* n){auto f=resolve(n);if(!f)LOG("Missing Unity icall: %s",n);return reinterpret_cast<T>(f);}
void quatMultiply(const float a[4],const float b[4],float out[4]){
    out[0]=a[3]*b[0]+a[0]*b[3]+a[1]*b[2]-a[2]*b[1];out[1]=a[3]*b[1]-a[0]*b[2]+a[1]*b[3]+a[2]*b[0];
    out[2]=a[3]*b[2]+a[0]*b[1]-a[1]*b[0]+a[2]*b[3];out[3]=a[3]*b[3]-a[0]*b[0]-a[1]*b[1]-a[2]*b[2];
}
void quatRotate(const float q[4],const float v[3],float out[3]){
    const float tx=2.f*(q[1]*v[2]-q[2]*v[1]),ty=2.f*(q[2]*v[0]-q[0]*v[2]),tz=2.f*(q[0]*v[1]-q[1]*v[0]);
    out[0]=v[0]+q[3]*tx+(q[1]*tz-q[2]*ty);out[1]=v[1]+q[3]*ty+(q[2]*tx-q[0]*tz);out[2]=v[2]+q[3]*tz+(q[0]*ty-q[1]*tx);
}
bool hideSonic(bool hide);
bool refreshFirstPersonAnchor(){
    firstPersonAnchor.valid=false;firstPersonAnchor.cameraTransform=nullptr;
    auto pcClass=klass("","PlayerCamera");auto objectClass=klass("UnityEngine","Object");if(!pcClass||!objectClass)return false;
    static auto find=icall<void*(*)(void*,bool)>("UnityEngine.Object::FindObjectsOfType");
    static void* pc=nullptr;static uint32_t pcRoot=0;
    if(!alive(pc)){if(pcRoot){gc_free(pcRoot);pcRoot=0;}pc=nullptr;
        if(!find)return false;auto pcs=find(type_object(class_type(pcClass)),false);
        if(!pcs||array_length(pcs)<1||array_length(pcs)>16)return false;
        pc=*reinterpret_cast<void**>(array_address(pcs,sizeof(void*),0));if(!alive(pc))return false;pcRoot=gc_root(pc,false);}
    auto player=field<void*>(pc,"PlayerBase");auto camera=field<void*>(pc,"Camera");if(!player||!camera)return false;
    if(!alive(player)||!alive(camera))return false;
    static auto transform=icall<void*(*)(void*)>("UnityEngine.Component::get_transform");
    static auto getPos=icall<void(*)(void*,float*)>("UnityEngine.Transform::get_position_Injected");
    static auto getRot=icall<void(*)(void*,float*)>("UnityEngine.Transform::get_rotation_Injected");
    if(!transform||!getPos||!getRot)return false;
    auto camTransform=transform(camera),playerTransform=transform(player);if(!camTransform||!playerTransform)return false;
    float camPos[3],targetPos[3],camRot[4],playerRot[4];getPos(camTransform,camPos);getPos(playerTransform,targetPos);targetPos[1]+=GetOptions().eyeHeight;getRot(camTransform,camRot);getRot(playerTransform,playerRot);
    float invCam[4]={-camRot[0],-camRot[1],-camRot[2],camRot[3]},delta[4];// Preserve the game camera yaw, without feeding player heading back into the view.
    float forward[3]={0,0,1},worldForward[3];quatRotate(camRot,forward,worldForward);
    float yaw=std::atan2(worldForward[0],worldForward[2]);
    static bool fpYawReady=false;static void* fpPlayer=nullptr;static float fpYaw=0,lastHeadYaw=0;static uint64_t fpOrigin=0;static double lastTime=0;
    auto options=GetOptions();MotionFrame frame;{std::lock_guard<std::mutex> lock(motionMutex);frame=motionFrame;}
    double now=std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    if(options.firstPerson&&options.mode==ViewMode::Immersive){
        if(!fpYawReady||fpPlayer!=player){fpYaw=yaw;fpYawReady=true;fpPlayer=player;fpOrigin=frame.origin;lastHeadYaw=0;lastTime=now;}
        if(fpOrigin!=frame.origin){fpYaw+=lastHeadYaw;fpOrigin=frame.origin;}
        Controls input;{std::lock_guard<std::mutex> lock(inputMutex);input=controls;}
        double dt=std::clamp(now-lastTime,0.0,.05);if(input.focused&&!input.blocked&&!(input.buttons&RClick))fpYaw+=input.rx*1.5707963f*float(dt);
        if(frame.head.valid){float headForward[3];quatRotate(frame.head.q,forward,headForward);lastHeadYaw=std::atan2(headForward[0],headForward[2]);}
        yaw=fpYaw;
    }else fpYawReady=false;
    lastTime=now;float flatRotation[4]={0,std::sin(yaw*.5f),0,std::cos(yaw*.5f)};quatMultiply(invCam,flatRotation,delta);
    float worldOffset[3]={targetPos[0]-camPos[0],targetPos[1]-camPos[1],targetPos[2]-camPos[2]},localOffset[3];quatRotate(invCam,worldOffset,localOffset);
    float length=std::sqrt(delta[0]*delta[0]+delta[1]*delta[1]+delta[2]*delta[2]+delta[3]*delta[3]);
    if(!(length>0.01f)||length>10.f)return false;for(float& v:delta)v/=length;
    for(int i=0;i<3;++i)if(!std::isfinite(localOffset[i]))return false;
    firstPersonAnchor.valid=true;std::copy(localOffset,localOffset+3,firstPersonAnchor.position);std::copy(delta,delta+4,firstPersonAnchor.rotation);firstPersonAnchor.cameraTransform=camTransform;firstPersonAnchor.player=player;
    if(rootedCameraTransform!=camTransform){if(cameraTransformRoot)gc_free(cameraTransformRoot);cameraTransformRoot=gc_root(camTransform,false);rootedCameraTransform=camTransform;}
    if(rootedPlayerBase!=player){hideSonic(false);if(playerRoot)gc_free(playerRoot);playerRoot=gc_root(player,false);rootedPlayerBase=player;}
    return true;
}
void setGameObjectActive(void* object,bool active){static auto set=icall<void(*)(void*,bool)>("UnityEngine.GameObject::SetActive");if(set&&object)set(object,active);}
void setRendererEnabled(void* renderer,bool enabled){static auto set=icall<void(*)(void*,bool)>("UnityEngine.Renderer::set_enabled");if(set&&renderer)set(renderer,enabled);}
bool getRendererEnabled(void* renderer){static auto get=icall<bool(*)(void*)>("UnityEngine.Renderer::get_enabled");return get&&renderer?get(renderer):true;}
void hideRenderer(void* renderer){
    if(!alive(renderer))return;
    auto it=std::find_if(hiddenRenderers.begin(),hiddenRenderers.end(),[&](const HiddenRenderer& saved){return saved.renderer==renderer;});
    if(it==hiddenRenderers.end())hiddenRenderers.push_back({renderer,getRendererEnabled(renderer),gc_root(renderer,false)});
    setRendererEnabled(renderer,false);
}
void hideRendererArray(void* array){
    if(!array)return;auto count=array_length(array);if(count>512)return;
    for(uintptr_t i=0;i<count;++i)hideRenderer(*reinterpret_cast<void**>(array_address(array,sizeof(void*),i)));
}
bool hideSonic(bool hide){
    if(!hide){for(const auto& saved:hiddenRenderers){if(alive(saved.renderer))setRendererEnabled(saved.renderer,saved.wasEnabled);if(saved.root)gc_free(saved.root);}hiddenRenderers.clear();return true;}
    for(auto it=hiddenRenderers.begin();it!=hiddenRenderers.end();){if(!alive(it->renderer)){gc_free(it->root);it=hiddenRenderers.erase(it);}else ++it;}
    auto player=firstPersonAnchor.player;if(!alive(player))return false;
    hideRendererArray(field<void*>(player,"PlayerRenderers"));
    hideRenderer(field<void*>(player,"PlayerRenderer")); // Metal Sonic uses a singular renderer.
    auto upgrades=field<void*>(player,"Upgrades");
    if(alive(upgrades)){auto list=field<void*>(upgrades,"Renderers");if(list)hideRendererArray(field<void*>(list,"_items"));}
    // Include authored accessories/meshes attached under the visual rig, never colliders.
    static auto gameObject=icall<void*(*)(void*)>("UnityEngine.Component::get_gameObject");
    static unsigned scan=0;
    if(hiddenRenderers.empty()||scan++%60==0){
        auto mesh=field<void*>(player,"Mesh"),rendererClass=klass("UnityEngine","Renderer"),goClass=klass("UnityEngine","GameObject");
        if(alive(mesh)&&gameObject&&rendererClass&&goClass){bool typed=true,recursive=true,inactive=true,reverse=false;
            void* args[]={type_object(class_type(rendererClass)),&typed,&recursive,&inactive,&reverse,nullptr};
            hideRendererArray(call(goClass,"GetComponentsInternal",gameObject(mesh),args,6));}
    }
    for(auto& saved:hiddenRenderers)setRendererEnabled(saved.renderer,false);
    return !hiddenRenderers.empty();
}
bool invokeChecked(void* k,const char* name,void* self,void** args,int count){
    if(!k)return false;auto method=class_method(k,name,count);if(!method){LOG("Missing managed glove method: %s",name);return false;}
    void* exception=nullptr;invoke(method,self,args,&exception);if(exception){LOG("Glove method failed: %s",name);return false;}return true;
}
bool buildGloves(){
    auto gameObjectClass=klass("UnityEngine","GameObject"),meshClass=klass("UnityEngine","Mesh"),materialClass=klass("UnityEngine","Material");
    auto colliderType=klass("UnityEngine","Collider"),rendererType=klass("UnityEngine","Renderer"),filterType=klass("UnityEngine","MeshFilter");
    auto vectorClass=klass("UnityEngine","Vector3"),intClass=klass("System","Int32"),shaderClass=klass("UnityEngine","Shader");
    if(!gameObjectClass||!meshClass||!materialClass||!colliderType||!rendererType||!filterType||!vectorClass||!intClass||!shaderClass)return false;
    static auto getTransform=icall<void*(*)(void*)>("UnityEngine.GameObject::get_transform");
    static auto setCollider=icall<void(*)(void*,bool)>("UnityEngine.Collider::set_enabled");
    static auto material=icall<void*(*)(void*)>("UnityEngine.Renderer::GetMaterial");
    static auto persist=icall<void(*)(void*)>("UnityEngine.Object::DontDestroyOnLoad");
    static auto propertyId=icall<int(*)(void*)>("UnityEngine.Shader::PropertyToID");
    static auto setShader=icall<void(*)(void*,void*)>("UnityEngine.Material::set_shader");
    static auto supported=icall<bool(*)(void*)>("UnityEngine.Shader::get_isSupported");
    if(!getTransform||!setCollider||!material||!persist||!propertyId||!setShader||!supported)return false;
    void* gloveShader=nullptr;
    for(const char* name:{"Legacy Shaders/Diffuse","Standard","Unlit/Color"}){void* args[]={string_new(name)};auto candidate=call(shaderClass,"Find",nullptr,args,1);if(alive(candidate)&&supported(candidate)){gloveShader=candidate;LOG("Authored glove shader: %s",name);break;}}
    if(!gloveShader)return false;
    for(int hand=0;hand<2;++hand){auto& piece=gloves[hand].pieces[0];int primitive=3;void* args[]={&primitive};
        piece.gameObject=call(gameObjectClass,"CreatePrimitive",nullptr,args,1);if(!alive(piece.gameObject))return false;
        piece.root=gc_root(piece.gameObject,false);setGameObjectActive(piece.gameObject,false);persist(piece.gameObject);
        piece.transform=getTransform(piece.gameObject);if(!alive(piece.transform))return false;piece.transformRoot=gc_root(piece.transform,false);
        void* colliderArgs[]={type_object(class_type(colliderType))};auto collider=call(gameObjectClass,"GetComponent",piece.gameObject,colliderArgs,1);
        if(!alive(collider))return false;setCollider(collider,false);
        void* rendererArgs[]={type_object(class_type(rendererType))};auto renderer=call(gameObjectClass,"GetComponent",piece.gameObject,rendererArgs,1);
        void* filterArgs[]={type_object(class_type(filterType))};auto filter=call(gameObjectClass,"GetComponent",piece.gameObject,filterArgs,1);
        if(!alive(renderer)||!alive(filter))return false;
        const auto* vertices=hand?kGlove1Vertices:kGlove0Vertices;const auto* normals=hand?kGlove1Normals:kGlove0Normals;
        const auto* triangles=hand?kGlove1Triangles:kGlove0Triangles;
        size_t count=hand?std::size(kGlove1Vertices):std::size(kGlove0Vertices),indexCount=hand?std::size(kGlove1Triangles):std::size(kGlove0Triangles);
        auto mesh=object_new(meshClass);if(!mesh)return false;auto meshRoot=gc_root(mesh,false);
        bool ok=invokeChecked(meshClass,".ctor",mesh,nullptr,0);
        auto v=array_new(vectorClass,count);auto vr=v?gc_root(v,false):0;
        auto n=array_new(vectorClass,count);auto nr=n?gc_root(n,false):0;
        auto t=array_new(intClass,indexCount);auto tr=t?gc_root(t,false):0;
        if(!v||!n||!t){if(vr)gc_free(vr);if(nr)gc_free(nr);if(tr)gc_free(tr);gc_free(meshRoot);return false;}
        memcpy(array_address(v,12,0),vertices,count*12);memcpy(array_address(n,12,0),normals,count*12);memcpy(array_address(t,4,0),triangles,indexCount*4);
        void* va[]={v};void* na[]={n};void* ta[]={t};void* ma[]={mesh};
        ok=ok&&invokeChecked(meshClass,"set_vertices",mesh,va,1)&&invokeChecked(meshClass,"set_normals",mesh,na,1)&&invokeChecked(meshClass,"set_triangles",mesh,ta,1)&&invokeChecked(meshClass,"RecalculateBounds",mesh,nullptr,0)&&invokeChecked(filterType,"set_sharedMesh",filter,ma,1);
        gc_free(vr);gc_free(nr);gc_free(tr);gc_free(meshRoot);if(!ok)return false;
        auto mat=material(renderer);if(!alive(mat))return false;setShader(mat,gloveShader);
        int color=propertyId(string_new("_Color"));float white[4]={.96f,.97f,1.f,1.f};void* colors[]={&color,white};
        if(!invokeChecked(materialClass,"SetColorImpl",mat,colors,2))return false;
        piece.scale[0]=piece.scale[1]=piece.scale[2]=1;piece.rotation[3]=1;
        LOG("Authored Sonic glove %d ready: %zu vertices, %zu triangles, smooth normals, collider disabled",hand,count,indexCount/3);
    }
    return true;
}
bool updateGloves(bool active){
    if(!glovesCreated){if(!active)return true;if(gloveBuildFailed)return false;glovesCreated=buildGloves();if(!glovesCreated){gloveBuildFailed=true;
        auto objectClass=klass("UnityEngine","Object");float delay=0;
        for(auto& glove:gloves){for(auto& piece:glove.pieces){
            if(alive(piece.gameObject)){setGameObjectActive(piece.gameObject,false);void* args[]={piece.gameObject,&delay};call(objectClass,"Destroy",nullptr,args,2);}
            if(piece.root)gc_free(piece.root);if(piece.transformRoot)gc_free(piece.transformRoot);piece={};
        }}
        return false;
    }}
    static auto setPosition=icall<void(*)(void*,const float*)>("UnityEngine.Transform::set_localPosition_Injected");
    static auto setRotation=icall<void(*)(void*,const float*)>("UnityEngine.Transform::set_localRotation_Injected");
    static auto setScale=icall<void(*)(void*,const float*)>("UnityEngine.Transform::set_localScale_Injected");
    if(!setPosition||!setRotation||!setScale)return false;
    float camPos[3]{},camRot[4]{0,0,0,1};
    static auto getPos=icall<void(*)(void*,float*)>("UnityEngine.Transform::get_position_Injected");
    static auto getRot=icall<void(*)(void*,float*)>("UnityEngine.Transform::get_rotation_Injected");
    if(active){if(!getPos||!getRot||!alive(firstPersonAnchor.cameraTransform))return false;getPos(firstPersonAnchor.cameraTransform,camPos);getRot(firstPersonAnchor.cameraTransform,camRot);}
    const auto visualOptions=GetOptions();
    const float worldScale=std::clamp(visualOptions.worldScale,.25f,3.f);
    MotionFrame pose;{std::lock_guard<std::mutex> lock(motionMutex);pose=motionFrame;}
    for(int hand=0;hand<2;++hand){bool visible=active&&pose.focused&&pose.hand[hand].valid&&pose.aim[hand].valid;auto& glove=gloves[hand];
        if(visible){float handRotation[4],adjustment[4],adjustedAim[4];P06HandAdjustment(visualOptions.handAngles[hand],adjustment);quatMultiply(pose.aim[hand].q,adjustment,adjustedAim);quatMultiply(firstPersonAnchor.rotation,adjustedAim,handRotation);float handPos[3],trackedPos[3]={pose.hand[hand].p[0]/worldScale,pose.hand[hand].p[1]/worldScale,pose.hand[hand].p[2]/worldScale};quatRotate(firstPersonAnchor.rotation,trackedPos,handPos);
            for(auto& piece:glove.pieces){float relative[3],scale[3];quatRotate(handRotation,piece.offset,relative);for(int j=0;j<3;++j){relative[j]/=worldScale;scale[j]=piece.scale[j]/worldScale;}setScale(piece.transform,scale);float position[3]={firstPersonAnchor.position[0]+handPos[0]+relative[0],firstPersonAnchor.position[1]+handPos[1]+relative[1],firstPersonAnchor.position[2]+handPos[2]+relative[2]};
                float rotation[4];quatMultiply(handRotation,piece.rotation,rotation);float worldPos[3],worldRot[4];quatRotate(camRot,position,worldPos);for(int j=0;j<3;++j)worldPos[j]+=camPos[j];quatMultiply(camRot,rotation,worldRot);setPosition(piece.transform,worldPos);setRotation(piece.transform,worldRot);if(!piece.active){setGameObjectActive(piece.gameObject,true);piece.active=true;}}}
        else for(auto& piece:glove.pieces)if(piece.active){setGameObjectActive(piece.gameObject,false);piece.active=false;}
    }return true;
}
#include "first_person_ui.inc"
void syncFirstPerson(){
    const auto options=GetOptions();const bool requested=options.firstPerson&&options.mode==ViewMode::Immersive;
    if(!requested){if(firstPersonWasActive){hideSonic(false);updateGloves(false);LOG("Experimental first-person mode disabled; Sonic body renderers restored");}firstPersonWasActive=false;firstPersonAnchor.valid=false;return;}
    refreshFirstPersonAnchor();
    if(!firstPersonAnchor.valid){if(firstPersonWasActive){hideSonic(false);updateGloves(false);firstPersonWasActive=false;}return;}
    if(!updateGloves(true)||!hideSonic(true)){if(firstPersonWasActive)hideSonic(false);updateGloves(false);firstPersonWasActive=false;static bool failedLogged=false;if(!failedLogged){failedLogged=true;LOG("First-person visuals unavailable; Sonic remains visible and third-person view stays active");}return;}
    if(!firstPersonWasActive)LOG("Experimental first-person mode enabled; camera anchor, Sonic body hide and controller gloves active");firstPersonWasActive=true;
}
void canvasModeHook(void* canvas,int mode){
    const bool redirected=mode==0&&hudCamera&&canvasSetCamera&&canvasSetDistance;
    if(redirected){
        canvasSetCamera(canvas,hudCamera);canvasSetDistance(canvas,GetOptions().hudDistance);mode=1;
    }
    if(oldCanvasModeSetter)oldCanvasModeSetter(canvas,mode);
    if(redirected&&managedReady&&gettid()==unityThread){static auto root=icall<bool(*)(void*)>("UnityEngine.Canvas::get_isRootCanvas");if(root&&root(canvas))rememberHUDCanvas(canvas);}
}
bool bindApi(){
#define API(name,symbol) name=reinterpret_cast<decltype(name)>(dlsym(il,symbol));if(!name){LOG("Missing export %s",symbol);return false;}
    API(domain_get,"il2cpp_domain_get") API(domain_assemblies,"il2cpp_domain_get_assemblies")
    API(assembly_image,"il2cpp_assembly_get_image") API(class_from_name,"il2cpp_class_from_name")
    API(object_class,"il2cpp_object_get_class") API(class_field,"il2cpp_class_get_field_from_name")
    API(field_get,"il2cpp_field_get_value") API(static_get,"il2cpp_field_static_get_value")
    API(field_set,"il2cpp_field_set_value")
    API(field_set_object,"il2cpp_field_set_value_object")
    API(class_init,"il2cpp_runtime_class_init") API(class_method,"il2cpp_class_get_method_from_name")
    API(invoke,"il2cpp_runtime_invoke") API(class_type,"il2cpp_class_get_type")
    API(type_object,"il2cpp_type_get_object") API(array_length,"il2cpp_array_length")
    API(array_header_size,"il2cpp_array_object_header_size") API(string_length,"il2cpp_string_length")
    API(string_chars,"il2cpp_string_chars") API(resolve,"il2cpp_resolve_icall")
    API(object_new,"il2cpp_object_new") API(string_new,"il2cpp_string_new") API(gc_root,"il2cpp_gchandle_new")
    API(array_new,"il2cpp_array_new") API(thread_current,"il2cpp_thread_current") API(gc_free,"il2cpp_gchandle_free")
#undef API
    return true;
}
void startXR(){
    // Entered only through verified managed game callbacks, never Android onCreate.
    if(!thread_current||!thread_current())return;
    if(!managedReady){unityThread=gettid();managedReady=true;LOG("Managed runtime ready on game thread %d",int(unityThread));}
    if(gettid()!=unityThread)return;
    if(haveXR&&XRDisplayGraphicsReady())return;
    haveXR=false;
    if(tryingXR || !resolve)return;
    if(xrStartAttempted){
        auto running=icall<bool(*)(void*)>("UnityEngine.IntegratedSubsystem::IsRunning");
        bool subsystemRunning=xrStartedSubsystem&&running&&running(xrStartedSubsystem);
        bool graphicsReady=XRDisplayGraphicsReady();haveXR=subsystemRunning&&graphicsReady;
        static bool statusLogged=false;if(!statusLogged||haveXR){LOG("XR activation check: subsystemRunning=%d graphicsReady=%d",int(subsystemRunning),int(graphicsReady));statusLogged=true;}
        return;
    }
    unityThread=gettid();
    static timespec last{};timespec now{};clock_gettime(CLOCK_MONOTONIC,&now);
    if(last.tv_sec && now.tv_sec-last.tv_sec<2)return;last=now;
    installCrashRecorder();tryingXR=true;
    // Unity's SubsystemManager .cctor calls StaticConstructScriptingClassMap.
    // That native class map populates managed descriptor types. The descriptor
    // store alone only initializes its lists; reading it first always finds 0.
    auto manager=klass("UnityEngine","SubsystemManager");
    if(!manager){LOG("XR bootstrap: no SubsystemManager class");tryingXR=false;return;}
    class_init(manager);
    LOG("XR bootstrap: SubsystemManager class map initialized");
    auto store=klass("UnityEngine.SubsystemsImplementation","SubsystemDescriptorStore");
    if(!store){LOG("XR bootstrap: no descriptor store");tryingXR=false;return;}
    class_init(store);void* list=nullptr;
    auto sf=class_field(store,"s_IntegratedDescriptors");if(sf)static_get(sf,&list);
    auto count=field<int>(list,"_size");auto items=field<void*>(list,"_items");
    LOG("XR bootstrap: %d integrated descriptors",count);
    auto getid=icall<void*(*)(void*)>("UnityEngine.SubsystemDescriptorBindings::GetId");
    auto create=icall<void*(*)(void*)>("UnityEngine.SubsystemDescriptorBindings::Create");
    auto start=icall<void(*)(void*)>("UnityEngine.IntegratedSubsystem::Start");
    auto running=icall<bool(*)(void*)>("UnityEngine.IntegratedSubsystem::IsRunning");
    if(items && count>=0 && count<=128 && uintptr_t(count)<=array_length(items) && getid && create && start && running){
        for(int i=0;i<count;++i){
            auto desc=*reinterpret_cast<void**>(array_address(items,sizeof(void*),i));
            auto ptr=field<void*>(desc,"m_Ptr");
            if(!ptr || !eq(getid(ptr),"P06 Quest Display"))continue;
            auto instancePtr=create(ptr);
            if(!instancePtr){LOG("XR descriptor Create failed");break;}
            void* args[]={&instancePtr};
            auto sub=call(klass("UnityEngine","SubsystemManager"),"GetIntegratedSubsystemByPtr",nullptr,args,1);
            if(!sub){LOG("XR managed subsystem missing after Create");break;}
            if(!setObjectField(sub,"m_SubsystemDescriptor",desc)){LOG("XR descriptor reference assignment failed");break;}
            LOG("XR descriptor managed reference verified by read-back");
            xrStartedSubsystem=sub;xrStartAttempted=true;start(sub);bool subsystemRunning=running(sub);bool graphicsReady=XRDisplayGraphicsReady();haveXR=subsystemRunning&&graphicsReady;LOG("XR activation: subsystemRunning=%d graphicsReady=%d",int(subsystemRunning),int(graphicsReady));break;
        }
    }
    tryingXR=false;
}
uint32_t action(void* name){
    if(eq(name,"Button A"))return A;if(eq(name,"Button B"))return B;
    if(eq(name,"Button X"))return X;if(eq(name,"Button Y"))return Y;
    if(eq(name,"Start"))return Menu;if(eq(name,"Back"))return LClick;
    if(eq(name,"Left Bumper"))return LB;if(eq(name,"Right Bumper"))return RB;
    if(eq(name,"Left Trigger"))return LT;if(eq(name,"Right Trigger"))return RT;return 0;
}
using AxisFn=float(*)(void*,const Method*);
using ButtonFn=bool(*)(void*,const Method*);
AxisFn oldAxis=nullptr,oldAxisRaw=nullptr;
ButtonFn oldHeld=nullptr,oldDown=nullptr,oldUp=nullptr;
void (*oldTitleStart)(void*,const Method*)=nullptr;
float axis(void* name,const Method* m,AxisFn original){
    ++axisQueries;
    if(!haveXR)startXR();
    Controls c;{std::lock_guard<std::mutex> lock(inputMutex);c=controls;}
    if(!c.focused)return original(name,m);
    if(steeringScope){c.lx=steeringX;c.ly=steeringY;}
    // Hold right stick click for D-pad, suppressing camera input while selecting.
    bool dpad=(c.buttons&RClick)!=0;
    if(eq(name,"Left Stick X"))return c.lx;if(eq(name,"Left Stick Y"))return c.ly;
    if(eq(name,"Right Stick X"))return dpad?0:c.rx;if(eq(name,"Right Stick Y"))return dpad?0:c.ry;
    if(eq(name,"D-Pad X"))return dpad?c.rx:0;if(eq(name,"D-Pad Y"))return dpad?c.ry:0;
    return original(name,m);
}
float axisHook(void* n,const Method* m){return axis(n,m,oldAxis);}
float rawHook(void* n,const Method* m){return axis(n,m,oldAxisRaw);}
bool button(void* n,const Method* m,ButtonFn original,int mode){
    ++buttonQueries;
    if(!haveXR)startXR();uint32_t mask=action(n);if(!mask)return original(n,m);
    Controls c;uint32_t prev;{std::lock_guard<std::mutex> lock(inputMutex);c=controls;prev=previous;}
    if(!c.focused)return original(n,m);
    if(c.blocked)return false;
    if(mode==1)return (c.buttons&mask)&&!(prev&mask);
    if(mode==2)return !(c.buttons&mask)&&(prev&mask);
    return (c.buttons&mask)!=0;
}
bool heldHook(void* n,const Method* m){return button(n,m,oldHeld,0);}
bool downHook(void* n,const Method* m){return button(n,m,oldDown,1);}
bool upHook(void* n,const Method* m){return button(n,m,oldUp,2);}
void configureTitleCamera();
void titleStartHook(void* self,const Method* m){
    installCrashRecorder();oldTitleStart(self,m);LOG("TitleScreen.Start completed; starting XR");startXR();
    configureTitleCamera();rememberFrontEnd(self);
}
bool hook(uintptr_t base,const Binding& b,void* fn,void** orig,const char* name){
    void* p=reinterpret_cast<void*>(base+b.rva);
    if(memcmp(p,b.bytes,16)){LOG("Rejected %s: prologue mismatch",name);return false;}
    auto r=DobbyHook(p,reinterpret_cast<dobby_dummy_func_t>(fn),reinterpret_cast<dobby_dummy_func_t*>(orig));LOG("Hook %s RVA=%lx result=%d",name,(unsigned long)b.rva,r);return r==0;
}
void* selectGameCamera(){
    static auto main=icall<void*(*)()>("UnityEngine.Camera::get_main");
    static auto count=icall<int(*)()>("UnityEngine.Camera::GetAllCamerasCount");
    static auto fill=icall<int(*)(void*)>("UnityEngine.Camera::GetAllCamerasImpl");
    static auto target=icall<void*(*)(void*)>("UnityEngine.Camera::get_targetTexture");
    static auto name=icall<void*(*)(void*)>("UnityEngine.Object::GetName");
    if(main){auto c=main();if(c&&c!=hudCamera)return c;}
    if(!count||!fill||!target||!name)return nullptr;
    static void* cameras=nullptr;
    if(!cameras){auto ck=klass("UnityEngine","Camera");if(!ck)return nullptr;cameras=array_new(ck,128);if(!cameras)return nullptr;gc_root(cameras,false);}
    int n=count();if(n<1||n>128)return nullptr;n=fill(cameras);if(n<1||n>128)return nullptr;
    void* fallback=nullptr;
    for(int i=0;i<n;++i){auto c=*reinterpret_cast<void**>(array_address(cameras,sizeof(void*),i));if(!c||c==hudCamera||target(c))continue;
        if(eq(name(c),"OutlineCamera"))return c;
        if(!fallback)fallback=c;
    }return fallback;
}
#include "gameplay_hooks.inc"
#include "animated_uv.inc"
#include "gesture_bridge.inc"
#include "menu_rendering.inc"
#include "visibility.inc"
#include "save_compatibility.inc"
void* (*oldDlopen)(const char*,int)=nullptr;
void* (*oldExt)(const char*,int,const android_dlextinfo*)=nullptr;
void loaded(const char* name,void* h){if(h&&name&&strstr(name,"libil2cpp.so"))InstallGameHooks(h);}
void* dlopenHook(const char* p,int flags){auto h=oldDlopen(p,flags);loaded(p,h);return h;}
void* extHook(const char* p,int flags,const android_dlextinfo* info){auto h=oldExt(p,flags,info);loaded(p,h);return h;}
}
void PublishMotionFrame(const MotionFrame& f){std::lock_guard<std::mutex> lock(motionMutex);motionFrame=f;}
bool GameAllowsHaptics(){return gameHapticsAllowed.load();}
void PublishControls(const Controls& raw){const Controls c=applyGestures(raw);std::lock_guard<std::mutex> lock(inputMutex);previous=controls.focused&&c.focused&&!controls.blocked&&!c.blocked?controls.buttons:c.buttons;controls=c;}
void PublishTrackedControllerPose(int hand,bool valid,const float position[3],const float rotation[4]){
    if(hand<0||hand>1)return;std::lock_guard<std::mutex> lock(trackedHandsMutex);auto& pose=trackedHands[hand];pose.valid=valid&&position&&rotation;
    if(!pose.valid)return;std::copy(position,position+3,pose.position);std::copy(rotation,rotation+4,pose.rotation);
}
bool GetFirstPersonCameraAnchor(float position[3],float rotation[4]){
    if(!firstPersonWasActive||!firstPersonAnchor.valid||!position||!rotation)return false;
    std::copy(firstPersonAnchor.position,firstPersonAnchor.position+3,position);std::copy(firstPersonAnchor.rotation,firstPersonAnchor.rotation+4,rotation);return true;
}
void SyncFirstPersonVisuals(){if(resolve&&gettid()==unityThread){syncFirstPerson();syncFirstPersonHUD(firstPersonWasActive);syncFirstPersonNearClip(firstPersonWasActive);}}
void LogBridgeStats(){LOG("Game input queries: axes=%llu buttons=%llu",(unsigned long long)axisQueries.load(),(unsigned long long)buttonQueries.load());}
void SyncHudCamera(){
    if(!resolve||!hudCamera||gettid()!=unityThread)return;
    syncWorldVisibility();
    static auto copy=icall<void(*)(void*,void*)>("UnityEngine.Camera::CopyFrom");
    static auto transform=icall<void*(*)(void*)>("UnityEngine.Component::get_transform");
    static auto getPos=icall<void(*)(void*,float*)>("UnityEngine.Transform::get_position_Injected");
    static auto getRot=icall<void(*)(void*,float*)>("UnityEngine.Transform::get_rotation_Injected");
    static auto setPos=icall<void(*)(void*,const float*)>("UnityEngine.Transform::set_position_Injected");
    static auto setRot=icall<void(*)(void*,const float*)>("UnityEngine.Transform::set_rotation_Injected");
    static auto mask=icall<int(*)(void*)>("UnityEngine.Camera::get_cullingMask");
    static auto setMask=icall<void(*)(void*,int)>("UnityEngine.Camera::set_cullingMask");
    static auto clear=icall<void(*)(void*,int)>("UnityEngine.Camera::set_clearFlags");
    static auto depth=icall<void(*)(void*,float)>("UnityEngine.Camera::set_depth");
    static auto near=icall<void(*)(void*,float)>("UnityEngine.Camera::set_nearClipPlane");
    static auto far=icall<void(*)(void*,float)>("UnityEngine.Camera::set_farClipPlane");
    static auto renderingPath=icall<void(*)(void*,int)>("UnityEngine.Camera::set_renderingPath");
    static auto hdr=icall<void(*)(void*,bool)>("UnityEngine.Camera::set_allowHDR");
    static auto force=icall<void(*)(void*,bool)>("UnityEngine.Camera::set_forceIntoRenderTexture");
    static auto enabled=icall<void(*)(void*,bool)>("UnityEngine.Behaviour::set_enabled");
    if(!copy||!transform||!getPos||!getRot||!setPos||!setRot||!mask||!setMask||!clear||!depth||!near||!far||!renderingPath||!hdr||!enabled)return;
    auto main=selectGameCamera();if(!main||main==hudCamera){enabled(hudCamera,false);return;}
    copy(hudCamera,main);setMask(hudCamera,hudLayerMask);clear(hudCamera,3);depth(hudCamera,10000.f);near(hudCamera,.05f);far(hudCamera,50.f);
    renderingPath(hudCamera,1);hdr(hudCamera,false);enabled(hudCamera,true);
    if(force)force(hudCamera,false); // CopyFrom must not inherit a desktop intermediate blit.
    setMask(main,uint32_t(mask(main))&~hudLayerMask);
    float pos[3],rot[4];auto source=transform(main),target=transform(hudCamera);
    if(source&&target){getPos(source,pos);getRot(source,rot);setPos(target,pos);setRot(target,rot);}
}
void GameMainTick(){
    if(!managedReady||gettid()!=unityThread||!resolve)return;
    static auto getTimeScale=icall<float(*)()>("UnityEngine.Time::get_timeScale");
    static auto setTimeScale=icall<void(*)(float)>("UnityEngine.Time::set_timeScale");
    static bool pausedByVR=false;static float savedTimeScale=1.f;
    if(getTimeScale&&setTimeScale){
        if(VRMenuOpen()){if(!pausedByVR){savedTimeScale=getTimeScale();pausedByVR=true;}setTimeScale(0.f);}
        else if(pausedByVR){setTimeScale(std::isfinite(savedTimeScale)?savedTimeScale:1.f);pausedByVR=false;}
    }
    // Canvas discovery is deliberately infrequent, outside rendering callbacks.
    SyncHudCamera();
    gameHapticTick();
    static int tick=0;++tick;
    if(tick%120==1)discoverWaterTriggers();
    if(tick%600==0&&skippedUVRows)LOG("AnimatedUV invalid material/property rows skipped: %llu (valid rows still animate)",(unsigned long long)skippedUVRows);
    if(tick!=1&&tick%60!=0)return;
    auto camera=icall<void*(*)()>("UnityEngine.Camera::get_main");
    auto find=icall<void*(*)(void*,bool)>("UnityEngine.Object::FindObjectsOfType");
    auto mode=icall<int(*)(void*)>("UnityEngine.Canvas::get_renderMode");
    auto setMode=icall<void(*)(void*,int)>("UnityEngine.Canvas::set_renderMode");
    auto setCamera=icall<void(*)(void*,void*)>("UnityEngine.Canvas::set_worldCamera");
    auto setDistance=icall<void(*)(void*,float)>("UnityEngine.Canvas::set_planeDistance");
    if(setCamera)canvasSetCamera=setCamera;if(setDistance)canvasSetDistance=setDistance;
    if(!canvasModeHookAttempted&&setMode){canvasModeHookAttempted=true;
        canvasModeHookInstalled=DobbyHook(reinterpret_cast<void*>(setMode),reinterpret_cast<dobby_dummy_func_t>(canvasModeHook),reinterpret_cast<dobby_dummy_func_t*>(&oldCanvasModeSetter))==0;
        LOG("Canvas overlay-to-stereo interception: %s",canvasModeHookInstalled?"installed":"unavailable; periodic conversion remains active");
    }
    auto isRoot=icall<bool(*)(void*)>("UnityEngine.Canvas::get_isRootCanvas");
    auto gameObject=icall<void*(*)(void*)>("UnityEngine.Component::get_gameObject");
    auto getName=icall<void*(*)(void*)>("UnityEngine.Object::GetName");
    auto enabled=icall<void(*)(void*,bool)>("UnityEngine.Behaviour::set_enabled");
    auto ck=klass("UnityEngine","Canvas");
    if(!camera||!find||!mode||!setMode||!setCamera||!setDistance||!isRoot||!ck)return;
    auto cam=selectGameCamera();if(!cam)return;
    if(!hudCamera){
        auto create=icall<void(*)(void*,void*)>("UnityEngine.GameObject::Internal_CreateGameObject");
        auto add=icall<void*(*)(void*,void*)>("UnityEngine.GameObject::Internal_AddComponentWithType");
        auto persist=icall<void(*)(void*)>("UnityEngine.Object::DontDestroyOnLoad");
        auto goClass=klass("UnityEngine","GameObject"),cameraClass=klass("UnityEngine","Camera");
        if(create&&add&&persist&&goClass&&cameraClass){auto go=object_new(goClass);create(go,string_new("P06Quest HUD Camera"));hudCamera=add(go,type_object(class_type(cameraClass)));
            if(hudCamera){gc_root(go,false);gc_root(hudCamera,false);persist(go);LOG("Dedicated stereo HUD camera created (depth clear; no scene postprocessing)");}}
    }
    if(hudCamera){SyncHudCamera();cam=hudCamera;}
    auto all=find(type_object(class_type(ck)),false);if(!all)return;
    auto n=array_length(all);if(n>1024)return;
    int converted=0;
    const float distance=GetOptions().hudDistance;
    for(uintptr_t i=0;i<n;++i){auto canvas=*reinterpret_cast<void**>(array_address(all,sizeof(void*),i));
        if(!canvas||!isRoot(canvas))continue;
        if(gameObject&&getName&&enabled){auto go=gameObject(canvas);auto name=go?getName(go):nullptr;
            if(eq(name,"CF2-Canvas")||eq(name,"CF2-Gamepad-Notifier")){enabled(canvas,false);continue;}}
        if(mode(canvas)==0){setCamera(canvas,cam);setDistance(canvas,distance);setMode(canvas,1);rememberHUDCanvas(canvas);++converted;}
        else if(mode(canvas)==1){setCamera(canvas,cam);setDistance(canvas,distance);rememberHUDCanvas(canvas);}}
    if(converted)LOG("Converted %d overlay canvases to stereo camera-space UI",converted);
}
void InstallGameHooks(void* lib){
    bool expected=false;if(!installed.compare_exchange_strong(expected,true))return;
    il=lib;if(!bindApi())return;
    Dl_info info{};if(!dladdr(dlsym(il,"il2cpp_domain_get"),&info)||!info.dli_fbase)return;
    uintptr_t base=reinterpret_cast<uintptr_t>(info.dli_fbase);
    LOG("IL2CPP load base=%p",info.dli_fbase);
    // Native address verification only here. Managed metadata is not initialized yet.
    bool ok=true;
    ok &= hook(base,kAxis,(void*)axisHook,(void**)&oldAxis,"CF2Input.GetAxis");
    ok &= hook(base,kAxisRaw,(void*)rawHook,(void**)&oldAxisRaw,"CF2Input.GetAxisRaw");
    ok &= hook(base,kHeld,(void*)heldHook,(void**)&oldHeld,"CF2Input.GetButton");
    ok &= hook(base,kDown,(void*)downHook,(void**)&oldDown,"CF2Input.GetButtonDown");
    ok &= hook(base,kUp,(void*)upHook,(void**)&oldUp,"CF2Input.GetButtonUp");
    ok &= hook(base,kTitleStart,(void*)titleStartHook,(void**)&oldTitleStart,"TitleScreen.Start");
    hook(base,kSonicFixed,(void*)sonicFixedHook,(void**)&oldSonicFixed,"SonicNew.FixedUpdate");
    hook(base,kWaterTrigger,(void*)waterTriggerHook,(void**)&oldWaterTrigger,"WaterSlider.OnTriggerEnter");
    hook(base,kBoosterTrigger,(void*)boosterTriggerHook,(void**)&oldBoosterTrigger,"WaterslideBooster.OnTriggerEnter");
    hook(base,kWaterEnter,(void*)waterEnterHook,(void**)&oldWaterEnter,"SonicNew.OnWaterSlideEnter");
    hook(base,kAddRing,(void*)addRingHook,(void**)&oldAddRing,"PlayerBase.AddRing");
    hook(base,kAcceleration,(void*)accelerationHook,(void**)&oldAcceleration,"PlayerBase.AccelerationSystem");
    bool rotateReady=hook(base,kRotatePlayer,(void*)rotatePlayerHook,(void**)&oldRotatePlayer,"PlayerBase.RotatePlayer");
    bool slopeReady=hook(base,kSlopePhysics,(void*)slopePhysicsHook,(void**)&oldSlopePhysics,"PlayerBase.SlopePhysics");
    nativeSteeringInstalled=rotateReady&&slopeReady;
    LOG("Native-consumer head steering: %s",nativeSteeringInstalled?"installed":"previous state-gated steering retained");
    hook(base,kMenuStart,(void*)menuStartHook,(void**)&oldMenuStart,"MainMenu.Start");
    hook(base,kBackgroundUpdate,(void*)backgroundUpdateHook,(void**)&oldBackgroundUpdate,"BackgroundVideo.UpdateVideo");
    hook(base,kGaugeStart,(void*)gaugeStartHook,(void**)&oldGaugeStart,"GaugeController.Start");
    hook(base,kGaugeUpdate,(void*)gaugeUpdateHook,(void**)&oldGaugeUpdate,"GaugeController.Update");
    hook(base,kSkyboxStart,(void*)skyboxStartHook,(void**)&oldSkyboxStart,"SkyboxModel.Start");
    hook(base,kAnimatedUV,(void*)animatedUVHook,(void**)&oldAnimatedUV,"AnimatedUV.Update");
    bool saveUI=hook(base,kSaveSlotSetup,(void*)saveSlotSetupHook,(void**)&oldSaveSlotSetup,"SaveSlotUI.SetUp");
    bool saveList=hook(base,kSaveStarter,(void*)saveStarterHook,(void**)&oldSaveStarter,"TitleScreen.StateStarterStart");
    bool saveVersion=hook(base,kApplicationVersion,(void*)applicationVersionHook,(void**)&oldApplicationVersion,"Application.get_version");
    saveCompatibilityInstalled=saveUI&&saveList&&saveVersion;
    LOG("Stable save compatibility: %s",saveCompatibilityInstalled?"installed; schema 1, independent of APK version":"INCOMPLETE; native version behavior retained");
    LOG("Pinned APK hooks installed: %s",ok?"all":"INCOMPLETE");
}
void BeginLoaderHooks(){
    auto d=dlsym(RTLD_DEFAULT,"dlopen");auto e=dlsym(RTLD_DEFAULT,"android_dlopen_ext");
    if(d)LOG("dlopen interception=%d",DobbyHook(d,(dobby_dummy_func_t)dlopenHook,(dobby_dummy_func_t*)&oldDlopen));
    if(e)LOG("android_dlopen_ext interception=%d",DobbyHook(e,(dobby_dummy_func_t)extHook,(dobby_dummy_func_t*)&oldExt));
    if(oldDlopen){auto h=oldDlopen("libil2cpp.so",RTLD_NOW|RTLD_NOLOAD);if(h)InstallGameHooks(h);}
}
extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM* vm,void*){g_vm=vm;return JNI_VERSION_1_6;}
extern "C" JNIEXPORT void JNICALL Java_com_p06_quest_QuestActivity_nativePrepare(JNIEnv* env,jclass,jobject activity,jstring directory,jint fd){
    if(fd>=0)logFd=dup(fd);
    g_activity=env->NewGlobalRef(activity);const char* dir=env->GetStringUTFChars(directory,nullptr);InitOptions(dir);env->ReleaseStringUTFChars(directory,dir);
    LOG("P06 Quest candidate 0.1.13 / Unity 2022.3.62f1 / ARM64");installCrashRecorder();
    LOG("System library loading is untouched; waiting for Unity activity creation");
}

extern "C" JNIEXPORT void JNICALL Java_com_p06_quest_QuestActivity_nativeAttach(JNIEnv*,jclass){
    LOG("Attaching game hooks after Unity activity creation (no loader interception)");
    auto lib=dlopen("libil2cpp.so",RTLD_NOW|RTLD_NOLOAD);
    if(!lib){LOG("IL2CPP not loaded after Unity creation: %s",dlerror());return;}
    // Keep the handle for API bindings. No process-wide dlopen detours are installed.
    InstallGameHooks(lib);
    for(const char* name:{"libunity.so","libp06quest.so"}){
        auto h=dlopen(name,RTLD_NOW|RTLD_NOLOAD);
        if(h){LOG("Loaded module %s handle=%p",name,h);dlclose(h);}
    }
}
