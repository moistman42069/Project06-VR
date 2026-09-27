"""Read-only menu camera/video evidence from the pinned extracted scene."""
import sys,json,pathlib
sys.path.insert(0,'vendor/unitypy')
import UnityPy
scene=sys.argv[1] if len(sys.argv)>1 else 'level2'
assert scene in ('level0','level2')
env=UnityPy.load('original/assets/'+scene)
objects={o.path_id:o for o in env.objects}
def tree(o):
    try:return o.read_typetree()
    except Exception:return {}
rows=[]
for o in env.objects:
    if o.type.name not in ('Camera','VideoPlayer'):continue
    t=tree(o); go=objects.get(t.get('m_GameObject',{}).get('m_PathID'));g=tree(go) if go else {}
    components=[]
    for c in g.get('m_Component',[]):
        co=objects.get(c['component']['m_PathID'])
        if co:components.append({'type':co.type.name,'id':co.path_id,'tree':tree(co)})
    rows.append({'type':o.type.name,'name':g.get('m_Name'),'tree':t,'components':components})
pathlib.Path('out/'+('title' if scene=='level0' else 'menu')+'-camera-components.json').write_text(json.dumps(rows,indent=2,default=str))
for r in rows:
    print(r['name'],r['type'], [(c['type'], c['tree'].get('m_Script')) for c in r['components']])
