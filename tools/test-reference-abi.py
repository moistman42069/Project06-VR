"""Execute pinned ARM64 IL2CPP setters to reproduce the bad reference write.

Host regression, not a Quest runtime test. Requires unicorn and pyelftools.
"""
import sys,pathlib,struct,json,hashlib
root=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'vendor/python'))
from elftools.elf.elffile import ELFFile
from unicorn import Uc,UC_ARCH_ARM64,UC_MODE_ARM
from unicorn.arm64_const import UC_ARM64_REG_X0,UC_ARM64_REG_X1,UC_ARM64_REG_X2,UC_ARM64_REG_LR,UC_ARM64_REG_SP
binary=root/'original/libil2cpp.so'
with binary.open('rb') as stream:
    elf=ELFFile(stream);segments=[s for s in elf.iter_segments() if s['p_type']=='PT_LOAD']
    symbols={s.name:s['st_value'] for s in elf.get_section_by_name('.dynsym').iter_symbols()}
    cpu=Uc(UC_ARCH_ARM64,UC_MODE_ARM)
    end=max(s['p_vaddr']+s['p_memsz'] for s in segments)
    cpu.mem_map(0,(end+4095)&~4095)
    for s in segments:cpu.mem_write(s['p_vaddr'],s.data())
cpu.mem_map(0x10000000,0x10000);cpu.mem_map(0x20000000,0x10000)
obj,field,typeinfo,reference=0x20000100,0x20000200,0x20000300,0x20001000
temporary,stop=0x10000100,0x10008000
def u64(address,value):cpu.mem_write(address,struct.pack('<Q',value))
def read(address):return struct.unpack('<Q',cpu.mem_read(address,8))[0]
u64(field+8,typeinfo);cpu.mem_write(field+0x18,struct.pack('<i',0x18));u64(temporary,reference)
def execute(name,value):
    cpu.reg_write(UC_ARM64_REG_SP,0x1000f000);cpu.reg_write(UC_ARM64_REG_LR,stop)
    for reg,v in [(UC_ARM64_REG_X0,obj),(UC_ARM64_REG_X1,field),(UC_ARM64_REG_X2,value)]:cpu.reg_write(reg,v)
    cpu.emu_start(symbols[name],stop,count=1000)
    return read(obj+0x18)
rows=[]
for kind in [0x0e,0x12,0x1c,0x1d]:
    cpu.mem_write(typeinfo+8,struct.pack('<I',kind<<16))
    old=execute('il2cpp_field_set_value',temporary)
    fixed=execute('il2cpp_field_set_value_object',reference)
    assert old==temporary and old!=reference
    assert fixed==reference
    rows.append({'type':hex(kind),'old_stores_stack_address':True,'object_setter_stores_reference':True})
cpu.mem_write(typeinfo+8,struct.pack('<I',0x0c<<16))
cpu.mem_write(temporary,struct.pack('<f',3.25));execute('il2cpp_field_set_value',temporary)
assert struct.unpack('<f',cpu.mem_read(obj+0x18,4))[0]==3.25
report={'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'reference_cases':rows,'float_value_setter_unchanged':True,'setter_rvas':{k:hex(symbols[k]) for k in ['il2cpp_field_set_value','il2cpp_field_set_value_object']},'headset_tested':False}
(root/'evidence/reference-abi-0111.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS: actual ARM64 setters reproduce stale-stack reference for all 4 reference kinds; object setter and float writes verified')
