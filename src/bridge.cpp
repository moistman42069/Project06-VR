#include "bridge.h"
#include "bindings.h"
#include "vr_options.h"
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
uint32_t hudLayerMask=1u<<5;
std::atomic<bool> installed{false};
std::atomic<uint64_t> axisQueries{0},buttonQueries{0};
std::mutex inputMutex;
Controls controls;
uint32_t previous=0;
bool haveXR=false,tryingXR=false;

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
bool eq(void* str,const char* text) {
    if(!str || !string_length || !string_chars) return false;
    auto n=string_length(str); if(n<0 || n>128 || size_t(n)!=strlen(text))return false;
    auto s=string_chars(str);for(int i=0;i<n;++i)if(s[i]!=uint8_t(text[i]))return false;return true;
}
void* klass(const char* ns,const char* name) {
    size_t n=0;auto a=domain_assemblies(domain_get(),&n);if(n>1024)return nullptr;
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
template<typename T>T icall(const char* n){auto f=resolve(n);if(!f)LOG("Missing Unity icall: %s",n);return reinterpret_cast<T>(f);}
bool bindApi(){
#define API(name,symbol) name=reinterpret_cast<decltype(name)>(dlsym(il,symbol));if(!name){LOG("Missing export %s",symbol);return false;}
    API(domain_get,"il2cpp_domain_get") API(domain_assemblies,"il2cpp_domain_get_assemblies")
    API(assembly_image,"il2cpp_assembly_get_image") API(class_from_name,"il2cpp_class_from_name")
    API(object_class,"il2cpp_object_get_class") API(class_field,"il2cpp_class_get_field_from_name")
    API(field_get,"il2cpp_field_get_value") API(static_get,"il2cpp_field_static_get_value")
    API(field_set,"il2cpp_field_set_value")
    API(class_init,"il2cpp_runtime_class_init") API(class_method,"il2cpp_class_get_method_from_name")
    API(invoke,"il2cpp_runtime_invoke") API(class_type,"il2cpp_class_get_type")
    API(type_object,"il2cpp_type_get_object") API(array_length,"il2cpp_array_length")
    API(array_header_size,"il2cpp_array_object_header_size") API(string_length,"il2cpp_string_length")
    API(string_chars,"il2cpp_string_chars") API(resolve,"il2cpp_resolve_icall")
    API(object_new,"il2cpp_object_new") API(string_new,"il2cpp_string_new") API(gc_root,"il2cpp_gchandle_new")
    API(array_new,"il2cpp_array_new")
#undef API
    return true;
}
void startXR(){
    if(haveXR || tryingXR || !resolve)return;
    unityThread=gettid();
    static timespec last{};timespec now{};clock_gettime(CLOCK_MONOTONIC,&now);
    if(last.tv_sec && now.tv_sec-last.tv_sec<2)return;last=now;
    installCrashRecorder();tryingXR=true;
    auto store=klass("UnityEngine.SubsystemsImplementation","SubsystemDescriptorStore");
    if(!store){LOG("XR bootstrap: no descriptor store");tryingXR=false;return;}
    class_init(store);void* list=nullptr;
    auto sf=class_field(store,"s_IntegratedDescriptors");if(sf)static_get(sf,&list);
    auto count=field<int>(list,"_size");auto items=field<void*>(list,"_items");
    LOG("XR bootstrap: %d descriptors",count);
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
            if(auto descriptorField=class_field(object_class(sub),"m_SubsystemDescriptor"))field_set(sub,descriptorField,&desc);
            start(sub);haveXR=running(sub);LOG("XR display running=%d",int(haveXR));break;
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
void titleStartHook(void* self,const Method* m){
    installCrashRecorder();oldTitleStart(self,m);LOG("TitleScreen.Start completed; starting XR");startXR();
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
void* (*oldDlopen)(const char*,int)=nullptr;
void* (*oldExt)(const char*,int,const android_dlextinfo*)=nullptr;
void loaded(const char* name,void* h){if(h&&name&&strstr(name,"libil2cpp.so"))InstallGameHooks(h);}
void* dlopenHook(const char* p,int flags){auto h=oldDlopen(p,flags);loaded(p,h);return h;}
void* extHook(const char* p,int flags,const android_dlextinfo* info){auto h=oldExt(p,flags,info);loaded(p,h);return h;}
}
void PublishControls(const Controls& c){std::lock_guard<std::mutex> lock(inputMutex);previous=controls.focused&&c.focused&&!controls.blocked&&!c.blocked?controls.buttons:c.buttons;controls=c;}
void LogBridgeStats(){LOG("Game input queries: axes=%llu buttons=%llu",(unsigned long long)axisQueries.load(),(unsigned long long)buttonQueries.load());}
void SyncHudCamera(){
    if(!resolve||!hudCamera||gettid()!=unityThread)return;
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
    static auto enabled=icall<void(*)(void*,bool)>("UnityEngine.Behaviour::set_enabled");
    if(!copy||!transform||!getPos||!getRot||!setPos||!setRot||!mask||!setMask||!clear||!depth||!near||!far||!renderingPath||!hdr||!enabled)return;
    auto main=selectGameCamera();if(!main||main==hudCamera){enabled(hudCamera,false);return;}
    copy(hudCamera,main);setMask(hudCamera,hudLayerMask);clear(hudCamera,3);depth(hudCamera,10000.f);near(hudCamera,.05f);far(hudCamera,50.f);
    renderingPath(hudCamera,1);hdr(hudCamera,false);enabled(hudCamera,true);
    setMask(main,uint32_t(mask(main))&~hudLayerMask);
    float pos[3],rot[4];auto source=transform(main),target=transform(hudCamera);
    if(source&&target){getPos(source,pos);getRot(source,rot);setPos(target,pos);setRot(target,rot);}
}
void GameMainTick(){
    if(!resolve)return;
    static auto getTimeScale=icall<float(*)()>("UnityEngine.Time::get_timeScale");
    static auto setTimeScale=icall<void(*)(float)>("UnityEngine.Time::set_timeScale");
    static bool pausedByVR=false;static float savedTimeScale=1.f;
    if(getTimeScale&&setTimeScale){
        if(VRMenuOpen()){if(!pausedByVR){savedTimeScale=getTimeScale();pausedByVR=true;}setTimeScale(0.f);}
        else if(pausedByVR){setTimeScale(std::isfinite(savedTimeScale)?savedTimeScale:1.f);pausedByVR=false;}
    }
    // Canvas discovery is deliberately infrequent, outside rendering callbacks.
    SyncHudCamera();
    static int tick=0;if((tick++%60)!=0)return;
    auto camera=icall<void*(*)()>("UnityEngine.Camera::get_main");
    auto find=icall<void*(*)(void*,bool)>("UnityEngine.Object::FindObjectsOfType");
    auto mode=icall<int(*)(void*)>("UnityEngine.Canvas::get_renderMode");
    auto setMode=icall<void(*)(void*,int)>("UnityEngine.Canvas::set_renderMode");
    auto setCamera=icall<void(*)(void*,void*)>("UnityEngine.Canvas::set_worldCamera");
    auto setDistance=icall<void(*)(void*,float)>("UnityEngine.Canvas::set_planeDistance");
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
        if(mode(canvas)==0){setCamera(canvas,cam);setDistance(canvas,distance);setMode(canvas,1);++converted;}
        else if(mode(canvas)==1){setCamera(canvas,cam);setDistance(canvas,distance);}}
    if(converted)LOG("Converted %d overlay canvases to stereo camera-space UI",converted);
}
void InstallGameHooks(void* lib){
    bool expected=false;if(!installed.compare_exchange_strong(expected,true))return;
    il=lib;if(!bindApi())return;
    Dl_info info{};if(!dladdr(dlsym(il,"il2cpp_domain_get"),&info)||!info.dli_fbase)return;
    uintptr_t base=reinterpret_cast<uintptr_t>(info.dli_fbase);
    LOG("IL2CPP load base=%p",info.dli_fbase);
    bool ok=true;
    ok &= hook(base,kAxis,(void*)axisHook,(void**)&oldAxis,"CF2Input.GetAxis");
    ok &= hook(base,kAxisRaw,(void*)rawHook,(void**)&oldAxisRaw,"CF2Input.GetAxisRaw");
    ok &= hook(base,kHeld,(void*)heldHook,(void**)&oldHeld,"CF2Input.GetButton");
    ok &= hook(base,kDown,(void*)downHook,(void**)&oldDown,"CF2Input.GetButtonDown");
    ok &= hook(base,kUp,(void*)upHook,(void**)&oldUp,"CF2Input.GetButtonUp");
    ok &= hook(base,kTitleStart,(void*)titleStartHook,(void**)&oldTitleStart,"TitleScreen.Start");
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
    LOG("P06 Quest candidate 0.1.0 / Unity 2022.3.62f1 / ARM64");BeginLoaderHooks();
}
