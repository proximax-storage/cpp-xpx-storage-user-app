# cpp-xpx-storage-user-app

## Prerequisites

* C++ compliler with C++20 support
* CMake 3.9 or higher
* Qt 5.12.8 or higher
* Boost 1.71.0 or higher


## Build for MacOS
```shell
git clone https://github.com/proximax-storage/cpp-xpx-storage-user-app.git
cd cpp-xpx-storage-user-app
git submodule update --init --recursive
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

## Running in Linux (prebuilt binary)
### Dependencies
```shell
sudo apt-get install libxcb-cursor0
```

### Make executable (optional)
```shell
chmod a+x ./StorageClientApp
```

### Run (CLI)
```shell
./StorageClientApp
```