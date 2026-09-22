import sys,struct,zipfile,json,pathlib
sys.path.insert(0,'vendor/unitypy')
import lz4.block
with zipfile.ZipFile(r'C:\Users\Shadow\Downloads\p-06RELEASE64.apk') as z:
 f=z.open('assets/bin/Data/data.unity3d')
 def read(fmt):return struct.unpack(fmt,f.read(struct.calcsize(fmt)))
 def cstr():
  b=bytearray()
  while (x:=f.read(1)) not in (b'\0',b''):b+=x
  return b.decode()
 sig=cstr();version=read('>I')[0];uv=cstr();gen=cstr();size,ci,ui,flags=read('>QIII');f.read((-f.tell())%16)
 info=lz4.block.decompress(f.read(ci),uncompressed_size=ui);p=16
 n=struct.unpack_from('>I',info,p)[0];p+=4;blocks=[]
 for i in range(n):u,c,fl=struct.unpack_from('>IIH',info,p);p+=10;blocks.append((u,c,fl))
 n=struct.unpack_from('>I',info,p)[0];p+=4;nodes=[]
 for i in range(n):
  offset,length,fl=struct.unpack_from('>QQI',info,p);p+=20;end=info.index(0,p);name=info[p:end].decode();p=end+1;nodes.append({'name':name,'offset':offset,'size':length})
 print('BUNDLE',sig,gen,'blocks',len(blocks),'nodes',len(nodes));print(nodes[:12]);pathlib.Path('evidence/bundle-directory.json').write_text(json.dumps(nodes,indent=2))
 if flags&0x200:f.read((-f.tell())%16)
 selected=sys.argv[1:] or ['globalgamemanagers','globalgamemanagers.assets','level0','sharedassets0.assets']
 targets=[x for x in nodes if x['name'] in selected]
 outputs={x['name']:bytearray() for x in targets};offset=0
 for u,c,fl in blocks:
  relevant=[x for x in targets if x['offset']<offset+u and x['offset']+x['size']>offset]
  if relevant:
   b=f.read(c);data=lz4.block.decompress(b,uncompressed_size=u) if fl&63 in [2,3] else b
   for x in relevant:outputs[x['name']]+=data[max(0,x['offset']-offset):min(u,x['offset']+x['size']-offset)]
  else:f.seek(c,1)
  offset+=u
  if all(len(outputs[x['name']])==x['size'] for x in targets):break
 r=pathlib.Path('original/assets');r.mkdir(exist_ok=True)
 for x in targets:assert len(outputs[x['name']])==x['size'];(r/x['name']).write_bytes(outputs[x['name']]);print('Extracted',x['name'],x['size'])
