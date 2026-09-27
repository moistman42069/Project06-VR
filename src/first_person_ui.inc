// Only canvases that the bridge owns as screen-space HUD are adjusted here.
// Existing world-space gameplay UI is untouched.
void* frontEndOwner=nullptr;uint32_t frontEndRoot=0;
void rememberFrontEnd(void* owner){
    if(!managedReady||gettid()!=unityThread||!alive(owner))return;
    if(frontEndRoot)gc_free(frontEndRoot);frontEndOwner=owner;frontEndRoot=gc_root(owner,false);
}
struct HUDCanvas { void* canvas=nullptr;uint32_t root=0;bool world=false;float scale[3]{1,1,1};float distance=2; };
std::vector<HUDCanvas> hudCanvases;
void rememberHUDCanvas(void* canvas){
    if(!alive(canvas))return;
    if(std::none_of(hudCanvases.begin(),hudCanvases.end(),[&](const HUDCanvas& c){return c.canvas==canvas;}))hudCanvases.push_back({canvas,gc_root(canvas,false)});
}
void syncFirstPersonHUD(bool firstPerson){
    static auto transform=icall<void*(*)(void*)>("UnityEngine.Component::get_transform");
    static auto getPos=icall<void(*)(void*,float*)>("UnityEngine.Transform::get_position_Injected");
    static auto getRot=icall<void(*)(void*,float*)>("UnityEngine.Transform::get_rotation_Injected");
    static auto getScale=icall<void(*)(void*,float*)>("UnityEngine.Transform::get_localScale_Injected");
    static auto setPos=icall<void(*)(void*,const float*)>("UnityEngine.Transform::set_position_Injected");
    static auto setRot=icall<void(*)(void*,const float*)>("UnityEngine.Transform::set_rotation_Injected");
    static auto setScale=icall<void(*)(void*,const float*)>("UnityEngine.Transform::set_localScale_Injected");
    static auto getMode=icall<int(*)(void*)>("UnityEngine.Canvas::get_renderMode");
    static auto setMode=icall<void(*)(void*,int)>("UnityEngine.Canvas::set_renderMode");
    static auto getDistance=icall<float(*)(void*)>("UnityEngine.Canvas::get_planeDistance");
    if(!transform||!getPos||!getRot||!getScale||!setPos||!setRot||!setScale||!getMode||!setMode||!getDistance)return;
    const auto options=GetOptions();
    const bool frontEnd=alive(frontEndOwner);
    if(!frontEnd&&frontEndRoot){gc_free(frontEndRoot);frontEndRoot=0;frontEndOwner=nullptr;}
    const float distance=frontEnd?options.titleDistance:options.hudDistance;
    const float size=frontEnd?options.titleSize:options.hudSize;
    const float offsetX=frontEnd?options.titleX:options.hudX,offsetY=frontEnd?options.titleY:options.hudY;
    if(frontEnd)firstPerson=false;
    bool customHUD=options.hudX!=0||options.hudY!=0||options.hudSize!=1||options.hudFollowView;
    bool thirdPerson=options.mode==ViewMode::Immersive&&!firstPerson&&customHUD&&refreshFirstPersonAnchor();
    bool positioned=firstPerson||thirdPerson||frontEnd;
    float cameraPos[3]{},cameraRot[4]{0,0,0,1},worldPos[3]{},worldRot[4]{0,0,0,1};
    if(positioned){
        auto camera=firstPerson?firstPersonAnchor.cameraTransform:(alive(hudCamera)?transform(hudCamera):nullptr);if(!alive(camera))return;
        getPos(camera,cameraPos);getRot(camera,cameraRot);
        float forward[3]={offsetX,offsetY,distance},local[3];
        float headRotation[4]{0,0,0,1};
        if(!frontEnd&&options.hudFollowView&&options.mode==ViewMode::Immersive){
            MotionFrame tracked;{std::lock_guard<std::mutex> lock(motionMutex);tracked=motionFrame;}
            if(tracked.focused&&tracked.head.valid){
                float shifted[3];P06HUDFromHead(forward,tracked.head.p,tracked.head.q,options.worldScale,options.positionalTracking,shifted);
                std::copy(shifted,shifted+3,forward);std::copy(tracked.head.q,tracked.head.q+4,headRotation);
            }
        }
        if(firstPerson){quatRotate(firstPersonAnchor.rotation,forward,local);for(int i=0;i<3;++i)local[i]+=firstPersonAnchor.position[i];quatMultiply(cameraRot,firstPersonAnchor.rotation,worldRot);}
        else{std::copy(forward,forward+3,local);std::copy(cameraRot,cameraRot+4,worldRot);}
        float followedRotation[4];quatMultiply(worldRot,headRotation,followedRotation);std::copy(followedRotation,followedRotation+4,worldRot);
        quatRotate(cameraRot,local,worldPos);for(int i=0;i<3;++i)worldPos[i]+=cameraPos[i];
    }
    for(auto it=hudCanvases.begin();it!=hudCanvases.end();){
        auto& c=*it;if(!alive(c.canvas)){gc_free(c.root);it=hudCanvases.erase(it);continue;}
        auto tr=transform(c.canvas);if(!alive(tr)){++it;continue;}
        if(positioned){
            if(!c.world||getMode(c.canvas)!=2){getScale(tr,c.scale);c.distance=std::max(getDistance(c.canvas),.1f);setMode(c.canvas,2);c.world=true;}
            float scale[3];for(int i=0;i<3;++i)scale[i]=c.scale[i]*distance/c.distance*size;
            setScale(tr,scale);setPos(tr,worldPos);setRot(tr,worldRot);
        }else if(c.world){setScale(tr,c.scale);setMode(c.canvas,1);if(canvasSetCamera)canvasSetCamera(c.canvas,hudCamera);if(canvasSetDistance)canvasSetDistance(c.canvas,distance);c.world=false;}
        ++it;
    }
}
void syncFirstPersonNearClip(bool active){
    struct ClipState {void* camera=nullptr;uint32_t root=0;float saved=.3f;};static ClipState states[2];
    static auto get=icall<float(*)(void*)>("UnityEngine.Camera::get_nearClipPlane");
    static auto set=icall<void(*)(void*,float)>("UnityEngine.Camera::set_nearClipPlane");
    if(!get||!set)return;
    auto pc=firstPersonAnchor.player?field<void*>(firstPersonAnchor.player,"Camera"):nullptr;
    for(int i=0;i<2;++i){auto& s=states[i];auto next=active&&alive(pc)?field<void*>(pc,i==0?"Camera":"OutlineCamera"):nullptr;
        if(s.camera!=next){if(alive(s.camera))set(s.camera,s.saved);if(s.root)gc_free(s.root);s.root=0;s.camera=nullptr;
            if(alive(next)){s.camera=next;s.root=gc_root(s.camera,false);s.saved=get(s.camera);}}
        if(alive(s.camera))set(s.camera,.04f);
    }
}
