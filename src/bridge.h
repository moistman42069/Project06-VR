#pragma once
#include <jni.h>
#include <cstdint>
#include <android/log.h>
#include "controls.h"
void Log(const char* format,...) __attribute__((format(printf,1,2)));
#define LOG(...) Log(__VA_ARGS__)
extern JavaVM* g_vm;
extern jobject g_activity;
void PublishControls(const Controls&);
void GameMainTick();
bool XRDisplayGraphicsReady();
void SyncHudCamera();
void LogBridgeStats();
void InstallGameHooks(void* library);
void BeginLoaderHooks();
