"""Read-only pinned ABI/lifecycle audit. Not a headset runtime test."""
import re,json,pathlib,sys
root=pathlib.Path(__file__).resolve().parents[1]
source='\n'.join(p.read_text() for p in (root/'src').glob('*') if p.suffix in ('.h','.cpp','.inc'))
unity=(root/'original/libunity.so').read_bytes()
calls=sorted(set(re.findall(r'"(UnityEngine\.[^"]*::[^"]*)"',source)))
missing=[n for n in calls if n.encode()+b'\0' not in unity]
bridge=(root/'src/bridge.cpp').read_text()
attach=bridge[bridge.index('void InstallGameHooks('):bridge.index('void BeginLoaderHooks(')]
assert not re.search(r'\b(klass|class_method|invoke|class_init|domain_get)\s*\(',attach), 'Early managed API regression'
assert 'UnityEngine.Object::op_Implicit' not in source
assert 'UnityEngine.Transform::set_forward_Injected' not in source
assert 'position[0]/worldScale' not in (root/'src/view_math.h').read_text()
sys.path.insert(0,str(root/'vendor/python'))
from elftools.elf.elffile import ELFFile
with (root/'original/libil2cpp.so').open('rb') as f:
    elf=ELFFile(f);exports={s.name for s in elf.get_section_by_name('.dynsym').iter_symbols()}
required=sorted(set(re.findall(r'API\(\w+,"([^"]+)"\)',bridge)))
missing_exports=[s for s in required if s not in exports]
assert '#include "ground_grace.h"' not in bridge
dump=(root/'vendor/dumper/dump.cs').read_text(encoding='utf-8-sig')
managed_methods={'Mesh':['public void .ctor()', 'public void set_vertices(Vector3[] value)', 'public void set_normals(Vector3[] value)', 'public void set_triangles(int[] value)', 'public void RecalculateBounds()'], 'MeshFilter':['public void set_sharedMesh(Mesh value)'], 'Material':['private void SetColorImpl(int name, Color value)']}
for cls,signatures in managed_methods.items():
    block=re.search(r'(?m)^public (?:sealed )?class '+cls+r'\b[^\n]*\n\{(.*?)(?=\n\})',dump,re.S)
    assert block,cls
    for signature in signatures:assert block.group(1).count(signature+' { }')==1,(cls,signature)
report={'icalls':calls,'missing_icall_strings':missing,'exports':required,'missing_exports':missing_exports,'managed_glove_signatures':managed_methods,'early_managed_lookup_guard':True,'headset_tested':False}
(root/'out/candidate-abi-audit.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
if missing or missing_exports:sys.exit(1)
