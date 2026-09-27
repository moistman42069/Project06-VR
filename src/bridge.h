#pragma once
#include <jni.h>
#include <cstdint>
#include <android/log.h>
#include "controls.h"
#include "gestures.h"
void PublishMotionFrame(const MotionFrame& frame);
void QueueGameHaptic(float amplitude,float seconds,int hand=-1);
bool GameAllowsHaptics();
void Log(const char* format,...) __attribute__((format(printf,1,2)));
#define LOG(...) Log(__VA_ARGS__)
extern JavaVM* g_vm;
extern jobject g_activity;
void PublishControls(const Controls&);
void GameMainTick();
bool XRDisplayGraphicsReady();
void SyncHudCamera();
void PublishTrackedControllerPose(int hand,bool valid,const float position[3],const float rotation[4]);
bool GetFirstPersonCameraAnchor(float position[3],float rotation[4]);
void SyncFirstPersonVisuals();
void LogBridgeStats();
void InstallGameHooks(void* library);
void BeginLoaderHooks();
