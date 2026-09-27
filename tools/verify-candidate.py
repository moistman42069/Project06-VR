"""Verify final APK payload/ABI against its receipt and the pinned original."""
import hashlib,io,json,pathlib,sys,zipfile
root=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'vendor/python'))
from elftools.elf.elffile import ELFFile
receipt=json.loads((root/'out/p06-quest-0.1.15-receipt.json').read_text())
apk=root/'out'/receipt['apk'];original=pathlib.Path(r'C:\Users\Shadow\Downloads\p-06RELEASE64.apk')
def sha_file(path):
    with path.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
assert sha_file(apk)==receipt['apk_sha256']
assert sha_file(original)==receipt['base_apk_sha256']
assert sha_file(root/'out/generated/sonic_gloves.inc')==receipt['generated_gloves_sha256']
report={'apk_sha256':receipt['apk_sha256'],'headset_tested':False,'preserved_game_entries_sha256_match':True,'preserved_game_entry_count':len(receipt['preserved_entries']),'native_libraries':[]}
with zipfile.ZipFile(apk) as final,zipfile.ZipFile(original) as base:
    for entry in receipt['preserved_entries']:
        name=entry['name']
        with final.open(name) as a,base.open(name) as b:assert hashlib.file_digest(a,'sha256').digest()==hashlib.file_digest(b,'sha256').digest(),name
    for name in final.namelist():
        if not(name.startswith('lib/arm64-v8a/') and name.endswith('.so')):continue
        blob=final.read(name);elf=ELFFile(io.BytesIO(blob));assert elf['e_machine']=='EM_AARCH64',name
        needed=[t.needed for t in elf.get_section_by_name('.dynamic').iter_tags() if t.entry.d_tag=='DT_NEEDED']
        alignment=[seg['p_align'] for seg in elf.iter_segments() if seg['p_type']=='PT_LOAD']
        report['native_libraries'].append({'library':name,'needed':needed,'load_alignments':alignment,'sha256':hashlib.sha256(blob).hexdigest()})
        if name.endswith(('libp06quest.so','libopenxr_loader.so')):assert all(n>=16384 for n in alignment),name
        if name.endswith('libp06quest.so'):
            assert hashlib.sha256(blob).hexdigest()==receipt['native_plugin_sha256']
            symbols={s.name for s in elf.get_section_by_name('.dynsym').iter_symbols()}
            for symbol in ['UnityPluginLoad','JNI_OnLoad','Java_com_p06_quest_QuestActivity_nativePrepare','Java_com_p06_quest_QuestActivity_nativeAttach']:assert symbol in symbols,symbol
(root/'out/compatibility-audit.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS: signed artifact hash, pinned original hash, every preserved entry SHA-256, ARM64 libraries, plugin exports and 16-KiB new-library alignment')
