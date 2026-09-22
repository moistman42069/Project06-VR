import pathlib,urllib.request,zipfile,concurrent.futures,json
r=pathlib.Path('vendor')
j=[{'binary':{'package':{'link':'https://github.com/adoptium/temurin17-binaries/releases/download/jdk-17.0.20.1%2B1/OpenJDK17U-jdk_x64_windows_hotspot_17.0.20.1_1.zip'}}}]
items=[('ndk','https://dl.google.com/android/repository/android-ndk-r27c-windows.zip'),('build-tools','https://dl.google.com/android/repository/build-tools_r35_windows.zip'),('platform','https://dl.google.com/android/repository/platform-35_r02.zip'),('jdk',j[0]['binary']['package']['link'])]
def fetch(pair):
 n,u=pair;p=r/(n+'.zip');print('Downloading',n,flush=True);urllib.request.urlretrieve(u,p)
 with zipfile.ZipFile(p) as z: z.extractall(r/n)
 print('Ready',n,flush=True)
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as e:list(e.map(fetch,items))
