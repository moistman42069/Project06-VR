// Menu-only compatibility. Do not alter gameplay cameras, water shaders or post FX.
void (*oldMenuStart)(void*,const Method*)=nullptr;
void (*oldBackgroundUpdate)(void*,const Method*)=nullptr;
void configureMenuCameras(){
    static auto find=icall<void*(*)(void*,bool)>("UnityEngine.Object::FindObjectsOfType");
    static auto name=icall<void*(*)(void*)>("UnityEngine.Object::GetName");
    static auto path=icall<void(*)(void*,int)>("UnityEngine.Camera::set_renderingPath");
    static auto target=icall<void*(*)(void*)>("UnityEngine.Camera::get_targetTexture");
    static auto eye=icall<void(*)(void*,int)>("UnityEngine.Camera::set_stereoTargetEye");
    static auto force=icall<void(*)(void*,bool)>("UnityEngine.Camera::set_forceIntoRenderTexture");
    auto ck=klass("UnityEngine","Camera");if(!ck||!find||!name||!path||!target||!eye||!force)return;
    auto cameras=find(type_object(class_type(ck)),false);if(!cameras||array_length(cameras)>64)return;
    for(uintptr_t i=0;i<array_length(cameras);++i){auto camera=*reinterpret_cast<void**>(array_address(cameras,sizeof(void*),i));if(!alive(camera))continue;
        if(eq(name(camera),"Model Camera")&&target(camera)){
            path(camera,1);eye(camera,0);force(camera,false);
            LOG("Main menu model camera: forward mono render-to-texture; shared UI preview");
        }else if(eq(name(camera),"Main Camera")&&!target(camera)){
            path(camera,1);eye(camera,3);force(camera,false);
            LOG("Main menu camera: forward stereo, no forced intermediate target");
        }
    }
}
void menuStartHook(void* self,const Method* method){
    oldMenuStart(self,method);
    if(managedReady&&gettid()==unityThread)configureMenuCameras();
}
void backgroundUpdateHook(void* self,const Method* method){
    oldBackgroundUpdate(self,method);
    if(!managedReady||gettid()!=unityThread)return;
    auto backgrounds=field<void*>(self,"BGs");
    if(!backgrounds||array_length(backgrounds)<2||array_length(backgrounds)>16)return;
    auto animated=*reinterpret_cast<void**>(array_address(backgrounds,sizeof(void*),0));
    auto still=*reinterpret_cast<void**>(array_address(backgrounds,sizeof(void*),1));
    if(!alive(still))return; // Title/cutscene videos without this menu fallback remain stock.
    auto video=field<void*>(self,"VideoPlayer");
    if(alive(video))call(object_class(video),"Stop",video,nullptr,0);
    if(alive(animated))setGameObjectActive(animated,false);
    auto videoObject=field<void*>(self,"BGVideo");if(alive(videoObject))setGameObjectActive(videoObject,false);
    setGameObjectActive(still,true);
    LOG("Main menu background: authored static fallback; animated video excluded from stereo UI");
}
