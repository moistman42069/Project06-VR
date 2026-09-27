#include "save_schema.h"
bool saveCompatibilityInstalled=false;
void* (*oldApplicationVersion)(const Method*)=nullptr;
void (*oldSaveSlotSetup)(void*,float,void*,int,void*,const Method*)=nullptr;
void (*oldSaveStarter)(void*,const Method*)=nullptr;
bool compatibleManagedSaveVersion(void* version){
    if(!version)return false;int length=string_length(version);if(length<1||length>63)return false;
    char text[64]{};auto chars=string_chars(version);
    for(int i=0;i<length;++i){if(chars[i]>127)return false;text[i]=char(chars[i]);}
    return CompatibleSaveVersion(text);
}
void* applicationVersionHook(const Method* method){
    // Invoked by managed game code, never by the loader/bootstrap.
    if(!saveCompatibilityInstalled||!thread_current||!thread_current())return oldApplicationVersion(method);
    return string_new(kSaveSchemaVersion);
}
void saveSlotSetupHook(void* self,float playtime,void* lastSave,int banner,void* version,const Method* method){
    if(saveCompatibilityInstalled&&compatibleManagedSaveVersion(version))version=string_new(kSaveSchemaVersion);
    oldSaveSlotSetup(self,playtime,lastSave,banner,version,method);
}
void saveStarterHook(void* self,const Method* method){
    oldSaveStarter(self,method);
    if(!saveCompatibilityInstalled||!managedReady||gettid()!=unityThread)return;
    auto list=field<void*>(self,"Datas");if(!list)return;
    auto items=field<void*>(list,"_items");int count=field<int>(list,"_size");
    if(!items||count<0||count>16||uintptr_t(count)>array_length(items))return;
    int migrated=0;
    for(int i=0;i<count;++i){auto data=*reinterpret_cast<void**>(array_address(items,sizeof(void*),i));if(!data)continue;
        auto version=field<void*>(data,"version");
        if(compatibleManagedSaveVersion(version)&&!eq(version,kSaveSchemaVersion)){
            if(setObjectField(data,"version",string_new(kSaveSchemaVersion)))++migrated;
        }
    }
    LOG("Save slots checked: %d; older candidate versions accepted in memory: %d; files unchanged",count,migrated);
}
