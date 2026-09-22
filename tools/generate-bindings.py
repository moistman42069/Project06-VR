import re,pathlib,sys,json
sys.path.insert(0,'vendor/python')
from elftools.elf.elffile import ELFFile
s=pathlib.Path('vendor/dumper/dump.cs').read_text(encoding='utf-8-sig')
spec=[('TitleStart','TitleScreen','private void Start()'),('Axis','CF2Input','public static float GetAxis(string axisName)'),('AxisRaw','CF2Input','public static float GetAxisRaw(string axisName)'),('Held','CF2Input','public static bool GetButton(string axisName)'),('Down','CF2Input','public static bool GetButtonDown(string axisName)'),('Up','CF2Input','public static bool GetButtonUp(string axisName)')]
f=open('original/libil2cpp.so','rb');e=ELFFile(f);out=['#pragma once','#include <cstdint>','struct Binding { uintptr_t rva; unsigned char bytes[16]; };'];record=[]
for tag,cls,signature in spec:
 blocks=re.findall(r'(?m)^public (?:(?:sealed|static|abstract) )?class '+cls+r'\b[^\n]*\n\{(.*?)(?=\n\})',s,re.S)
 matches=[]
 for b in blocks:
  m=re.search(r'// RVA: (0x[0-9A-F]+)[^\n]*\n\s*'+re.escape(signature),b)
  if m: matches.append(int(m[1],16))
 assert len(matches)==1,(tag,matches)
 a=matches[0];seg=next(x for x in e.iter_segments() if x['p_type']=='PT_LOAD' and x['p_vaddr']<=a<x['p_vaddr']+x['p_filesz']);f.seek(a-seg['p_vaddr']+seg['p_offset']);b=f.read(16)
 out.append('constexpr Binding k'+tag+' = {0x%x, {%s}};'%(a,','.join('0x%02x'%v for v in b)))
 record.append({'name':tag,'method':cls+'.'+signature,'rva':hex(a),'prologue':b.hex()})
pathlib.Path('src/bindings.h').write_text('\n'.join(out)+'\n');pathlib.Path('evidence/bindings.json').write_text(json.dumps(record,indent=2));print(record)
