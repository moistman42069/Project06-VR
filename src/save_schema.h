#pragma once
#include <cstring>
// Save payload schema 1: pinned original game assemblies, unchanged since 0.1.0.
// This identity MUST NOT follow APK versionName/versionCode or the VR menu label.
constexpr const char* kSaveSchemaVersion="0.1.10-vr-candidate";
inline bool CompatibleSaveVersion(const char* version){
    if(!version)return false;
    constexpr const char* known[]={"0.1.0-vr-candidate","0.1.1-vr-candidate","0.1.2-vr-candidate","0.1.3-vr-candidate","0.1.4-vr-candidate","0.1.5-vr-candidate","0.1.6-vr-candidate","0.1.7-vr-candidate","0.1.8-vr-candidate","0.1.9-vr-candidate",kSaveSchemaVersion};
    for(auto s:known)if(std::strcmp(version,s)==0)return true;
    return false;
}
