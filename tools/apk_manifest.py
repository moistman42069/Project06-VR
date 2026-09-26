"""Patch Android binary XML without rebuilding or renumbering game resources."""
import struct

NONE = 0xFFFFFFFF
ANDROID = 'http://schemas.android.com/apk/res/android'
IDS = {'name': 0x01010003, 'label': 0x01010001, 'required': 0x0101028e,
       'version': 0x01010519, 'value': 0x01010024, 'authorities': 0x01010018}

class Manifest:
    def __init__(self, data):
        assert struct.unpack_from('<HH', data) == (3, 8)
        self.strings = []; self.resource_ids = []; self.events = []
        offset = 8
        while offset < len(data):
            kind, header, size = struct.unpack_from('<HHI', data, offset)
            assert size >= header and offset + size <= len(data)
            chunk = data[offset:offset+size]
            if kind == 1:
                n, styles, flags, start, style_start = struct.unpack_from('<5I', chunk, 8)
                assert styles == 0, 'Styled manifest string pool is unsupported'
                for i in range(n):
                    pos = start + struct.unpack_from('<I', chunk, header+i*4)[0]
                    if flags & 0x100:
                        def length8(pos):
                            v=chunk[pos];return ((v&127)*256+chunk[pos+1],pos+2) if v&128 else (v,pos+1)
                        _, pos=length8(pos);length,pos=length8(pos);s=chunk[pos:pos+length].decode('utf-8')
                    else:
                        length=struct.unpack_from('<H',chunk,pos)[0];pos+=2
                        if length&0x8000:length=((length&0x7fff)<<16)|struct.unpack_from('<H',chunk,pos)[0];pos+=2
                        s=chunk[pos:pos+length*2].decode('utf-16le')
                    self.strings.append(s)
            elif kind == 0x180:
                self.resource_ids=list(struct.unpack_from('<%dI'%((size-8)//4),chunk,8))
            else:self.events.append(chunk)
            offset+=size
        assert offset == len(data)

    def string(self, value):
        if value in self.strings:return self.strings.index(value)
        self.strings.append(value);return len(self.strings)-1

    def attribute(self,name,value,kind=3,android=True):
        index=self.string(name)
        if android:
            while len(self.resource_ids)<=index:self.resource_ids.append(0)
            assert name in IDS,name
            self.resource_ids[index]=IDS[name]
        raw=self.string(value) if kind==3 else NONE
        return [self.string(ANDROID) if android else NONE,index,raw,8,0,kind,raw if kind==3 else value]

    def start(self,name,attrs=(),line=1):
        attrs=sorted(attrs,key=lambda a:self.resource_ids[a[1]] if a[1]<len(self.resource_ids) else 0)
        ext=struct.pack('<IIHHHHHH',NONE,self.string(name),20,20,len(attrs),0,0,0)
        values=b''.join(struct.pack('<IIIHBBI',*a) for a in attrs)
        return struct.pack('<HHIII',0x102,16,36+len(values),line,NONE)+ext+values

    def end(self,name,line=1):
        return struct.pack('<HHIIIII',0x103,16,24,line,NONE,NONE,self.string(name))

    def node(self,name,attrs=()):return [self.start(name,attrs),self.end(name)]

    def edit(self):
        result=[]
        stack=[]
        for chunk in self.events:
            kind=struct.unpack_from('<H',chunk)[0]
            if kind==0x102:
                name=self.strings[struct.unpack_from('<I',chunk,20)[0]]
                count=struct.unpack_from('<H',chunk,28)[0]
                attrs=[list(struct.unpack_from('<IIIHBBI',chunk,36+i*20)) for i in range(count)]
                def replace(attr,value,typ=3):
                    for a in attrs:
                        if self.strings[a[1]]==attr:
                            a[2]=self.string(value) if typ==3 else NONE;a[5]=typ;a[6]=self.string(value) if typ==3 else value;return
                    raise ValueError('Missing original attribute '+attr)
                if name=='manifest':
                    replace('package','com.p06.quest');replace('versionName','0.1.4-vr-candidate');replace('versionCode',104,0x10)
                if name=='uses-sdk':replace('minSdkVersion',29,0x10)
                if name=='application':replace('label','Project 06 Quest')
                if name=='activity':replace('name','com.p06.quest.QuestActivity')
                if name=='meta-data':
                    values={self.strings[a[1]]:self.strings[a[6]] for a in attrs if a[5]==3}
                    if values.get('name')=='unity.splash-enable':replace('value',0,0x12)
                result.append(self.start(name,attrs,struct.unpack_from('<I',chunk,8)[0]));stack.append(name)
            elif kind==0x103:
                name=self.strings[struct.unpack_from('<I',chunk,20)[0]]
                assert stack.pop()==name
                if name=='intent-filter':
                    for cat in ['com.oculus.intent.category.VR','org.khronos.openxr.intent.category.IMMERSIVE_HMD']:
                        result+=self.node('category',[self.attribute('name',cat)])
                if name=='application':
                    for key,value in [('com.samsung.android.vr.application.mode','vr_only'),('com.oculus.supportedDevices','quest3'),('com.oculus.focusaware','true')]:
                        result+=self.node('meta-data',[self.attribute('name',key),self.attribute('value',0xffffffff,0x12) if value=='true' else self.attribute('value',value)])
                if name=='manifest':
                    result+=self.node('uses-feature',[self.attribute('name','android.hardware.vr.headtracking'),self.attribute('version',1,0x10),self.attribute('required',0xffffffff,0x12)])
                    result.append(self.start('queries'))
                    result+=self.node('provider',[self.attribute('authorities','org.khronos.openxr.runtime_broker;org.khronos.openxr.system_runtime_broker')])
                    result.append(self.end('queries'))
                result.append(chunk)
            else:result.append(chunk)
        assert not stack
        self.events=result

    def bytes(self):
        encoded=[];offsets=[];offset=0
        for s in self.strings:
            b=s.encode('utf-16le');n=len(b)//2;assert n<32768
            item=struct.pack('<H',n)+b+b'\0\0';offsets.append(offset);encoded.append(item);offset+=len(item)
        strings=b''.join(encoded);strings+=b'\0'*((-len(strings))%4)
        start=28+len(offsets)*4;pool=struct.pack('<HHI5I',1,28,start+len(strings),len(offsets),0,0,start,0)+struct.pack('<%dI'%len(offsets),*offsets)+strings
        resource_map=struct.pack('<HHI',0x180,8,8+len(self.resource_ids)*4)+struct.pack('<%dI'%len(self.resource_ids),*self.resource_ids)
        body=pool+resource_map+b''.join(self.events)
        return struct.pack('<HHI',3,8,8+len(body))+body

def patch_manifest(original):
    manifest=Manifest(original);manifest.edit();result=manifest.bytes();Manifest(result)
    return result
