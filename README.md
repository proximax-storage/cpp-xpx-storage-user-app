# cpp-xpx-storage-user-app

## Prerequisites

* ***macOS 12.0 or later (macOS Monterey)***
* C++ compliler with C++20 support
* CMake 3.9 or higher
* [Conan] (https://conan.io)
* Qt 5.12.8 or higher
* Boost 1.71.0 or higher 
* [Cereal] 1.3.2 or higher (https://github.com/USCiLab/cereal)


## Build for MacOS
### Install Xcode Command Line Tools (CLT)
```shell
xcode-select --install
```

### Install Homebrew
```shell
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

### Install dependencies/tools
```shell
brew install cmake
```
```shell
brew install conan
```
```shell
brew install qt
```
```shell
brew install boost
```
```shell
brew install cereal
```

### Clone repo
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

### Static link library
```shell
cd /cpp-xpx-storage-user-app/build/UserApp
make clean
qmake -config release
make
```

### View linked library
```shell
otool -L StorageClientApp.app/Contents/MacOs/StorageClientApp
```
