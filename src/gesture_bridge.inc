// Included in bridge.cpp's anonymous namespace: Unity access remains main-thread only.
Controls applyGestures(Controls c){
    if(!managedReady||gettid()!=unityThread)return c;
    gestureRunPlayer=nullptr;gestureRunAmount=0;
    const auto o=GetOptions();
    MotionFrame f;{std::lock_guard<std::mutex> lock(motionMutex);f=motionFrame;}
    const double now=std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    GestureContext context;
    bool need=o.motionRun||o.gestureHoming||o.crouchSpin||gesturePlayer;
    void* player=nullptr;
    if(need&&refreshFirstPersonAnchor())player=firstPersonAnchor.player;
    static void* sonicClass=nullptr;if(need&&!sonicClass)sonicClass=klass("","SonicNew");
    bool sonic=alive(player)&&object_class(player)==sonicClass;
    static auto timeScale=icall<float(*)()>("UnityEngine.Time::get_timeScale");
    context.allowed=sonic&&c.focused&&!c.blocked&&!VRMenuOpen()&&o.mode==ViewMode::Immersive&&timeScale&&timeScale()>0&&!field<bool>(player,"IsDead")&&!field<bool>(player,"LockControls");
    context.player=reinterpret_cast<uintptr_t>(player);
    context.manual=(c.buttons&(A|X|B|Menu))!=0;
    int state=sonic?field<int>(player,"PlayerState"):-1;
    context.ground=state==0&&!field<bool>(player,"IsSinking");
    context.charge=state==15&&field<int>(player,"SpinDashState")==0;
    context.homing=(state==2&&field<bool>(player,"ReleasedKey")&&field<int>(player,"JumpLimit")<1)||(state==3&&field<bool>(player,"CanFallJumpdash"));
    auto target=sonic?field<void*>(player,"HomingTarget"):nullptr;
    static auto goTransform=icall<void*(*)(void*)>("UnityEngine.GameObject::get_transform");
    static auto compTransform=icall<void*(*)(void*)>("UnityEngine.Component::get_transform");
    static auto getPos=icall<void(*)(void*,float*)>("UnityEngine.Transform::get_position_Injected");
    static auto getRot=icall<void(*)(void*,float*)>("UnityEngine.Transform::get_rotation_Injected");
    static auto setRotation=icall<void(*)(void*,const float*)>("UnityEngine.Transform::set_rotation_Injected");
    float camRot[4]{0,0,0,1},baseRot[4]{0,0,0,1},headWorld[4]{0,0,0,1};
    if(sonic&&getRot&&alive(firstPersonAnchor.cameraTransform))getRot(firstPersonAnchor.cameraTransform,camRot);
    if(o.firstPerson)quatMultiply(camRot,firstPersonAnchor.rotation,baseRot);else std::copy(camRot,camRot+4,baseRot);
    quatMultiply(baseRot,f.head.q,headWorld);float forward[3]={0,0,1},direction[3];quatRotate(headWorld,forward,direction);
    if(context.allowed&&context.homing&&alive(target)&&goTransform&&getPos&&getRot){
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
    if(result.cancel&&alive(gesturePlayer)&&field<int>(gesturePlayer,"PlayerState")==15&&field<int>(gesturePlayer,"SpinDashState")==0){
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
    if(result.run>0&&!context.manual&&std::fabs(c.lx)<.1f&&std::fabs(c.ly)<.1f){
        float cameraForward[3];quatRotate(camRot,forward,cameraForward);
        float yaw=std::atan2(direction[0],direction[2])-std::atan2(cameraForward[0],cameraForward[2]);
        c.lx=std::sin(yaw);c.ly=std::cos(yaw);
        if(context.ground){gestureRunPlayer=player;gestureRunAmount=result.run;}
    }
    return c;
}
