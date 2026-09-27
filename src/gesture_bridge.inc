// Included in bridge.cpp's anonymous namespace: Unity access remains main-thread only.
Controls applyGestures(Controls c){
    if(!managedReady||gettid()!=unityThread)return c;
    gestureRunPlayer=nullptr;gestureRunAmount=0;
    const auto o=GetOptions();
    MotionFrame f;{std::lock_guard<std::mutex> lock(motionMutex);f=motionFrame;}
    const double now=std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    GestureContext context;
    bool need=o.firstPerson||o.motionRun||o.gestureHoming||o.crouchSpin||gesturePlayer;
    void* player=nullptr;
    if(need&&refreshFirstPersonAnchor())player=firstPersonAnchor.player;
    bool present=alive(player);
    static void* sonicClass=nullptr;static void* shadowClass=nullptr;static void* metalClass=nullptr;static void* princessClass=nullptr;static void* fastClass=nullptr;static void* boardClass=nullptr;
    if(need&&!sonicClass){sonicClass=klass("","SonicNew");shadowClass=klass("","Shadow");metalClass=klass("","MetalSonic");princessClass=klass("","Princess");fastClass=klass("","SonicFast");boardClass=klass("","SnowBoard");}
    auto playerClass=present?object_class(player):nullptr;
    bool sonic=present&&playerClass==sonicClass;
    bool spin=present&&(sonic||playerClass==shadowClass||playerClass==metalClass);
    bool homingSupported=spin||(present&&playerClass==princessClass);
    auto state=present?call(playerClass,"GetState",player,nullptr,0):nullptr;
    auto is=[&](const char* name){return eq(state,name);};
    static auto timeScale=icall<float(*)()>("UnityEngine.Time::get_timeScale");
    context.allowed=present&&c.focused&&!c.blocked&&!VRMenuOpen()&&o.mode==ViewMode::Immersive&&timeScale&&timeScale()>0&&!field<bool>(player,"IsDead")&&!field<bool>(player,"LockControls");
    context.player=reinterpret_cast<uintptr_t>(player);
    context.manual=(c.buttons&(A|X|B|Menu))!=0;
    context.ground=is("Ground")&&!field<bool>(player,"IsSinking");
    context.spinSupported=spin;
    context.charge=spin&&is("SpinDash")&&field<int>(player,"SpinDashState")==0;
    context.homing=homingSupported&&is("Jump")&&field<bool>(player,"ReleasedKey");
    if(sonic)context.homing=(context.homing&&field<int>(player,"JumpLimit")<1&&!field<bool>(player,"UsingPurpleGem"))||(is("Air")&&field<bool>(player,"CanFallJumpdash"));
    if(homingSupported&&(is("AfterHoming")||is("Spring")||is("WideSpring")||is("Rope")))context.homing=true;
    // Pre-arm while pointing on the ground; fire only in a native eligible attack state.
    auto target=homingSupported?field<void*>(player,"HomingTarget"):nullptr;
    static auto goTransform=icall<void*(*)(void*)>("UnityEngine.GameObject::get_transform");
    static auto compTransform=icall<void*(*)(void*)>("UnityEngine.Component::get_transform");
    static auto getPos=icall<void(*)(void*,float*)>("UnityEngine.Transform::get_position_Injected");
    static auto getRot=icall<void(*)(void*,float*)>("UnityEngine.Transform::get_rotation_Injected");
    static auto setRotation=icall<void(*)(void*,const float*)>("UnityEngine.Transform::set_rotation_Injected");
    float camRot[4]{0,0,0,1},baseRot[4]{0,0,0,1},headWorld[4]{0,0,0,1};
    if(present&&getRot&&alive(firstPersonAnchor.cameraTransform))getRot(firstPersonAnchor.cameraTransform,camRot);
    if(o.firstPerson)quatMultiply(camRot,firstPersonAnchor.rotation,baseRot);else std::copy(camRot,camRot+4,baseRot);
    quatMultiply(baseRot,f.head.q,headWorld);float forward[3]={0,0,1},direction[3];quatRotate(headWorld,forward,direction);
    static auto getLocalRot=icall<void(*)(void*,float*)>("UnityEngine.Transform::get_localRotation_Injected");
    float movementRot[4]{0,0,0,1},movementForward[3]{0,0,1};
    auto movementCamera=present?field<void*>(player,"Camera"):nullptr;
    bool movementBasis=alive(movementCamera)&&compTransform&&getLocalRot;
    if(movementBasis){getLocalRot(compTransform(movementCamera),movementRot);quatRotate(movementRot,forward,movementForward);}
    static float stableHeadYaw=0;static void* headingPlayer=nullptr;
    if(headingPlayer!=player){headingPlayer=player;stableHeadYaw=std::atan2(movementForward[0],movementForward[2]);}
    if(direction[0]*direction[0]+direction[2]*direction[2]>.01f)stableHeadYaw=std::atan2(direction[0],direction[2]);
    float headYaw=stableHeadYaw,nativeYaw=std::atan2(movementForward[0],movementForward[2]);
    // Rail/spline/vehicle controls are lane coordinates, not free world movement.
    bool standardMotor=present&&playerClass!=fastClass&&playerClass!=boardClass;
    bool freeMove=standardMotor&&(is("Ground")||is("Brake")||is("Jump")||is("Air")||is("SlowFall")||is("AfterHoming")||is("SpinDash"));
    bool steering=freeMove||(standardMotor&&(is("Fly")||is("Glide")||is("Levitate")||is("Hover")||is("HammerJump")||is("AccelJump")));
    if(o.firstPerson&&firstPersonWasActive&&context.allowed&&steering&&movementBasis&&f.head.valid)
        P06RemapStick(c.lx,c.ly,headYaw,nativeYaw);
    if(context.allowed&&alive(target)&&goTransform&&getPos&&getRot){
        context.target=reinterpret_cast<uintptr_t>(target);float targetPos[3],camPos[3];getPos(goTransform(target),targetPos);getPos(firstPersonAnchor.cameraTransform,camPos);
        for(int h=0;h<2;++h)if(f.aim[h].valid){float local[3];for(int j=0;j<3;++j)local[j]=f.aim[h].p[j]/o.worldScale;
            float posePos[3];
            // Compose tracking pose into the game camera's coordinate system.
            if(o.firstPerson){quatRotate(firstPersonAnchor.rotation,local,posePos);for(int j=0;j<3;++j)posePos[j]+=firstPersonAnchor.position[j];}
            else std::copy(local,local+3,posePos);
            float worldPos[3],aimRot[4],aimDir[3];quatRotate(camRot,posePos,worldPos);quatMultiply(baseRot,f.aim[h].q,aimRot);quatRotate(aimRot,forward,aimDir);
            float dot=0,length=0;for(int j=0;j<3;++j){float d=targetPos[j]-(worldPos[j]+camPos[j]);dot+=d*aimDir[j];length+=d*d;}
            context.pointing[h]=length>.01f&&dot/std::sqrt(length)>.9063078f;
        }
    }
    auto result=gestures.step(f,context,o,now);
    if(!oldAcceleration)result.run=0;
    if(result.cancel&&alive(gesturePlayer)&&eq(call(object_class(gesturePlayer),"GetState",gesturePlayer,nullptr,0),"SpinDash")&&field<int>(gesturePlayer,"SpinDashState")==0){
        // Full native state transition prevents an unintended launch on tracking/menu loss.
        void* args[]={string_new("StateAir")};call(object_class(gesturePlayer),"SetMachineState",gesturePlayer,args,1);
        LOG("Gesture spindash cancelled through native state transition");
    }
    if(result.charge){c.buttons|=X;if(gesturePlayer!=player){if(gesturePlayerRoot)gc_free(gesturePlayerRoot);gesturePlayer=player;gesturePlayerRoot=gc_root(player,false);LOG("Gesture spindash charge started");}}
    else if(gesturePlayer){if(gesturePlayerRoot)gc_free(gesturePlayerRoot);gesturePlayerRoot=0;gesturePlayer=nullptr;}
    if(result.release&&compTransform&&getRot&&setRotation){
        float flat[3]={direction[0],0,direction[2]};float len=std::sqrt(flat[0]*flat[0]+flat[2]*flat[2]);
        if(len>.1f){auto tr=compTransform(player);float q[4],oldForward[3],resultQ[4];getRot(tr,q);quatRotate(q,forward,oldForward);
            float turn=std::atan2(flat[0],flat[2])-std::atan2(oldForward[0],oldForward[2]);float yaw[4]={0,std::sin(turn*.5f),0,std::cos(turn*.5f)};quatMultiply(yaw,q,resultQ);setRotation(tr,resultQ);}
        LOG("Gesture spindash released in headset direction");
    }
    if(result.homing){c.buttons|=A;LOG("Gesture homing pulse: native selected target=%p",target);}
    if(result.run>0&&!context.manual&&freeMove&&movementBasis&&std::fabs(c.lx)<.1f&&std::fabs(c.ly)<.1f){
        float yaw=headYaw-nativeYaw;
        c.lx=std::sin(yaw);c.ly=std::cos(yaw);
        if(context.ground){gestureRunPlayer=player;gestureRunAmount=result.run;}
    }
    static double lastReport=0;
    if(context.allowed&&now-lastReport>5&&(o.motionRun||o.gestureHoming||o.firstPerson)){
        lastReport=now;LOG("VR locomotion: prefab=%d ground=%d run=%.3f speed=%.2f nativeYaw=%.1f headYaw=%.1f homingEligible=%d target=%p spinSupported=%d",field<int>(player,"PlayerPrefab"),int(context.ground),result.run,field<float>(player,"CurSpeed"),nativeYaw*57.29578f,headYaw*57.29578f,int(context.homing),target,int(spin));
    }
    return c;
}
