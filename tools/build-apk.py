"""Build and sign the standalone Quest candidate without touching installed apps."""
import argparse, hashlib, json, os, pathlib, shutil, subprocess, sys, zipfile
from apk_manifest import patch_manifest

ROOT = pathlib.Path(__file__).resolve().parents[1]
os.chdir(ROOT)
OUT = ROOT / 'out'
OUT.mkdir(exist_ok=True)
parser=argparse.ArgumentParser()
parser.add_argument('--apk',default=r'C:\Users\Shadow\Downloads\p-06RELEASE64.apk')
parser.add_argument('--skip-native',action='store_true')
args=parser.parse_args()
original=pathlib.Path(args.apk).resolve()
identity=json.loads((ROOT/'evidence/apk-identity.json').read_text())
def digest(p):
    with open(p,'rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
assert digest(original)==identity['sha256'],'Wrong base APK: refusing to use pinned native bindings'
jdk=next((ROOT/'vendor/jdk').glob('jdk-*'))/'bin'
android=ROOT/'vendor/build-tools/android-15'
platform=ROOT/'vendor/platform/android-35/android.jar'
env=os.environ.copy();env['JAVA_HOME']=str(jdk.parent);env['PATH']=str(jdk)+os.pathsep+env.get('PATH','')
def run(command,log):
    print('Running',log,flush=True)
    with (OUT/log).open('w',encoding='utf-8') as f:
        result=subprocess.run([str(x) for x in command],env=env,stdout=f,stderr=subprocess.STDOUT)
    if result.returncode:
        print((OUT/log).read_text(encoding='utf-8',errors='replace')[-10000:]);raise SystemExit(result.returncode)
if not args.skip_native:run(['cmake','--build','build','--target','p06quest','-j','6'],'native-build.log')
classes=OUT/'java-classes';classes.mkdir(exist_ok=True)
java_sources=list((ROOT/'src/java').rglob('*.java'))+list((ROOT/'tools/java-stubs').rglob('*.java'))
run([jdk/'javac.exe','-encoding','UTF-8','-source','8','-target','8','-classpath',platform,'-d',classes,*java_sources],'javac.log')
dex=OUT/'dex';dex.mkdir(exist_ok=True)
class_files=list((classes/'com/p06/quest').glob('*.class'))
assert class_files
run([jdk/'java.exe','-cp',android/'lib/d8.jar','com.android.tools.r8.D8','--release','--min-api','29','--lib',platform,'--classpath',classes,'--output',dex,*class_files],'d8.log')
manifest=None
with zipfile.ZipFile(original) as z:manifest=patch_manifest(z.read('AndroidManifest.xml'))
(OUT/'AndroidManifest.xml').write_bytes(manifest)
additions={
    'AndroidManifest.xml':manifest,
    'classes2.dex':(dex/'classes.dex').read_bytes(),
    'lib/arm64-v8a/libp06quest.so':(ROOT/'build/libp06quest.so').read_bytes(),
    'lib/arm64-v8a/libopenxr_loader.so':(ROOT/'build/vendor/OpenXR-SDK/src/loader/libopenxr_loader.so').read_bytes(),
    'assets/bin/Data/UnitySubsystems/P06Quest/UnitySubsystemsManifest.json':(ROOT/'src/UnitySubsystemsManifest.json').read_bytes(),
}
unsigned=OUT/'p06-quest-0.1.0-unsigned.apk'
print('Packaging preserved game payload and Quest plugin',flush=True)
preserved=[]
with zipfile.ZipFile(original) as source, zipfile.ZipFile(unsigned,'w',allowZip64=True) as target:
    for info in source.infolist():
        if info.filename in additions or info.filename.startswith(('META-INF/','lib/armeabi-v7a/')):continue
        clone=zipfile.ZipInfo(info.filename,info.date_time);clone.compress_type=info.compress_type;clone.external_attr=info.external_attr
        with source.open(info) as reader, target.open(clone,'w',force_zip64=info.file_size>0x7fffffff) as writer:shutil.copyfileobj(reader,writer,8*1024*1024)
        saved=target.getinfo(info.filename);assert saved.CRC==info.CRC and saved.file_size==info.file_size
        preserved.append({'name':info.filename,'crc32':info.CRC,'size':info.file_size})
        if info.file_size>100_000_000:print('Preserved',info.filename,info.file_size,flush=True)
    for name,data in additions.items():
        target.writestr(name,data,compress_type=zipfile.ZIP_STORED)
aligned=OUT/'p06-quest-0.1.0-aligned.apk'
run([android/'zipalign.exe','-f','-P','16','4',unsigned,aligned],'zipalign.log')
keys=ROOT/'keys';keys.mkdir(exist_ok=True);key=keys/'p06quest-development.jks'
if not key.exists():run([jdk/'keytool.exe','-genkeypair','-keystore',key,'-alias','p06quest','-storepass','android','-keypass','android','-dname','CN=P06 Quest Local Development','-keyalg','RSA','-keysize','2048','-validity','10000'],'key-generation.log')
apk=OUT/'p06-quest-0.1.0.apk'
run([jdk/'java.exe','-jar',android/'lib/apksigner.jar','sign','--ks',key,'--ks-key-alias','p06quest','--ks-pass','pass:android','--key-pass','pass:android','--out',apk,aligned],'apk-signing.log')
run([jdk/'java.exe','-jar',android/'lib/apksigner.jar','verify','--verbose','--print-certs',apk],'apk-signature-verification.log')
run([android/'zipalign.exe','-c','-P','16','4',apk],'apk-alignment-verification.log')
run([android/'aapt2.exe','dump','xmltree','--file','AndroidManifest.xml',apk],'packaged-manifest.txt')
run([android/'dexdump.exe',dex/'classes.dex'],'quest-dexdump.txt')
with zipfile.ZipFile(apk) as final:
    assert len(final.namelist())==len(set(final.namelist()))
    for item in preserved:
        info=final.getinfo(item['name']);assert (info.CRC,info.file_size)==(item['crc32'],item['size'])
    for name,data in additions.items():assert final.read(name)==data
    assert not any(n.startswith('lib/armeabi-v7a/') for n in final.namelist())
manifest_text=(OUT/'packaged-manifest.txt').read_text(encoding='utf-8',errors='replace')
for text in ['com.p06.quest.QuestActivity','com.p06.quest','Project 06 Quest','android.hardware.vr.headtracking','IMMERSIVE_HMD','org.khronos.openxr.runtime_broker','com.oculus.focusaware']:
    assert text in manifest_text,text
dex_text=(OUT/'quest-dexdump.txt').read_text(encoding='utf-8',errors='replace')
assert "Class descriptor  : 'Lcom/p06/quest/QuestActivity;'" in dex_text
assert "Class descriptor  : 'Lcom/unity3d/player/UnityPlayerActivity;'" not in dex_text,'Compile-time stub leaked into APK'
source_files=list((ROOT/'src').rglob('*'))+[ROOT/'CMakeLists.txt']
source_hash=hashlib.sha256()
for p in sorted(x for x in source_files if x.is_file()):source_hash.update(p.relative_to(ROOT).as_posix().encode());source_hash.update(p.read_bytes())
receipt={'apk':apk.name,'apk_sha256':digest(apk),'bytes':apk.stat().st_size,'base_apk_sha256':identity['sha256'],'source_tree_sha256':source_hash.hexdigest(),'native_plugin_sha256':digest(ROOT/'build/libp06quest.so'),'preserved_entries':preserved,'added_or_modified_entries':list(additions),'headset_tested':False}
(OUT/'package-receipt.json').write_text(json.dumps(receipt,indent=2),encoding='utf-8')
print('APK ready:',apk,flush=True);print('SHA256:',receipt['apk_sha256'],flush=True)
