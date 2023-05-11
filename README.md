# cpp-xpx-storage-user-app

## Prerequisites

* C++ compliler with C++20 support
* CMake 3.9 or higher
* [Conan] (https://conan.io)
* Qt 5.12.8 or higher
* Boost 1.71.0 or higher 
* [Cereal] 1.3.2 or higher (https://github.com/USCiLab/cereal)


## Build for MacOS
```shell
git clone https://github.com/proximax-storage/cpp-xpx-storage-user-app.git
cd cpp-xpx-storage-user-app
git submodule update --init --recursive --remote
```

### Build cpp-xpx-storage-user-app
```shell
mkdir build && cd build
cmake -DSIRIUS_DRIVE_MULTI=ON ..
```

### Building with *nix Make
```shell
make -j 4
sudo make install
```

## Web Deployment

### Links
Reference for Web Deployment: https://doc.qt.io/qtcreator/creator-setup-webassembly.html

Compatible emscipten version for Qt version 6: https://doc.qt.io/qt-6/wasm.html

Compatible emscipten version for Qt version 5: https://doc.qt.io/qt-5/wasm.html

emscripten Introduction: https://emscripten.org/docs/introducing_emscripten/index.html

emscripten Download: https://emscripten.org/docs/getting_started/downloads.html

Verifying emscripten installation: https://emscripten.org/docs/getting_started/Tutorial.html#tutorial

### Additional Prerequisites
* Qt WebAssembly 5.15 or later (same version as your installed Qt)
(for now we are using Qt 6.4.3) (<-- to be removed, need to clarify if this works well in Qt 6.5)
* emscripten (need to install version that is compatible with your Qt version)

### Install Qt WebAssembly
Normally, this should have been installed together with your Qt application if you installed the whole components.
![image](https://github.com/proximax-storage/cpp-xpx-storage-user-app/assets/121498420/ae00de89-faea-451f-9735-3f751f4c8549)

If you have not installed Qt WebAssembly, you can navigate to your installed Qt folder/directory. And run the MaintenanceTool application/executable.
![image](https://github.com/proximax-storage/cpp-xpx-storage-user-app/assets/121498420/bc09a41c-9df4-4592-b4e3-67c8804723ff)

Similar to how you installed Qt at the first time, you just need to install the Qt WebAssembly component (with the same version as your installed Qt).
Just tick the WebAssembly option, and press `Next`
![image](https://github.com/proximax-storage/cpp-xpx-storage-user-app/assets/121498420/870ec821-2dd6-49ca-90e4-0f521a97efe1)

### Install emscripten

### Web Deploy



