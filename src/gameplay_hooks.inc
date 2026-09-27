struct WaterTrigger { void* script=nullptr;void* collider=nullptr;uint32_t root=0,colliderRoot=0;bool booster=false;uint64_t seen=0; };
std::vector<WaterTrigger> waterTriggers;
uint64_t physicsSequence=1;
using TriggerFn=void(*)(void*,void*,const Method*);
TriggerFn oldWaterTrigger=nullptr,oldBoosterTrigger=nullptr;
void (*oldSonicFixed)(void*,const Method*)=nullptr;
void (*oldWaterEnter)(void*,void*,bool,float,const Method*)=nullptr;
void (*oldAddRing)(void*,int,void*,const Method*)=nullptr;
void (*oldAcceleration)(void*,float,const Method*)=nullptr;
float gestureRunAmount=0;void* gestureRunPlayer=nullptr;
void markWaterTrigger(void* self){for(auto& t:waterTriggers)if(t.script==self)t.seen=physicsSequence;}
void waterTriggerHook(void* self,void* collider,const Method* m){markWaterTrigger(self);oldWaterTrigger(self,collider,m);}
void boosterTriggerHook(void* self,void* collider,const Method* m){markWaterTrigger(self);oldBoosterTrigger(self,collider,m);}
void waterEnterHook(void* self,void* spline,bool enter,float speed,const Method* m){
    LOG("WaterSlide entry: player=%p state=%d trigger=%d speed=%.3f old_speed=%.3f progress=%.5f",self,field<int>(self,"PlayerState"),enter,speed,field<float>(self,"WSpeed"),field<float>(self,"WSTime"));
    oldWaterEnter(self,spline,enter,speed,m);QueueGameHaptic(.3f,.06f);
}
void addRingHook(void* self,int amount,void* source,const Method* m){oldAddRing(self,amount,source,m);if(amount>0)QueueGameHaptic(.16f,.025f);}
void accelerationHook(void* self,float acceleration,const Method* m){
    if(self!=gestureRunPlayer||gestureRunAmount<=0||field<int>(self,"PlayerState")!=0){oldAcceleration(self,acceleration,m);return;}
    float maximum=field<float>(self,"MaximumSpeed");auto f=class_field(object_class(self),"MaximumSpeed");
    if(!f||!std::isfinite(maximum)||maximum<=0){oldAcceleration(self,acceleration,m);return;}
    float cap=maximum*std::clamp(gestureRunAmount,.02f,1.f);field_set(self,f,&cap);
    oldAcceleration(self,acceleration*std::clamp(gestureRunAmount,.1f,1.f),m);field_set(self,f,&maximum);
}
void discoverWaterTriggers(){
    for(auto it=waterTriggers.begin();it!=waterTriggers.end();){if(!alive(it->script)||!alive(it->collider)){gc_free(it->root);gc_free(it->colliderRoot);it=waterTriggers.erase(it);}else ++it;}
    static auto find=icall<void*(*)(void*,bool)>("UnityEngine.Object::FindObjectsOfType");
    static auto go=icall<void*(*)(void*)>("UnityEngine.Component::get_gameObject");
    auto ck=klass("UnityEngine","Collider"),gk=klass("UnityEngine","GameObject");if(!find||!go||!ck||!gk)return;
    for(int kind=0;kind<2;++kind){auto k=klass("",kind?"WaterslideBooster":"WaterSlider");if(!k)continue;auto all=find(type_object(class_type(k)),false);if(!all)continue;auto n=array_length(all);if(n>2048)continue;
        for(uintptr_t i=0;i<n;++i){auto script=*reinterpret_cast<void**>(array_address(all,sizeof(void*),i));if(!alive(script))continue;
            if(std::any_of(waterTriggers.begin(),waterTriggers.end(),[&](const WaterTrigger& t){return t.script==script;}))continue;
            void* args[]={type_object(class_type(ck))};auto collider=call(gk,"GetComponent",go(script),args,1);if(!alive(collider))continue;
            waterTriggers.push_back({script,collider,gc_root(script,false),gc_root(collider,false),kind!=0,0});
        }
    }
    static size_t last=0;if(last!=waterTriggers.size()){last=waterTriggers.size();LOG("Water trigger sweep registry: %zu live entry/booster colliders",last);}
}
void sonicFixedHook(void* self,const Method* m){
    // This callback is already in initialized managed gameplay on the Unity thread.
    if(!managedReady||gettid()!=unityThread){oldSonicFixed(self,m);return;}
    static void* previousPlayer=nullptr;static uint32_t previousRoot=0;static float previousPos[3]{};static bool havePosition=false;static double previousTime=0;
    static auto tr=icall<void*(*)(void*)>("UnityEngine.Component::get_transform");
    static auto getPos=icall<void(*)(void*,float*)>("UnityEngine.Transform::get_position_Injected");
    static auto inverse=icall<void(*)(void*,const float*,float*)>("UnityEngine.Transform::InverseTransformPoint_Injected");
    static auto enabled=icall<bool(*)(void*)>("UnityEngine.Collider::get_enabled");
    static auto trigger=icall<bool(*)(void*)>("UnityEngine.Collider::get_isTrigger");
    static auto go=icall<void*(*)(void*)>("UnityEngine.Component::get_gameObject");
    static auto active=icall<bool(*)(void*)>("UnityEngine.GameObject::get_activeInHierarchy");
    static auto sphereCenter=icall<void(*)(void*,float*)>("UnityEngine.SphereCollider::get_center_Injected");
    static auto sphereRadius=icall<float(*)(void*)>("UnityEngine.SphereCollider::get_radius");
    static auto boxCenter=icall<void(*)(void*,float*)>("UnityEngine.BoxCollider::get_center_Injected");
    static auto boxSize=icall<void(*)(void*,float*)>("UnityEngine.BoxCollider::get_size_Injected");
    static void* sphereClass=klass("UnityEngine","SphereCollider"),*boxClass=klass("UnityEngine","BoxCollider");
    float pos[3]{};bool valid=tr&&getPos&&alive(self)&&!field<bool>(self,"IsDead");if(valid)getPos(tr(self),pos);
    double now=std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    if(previousPlayer!=self){if(previousRoot)gc_free(previousRoot);previousRoot=gc_root(self,false);previousPlayer=self;havePosition=false;}
    float distance=0;for(int i=0;i<3;++i){if(!std::isfinite(pos[i]))valid=false;distance+=(pos[i]-previousPos[i])*(pos[i]-previousPos[i]);}
    int before=field<int>(self,"PlayerState");float speed=field<float>(self,"WSpeed"),progress=field<float>(self,"WSTime");
    // Bound by displacement and elapsed time; never bridge teleports, death or menus.
    bool eligible=before==0||before==2||before==3||before==11||before==18;
    if(oldWaterTrigger&&oldBoosterTrigger&&valid&&eligible&&havePosition&&now-previousTime<.15&&distance>.0001f&&distance<144.f&&!VRMenuOpen()&&inverse&&enabled&&trigger&&go&&active){
        auto playerCollider=field<void*>(self,"CapsuleCollider");
        for(auto& t:waterTriggers){if(!alive(t.script)||!alive(t.collider)||t.seen+1>=physicsSequence||!enabled(t.collider)||!trigger(t.collider)||!active(go(t.collider))||(t.booster&&field<bool>(t.script,"IsTriggered")))continue;
            auto transform=tr(t.collider);if(!alive(transform))continue;float a[3],b[3],center[3],size[3];inverse(transform,previousPos,a);inverse(transform,pos,b);bool crossed=false;
            if(object_class(t.collider)==sphereClass&&sphereCenter&&sphereRadius){sphereCenter(t.collider,center);crossed=SegmentSphere(a,b,center,sphereRadius(t.collider));}
            else if(object_class(t.collider)==boxClass&&boxCenter&&boxSize){boxCenter(t.collider,center);boxSize(t.collider,size);crossed=SegmentBox(a,b,center,size);}
            if(crossed&&alive(playerCollider)){void* args[]={playerCollider};call(object_class(t.script),"OnTriggerEnter",t.script,args,1);t.seen=physicsSequence;
                LOG("Water trigger full-crossing recovery: %s distance=%.3f before=%d after=%d",t.booster?"booster":"spline entry",std::sqrt(distance),before,field<int>(self,"PlayerState"));break;}
        }
    }
    std::copy(pos,pos+3,previousPos);havePosition=valid;previousTime=now;++physicsSequence;
    oldSonicFixed(self,m);
    int after=field<int>(self,"PlayerState");
    if(before==18&&after!=18)LOG("WaterSlide exit: next_state=%d speed_before=%.3f speed_after=%.3f progress_before=%.5f progress_after=%.5f position=(%.3f,%.3f,%.3f)",after,speed,field<float>(self,"WSpeed"),progress,field<float>(self,"WSTime"),pos[0],pos[1],pos[2]);
}
void gameHapticTick(){
    static void* lastPlayer=nullptr;static char lastState[80]{};static int lastSpin=-1;static double pulseAt=0;static bool left=false;
    Controls c;{std::lock_guard<std::mutex> lock(inputMutex);c=controls;}
    static auto timeScale=icall<float(*)()>("UnityEngine.Time::get_timeScale");
    gameHapticsAllowed.store(c.focused&&!c.blocked&&!VRMenuOpen()&&GetOptions().haptics&&timeScale&&timeScale()>0);
    if(!gameHapticsAllowed.load()){lastPlayer=nullptr;lastState[0]=0;return;}
    if(!refreshFirstPersonAnchor()){gameHapticsAllowed.store(false);lastPlayer=nullptr;lastState[0]=0;return;}
    auto player=firstPersonAnchor.player;auto value=call(object_class(player),"GetState",player,nullptr,0);if(!value)return;
    int n=string_length(value);if(n<0||n>=80)return;char state[80]{};auto chars=string_chars(value);for(int i=0;i<n;++i)state[i]=chars[i]<128?char(chars[i]):'?';
    double now=std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    if(player!=lastPlayer){lastPlayer=player;strcpy(lastState,state);lastSpin=-1;pulseAt=now;return;}
    if(strcmp(state,lastState)){auto pulse=ActionHaptic(state);QueueGameHaptic(pulse.amplitude,pulse.seconds);strcpy(lastState,state);pulseAt=now;}
    if(!strcmp(state,"SpinDash")){int spin=field<int>(player,"SpinDashState");if(spin==1&&lastSpin==0)QueueGameHaptic(.65f,.12f);else if(spin==0&&now-pulseAt>.16){QueueGameHaptic(.22f,.045f);pulseAt=now;}lastSpin=spin;}else lastSpin=-1;
    float speed=field<float>(player,"CurSpeed");if(!std::isfinite(speed))return;
    bool travel=!strcmp(state,"Ground")||!strcmp(state,"Grinding")||!strcmp(state,"WaterSlide")||!strcmp(state,"Slide");
    if(travel&&speed>2&&now-pulseAt>std::clamp(1.6/std::max(double(speed),1.0),.09,.4)){QueueGameHaptic(std::clamp(speed*.006f,.04f,.2f),.022f,left?0:1);left=!left;pulseAt=now;}
}
