"""Generate local-only glove geometry from the user's pinned game mesh.

Generated game geometry stays under ignored out/, never in the public source tree.
"""
import sys,pathlib,json,hashlib
root=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'vendor/unitypy'))
import numpy as np
import UnityPy
from UnityPy.helpers.MeshHelper import MeshHandler
asset=root/'original/assets/resources.assets'
if not asset.exists():
    import subprocess
    subprocess.run([sys.executable,str(root/'tools/extract-asset-metadata.py'),'resources.assets'],cwd=root,check=True)
env=UnityPy.load(str(asset));objects={o.path_id:o for o in env.objects}
assert hashlib.sha256(asset.read_bytes()).hexdigest()=='1eafa099d184161d8e750f89862023f79721faecd7b039b129383c4ea4d47640','Unpinned resources.assets'
mesh=objects[3590].read();assert mesh.m_Name=='sonic_Root'
handler=MeshHandler(mesh);handler.process()
assert objects[235].read().m_Name=='ch_sonic_cloth'
sub=mesh.m_SubMeshes[9];assert sub.indexCount==5688
vertices=np.array(handler.m_Vertices);normals=np.array(handler.m_Normals)
indices=np.array(handler.m_IndexBuffer[sub.firstByte//2:sub.firstByte//2+sub.indexCount]).reshape(-1,3)
def bone(index):
    b=mesh.m_BindPose[index]
    return np.linalg.inv([[getattr(b,f'e{r}{c}') for c in range(4)] for r in range(4)])[:3,3]
def smooth_mesh(v,n,f):
    # Weld texture/normal splits, then one Loop subdivision. Preserve the authored
    # shape and open cuff boundary while eliminating the coarse silhouette.
    lookup={};points=[];mapping=[]
    for p in v:
        key=tuple(np.round(p,6))
        if key not in lookup:lookup[key]=len(points);points.append(p)
        mapping.append(lookup[key])
    original_cross=np.cross(v[f[:,1]]-v[f[:,0]],v[f[:,2]]-v[f[:,0]])
    normal_sign=1 if np.sum(original_cross*n[f[:,0]])>=0 else -1
    f=np.array(mapping)[f];v=np.array(points);edges={};neighbors=[set() for _ in v]
    for a,b,c in f:
        for i,j,k in [(a,b,c),(b,c,a),(c,a,b)]:
            edges.setdefault(tuple(sorted((i,j))),[]).append(k);neighbors[i].add(j);neighbors[j].add(i)
    boundary=[[] for _ in v]
    for (a,b),other in edges.items():
        assert len(other)<=2
        if len(other)==1:boundary[a].append(b);boundary[b].append(a)
    result=[]
    for i,p in enumerate(v):
        if boundary[i]:
            assert len(boundary[i])==2
            result.append(p*.75+v[boundary[i]].sum(axis=0)*.125)
        else:
            count=len(neighbors[i]);beta=3/16 if count==3 else 3/(8*count)
            result.append(p*(1-count*beta)+v[list(neighbors[i])].sum(axis=0)*beta)
    edge_ids={}
    for edge,other in edges.items():
        a,b=edge;edge_ids[edge]=len(result)
        result.append((v[a]+v[b])*.5 if len(other)==1 else (v[a]+v[b])*.375+v[other].sum(axis=0)*.125)
    faces=[]
    for a,b,c in f:
        ab=edge_ids[tuple(sorted((a,b)))];bc=edge_ids[tuple(sorted((b,c)))];ca=edge_ids[tuple(sorted((c,a)))]
        faces.extend([(a,ab,ca),(ab,b,bc),(ca,bc,c),(ab,bc,ca)])
    v=np.array(result);f=np.array(faces);n=np.zeros_like(v)
    for face in f:
        normal=np.cross(v[face[1]]-v[face[0]],v[face[2]]-v[face[0]])*normal_sign
        for index in face:n[index]+=normal
    lengths=np.linalg.norm(n,axis=1);assert np.all(lengths>1e-10)
    return v,n/lengths[:,None],f
data=[];output=['// Generated from pinned APK; do not publish this extracted game geometry.']
for hand,sign,wrist,middle,index,pinky in [(0,-1,48,56,51,49),(1,1,59,67,62,60)]:
    # Fingers +Z, thumbs +X on left / -X on right, dorsal surface +Y.
    origin=bone(wrist);forward=bone(middle)-origin;forward/=np.linalg.norm(forward)
    across=(bone(index)-bone(pinky))*(-sign);across-=forward*np.dot(across,forward);across/=np.linalg.norm(across)
    up=np.cross(forward,across);basis=np.array([across,up,forward])
    assert np.linalg.det(basis)>.999
    triangles=indices[np.all(vertices[indices,0]*sign>0,axis=1)]
    used=sorted(set(triangles.flatten()));lookup={v:i for i,v in enumerate(used)}
    positions=(vertices[used]-origin)@basis.T*.8
    ns=normals[used]@basis.T;ns/=np.linalg.norm(ns,axis=1)[:,None]
    faces=np.array([[lookup[i] for i in t] for t in triangles])
    positions,ns,faces=smooth_mesh(positions,ns,faces)
    assert len(positions)>300 and len(faces)>700 and np.isfinite(positions).all()
    assert positions[:,2].max()>.15 and positions[:,2].min()>-.08
    for label,values in [('Vertices',positions),('Normals',ns)]:
        output.append(f'constexpr float kGlove{hand}{label}[][3]={{')
        output.extend('{'+','.join(f'{v:.8f}f' for v in row)+'},' for row in values);output.append('};')
    output.append(f'constexpr int kGlove{hand}Triangles[]={{'+','.join(str(i) for i in faces.flatten())+'};')
    data.append({'hand':hand,'vertices':positions.tolist(),'normals':ns.tolist(),'triangles':faces.tolist()})
out=root/'out/generated';out.mkdir(parents=True,exist_ok=True)
(out/'sonic_gloves.inc').write_text('\n'.join(output)+'\n')
(out/'sonic_gloves.json').write_text(json.dumps(data))
receipt={'asset_sha256':hashlib.sha256(asset.read_bytes()).hexdigest(),'mesh_path_id':3590,'submesh':9,'material':'ch_sonic_cloth','generated_sha256':hashlib.sha256((out/'sonic_gloves.inc').read_bytes()).hexdigest(),'hands':[{'vertices':len(d['vertices']),'triangles':len(d['triangles'])} for d in data]}
(root/'evidence/glove-extraction.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(receipt)
