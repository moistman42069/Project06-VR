// Conservative compatibility for views displaced from the authored camera.
// Keep the provider's per-eye frustum culling; bypass only baked occlusion.
void syncWorldVisibility(){
    static auto count=icall<int(*)()>("UnityEngine.Camera::GetAllCamerasCount");
    static auto fill=icall<int(*)(void*)>("UnityEngine.Camera::GetAllCamerasImpl");
    static auto target=icall<void*(*)(void*)>("UnityEngine.Camera::get_targetTexture");
    static auto get=icall<bool(*)(void*)>("UnityEngine.Camera::get_useOcclusionCulling");
    static auto set=icall<void(*)(void*,bool)>("UnityEngine.Camera::set_useOcclusionCulling");
    if(!count||!fill||!target||!get||!set)return;
    static void* cameras=nullptr;
    if(!cameras){auto ck=klass("UnityEngine","Camera");if(!ck)return;cameras=array_new(ck,128);if(!cameras)return;gc_root(cameras,false);}
    int n=count();if(n<1||n>128)return;n=fill(cameras);if(n<1||n>128)return;
    for(int i=0;i<n;++i){auto c=*reinterpret_cast<void**>(array_address(cameras,sizeof(void*),i));
        if(alive(c)&&c!=hudCamera&&!target(c)&&get(c)){set(c,false);LOG("World camera %p: baked occlusion disabled; stereo frustum culling retained",c);}}
}
void (*oldGaugeStart)(void*,const Method*)=nullptr;
void (*oldGaugeUpdate)(void*,const Method*)=nullptr;
void (*oldSkyboxStart)(void*,const Method*)=nullptr;
bool bindBillboardCamera(void* self,const char* fieldName){
    if(!managedReady||gettid()!=unityThread)return false;
    if(alive(field<void*>(self,fieldName)))return true;
    auto camera=selectGameCamera();
    static auto transform=icall<void*(*)(void*)>("UnityEngine.Component::get_transform");
    auto f=class_field(object_class(self),fieldName);if(!alive(camera)||!transform||!f)return false;
    auto t=transform(camera);if(!alive(t))return false;field_set(self,f,&t);return true;
}
void gaugeStartHook(void* self,const Method* method){
    if(!managedReady||gettid()!=unityThread){oldGaugeStart(self,method);return;}
    bool ready=bindBillboardCamera(self,"MainCamera");LOG("Gauge camera binding: %s",ready?"ready":"deferred until camera available");
}
void gaugeUpdateHook(void* self,const Method* method){
    if(!managedReady||gettid()!=unityThread){oldGaugeUpdate(self,method);return;}
    if(bindBillboardCamera(self,"MainCamera")&&alive(field<void*>(self,"_renderer")))oldGaugeUpdate(self,method);
}
void skyboxStartHook(void* self,const Method* method){
    if(!managedReady||gettid()!=unityThread){oldSkyboxStart(self,method);return;}
    bool ready=bindBillboardCamera(self,"PlayerCamera");LOG("Skybox camera binding: %s",ready?"ready":"unavailable; native null guard retained");
}
