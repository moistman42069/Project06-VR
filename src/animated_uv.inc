// Pinned ARM64 metadata + disassembly: TexScroll has a 24-byte stride,
// material index at 0, name at 8, and Vector2 speed at 16. Native Update
// computes speed * Time.time, then SetTextureOffset for each row.
struct UVRow { int32_t material;uint32_t padding;void* name;float speed[2]; };
static_assert(sizeof(UVRow)==24&&offsetof(UVRow,name)==8&&offsetof(UVRow,speed)==16,"Pinned TexScroll ABI");
void (*oldAnimatedUV)(void*,const Method*)=nullptr;
uint64_t skippedUVRows=0;
void animatedUVHook(void* self,const Method* method){
    if(!managedReady||gettid()!=unityThread){oldAnimatedUV(self,method);return;}
    static auto materials=icall<void*(*)(void*)>("UnityEngine.Renderer::GetMaterialArray");
    static auto time=icall<float(*)()>("UnityEngine.Time::get_time");
    static auto property=icall<int(*)(void*)>("UnityEngine.Shader::PropertyToID");
    static auto has=icall<bool(*)(void*,int)>("UnityEngine.Material::HasProperty");
    static auto offset=icall<void(*)(void*,int,const float*)>("UnityEngine.Material::SetTextureOffsetImpl_Injected");
    if(!materials||!time||!property||!has||!offset){oldAnimatedUV(self,method);return;}
    auto renderer=field<void*>(self,"renderer"),rows=field<void*>(self,"textures");if(!alive(renderer)||!rows)return;
    auto mats=materials(renderer);if(!mats)return;auto nr=array_length(rows),nm=array_length(mats);if(nr>256||nm>256)return;
    float t=time();if(!std::isfinite(t))return;
    for(uintptr_t i=0;i<nr;++i){UVRow row;memcpy(&row,array_address(rows,sizeof(UVRow),i),sizeof(row));
        if(row.material<0||uintptr_t(row.material)>=nm||!row.name){++skippedUVRows;continue;}
        auto mat=*reinterpret_cast<void**>(array_address(mats,sizeof(void*),row.material));if(!alive(mat)){++skippedUVRows;continue;}
        int id=property(row.name);if(!has(mat,id)){++skippedUVRows;continue;}
        float uv[2]={row.speed[0]*t,row.speed[1]*t};if(!std::isfinite(uv[0])||!std::isfinite(uv[1])){++skippedUVRows;continue;}offset(mat,id,uv);
    }
}
