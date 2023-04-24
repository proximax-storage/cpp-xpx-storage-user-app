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

## Build for Windows
These instructions are written and ran from the following PC specifications:
![1  PC Settings](https://user-images.githubusercontent.com/121498420/234025467-f9e1ad0f-f4ad-4109-a2ed-56d8a71299bc.png)

### Clone the repository
```shell
git clone https://github.com/proximax-storage/cpp-xpx-storage-user-app.git
cd cpp-xpx-storage-user-app
git submodule update --init --recursive --remote
```

### C++ compiler with C++20 support
Set your preferred IDE to enable C++20 support. 
Make sure that you can run C++20 code. Try running some sample code and see whether it can successfully run and produces correct output.

You can try this sample code:
```
#include <iostream>
int main()
{
    auto result = (10 <=> 20) > 0;
    std::cout << "result: " << result << std::endl;
}

```
The correct output should be:
```result: 0```

In case you are using Visual Studio Code (VS Code), and you are not able to enable C++20 support. You can try to download Microsoft Visual Studio (Visual Studio) instead (see `Troubleshooting` > `C++20 related Issues` section below).

### Install CMake
Open the link: https://cmake.org/download/ 

This is what the page looks like:
![2  CMake Website](https://user-images.githubusercontent.com/121498420/234028864-6a5071a6-aa82-4d38-a797-0572bd79b70a.png)

Choose Windows x64 Installer: `cmake-3.26.3-windows-x86_64.msi` (you need to download the 32 bit version if your PC is 32 bit).

Run the .msi file and install CMake.

This is what it looks like after installation:

![3  CMake Icon](https://user-images.githubusercontent.com/121498420/234028983-60977429-4800-45b1-8a8f-a9f7645b1c09.png)
![4  CMake App](https://user-images.githubusercontent.com/121498420/234030435-dba795d3-6442-4350-871a-94277c9bffad.png)

Using Command Prompt to check if it has been successfully installed:
![5  CMake Check](https://user-images.githubusercontent.com/121498420/234030513-365341fa-7cf6-4890-a1e5-e64bef28aacc.png)

### Install Conan
Open the link: https://conan.io/
This is what the page looks like:
![6  Conan Website](https://user-images.githubusercontent.com/121498420/234030765-59e028ca-a494-4f1b-a680-f7c44c8452de.png)

Press `Downloads`

Choose Windows version (32 bit or 64 bit, depending on your PC), and start the download.
![7  Conan Installation](https://user-images.githubusercontent.com/121498420/234030879-43c965b0-5280-4310-8b7a-a155257ee142.png)

Run the installation file and install Conan.

This is what it looks like after installation:

![8  Conan Icon](https://user-images.githubusercontent.com/121498420/234030946-b15ff529-5bde-46ee-931e-9a535ccb3ebd.png)

![9  Conan App](https://user-images.githubusercontent.com/121498420/234030990-310acb5c-3f50-469b-a945-da381002a49f.png)

Using Command Prompt to check whether it has been successfully installed:
![10  Conan Check](https://user-images.githubusercontent.com/121498420/234031116-0e3f53c8-0af4-46f4-8242-ade0ca77f79b.png)

### Install QT
Open the link: https://www.qt.io/download-open-source 
Scroll down and press `Download the Qt Online Installer`

![11  QT Website](https://user-images.githubusercontent.com/121498420/234031277-2fe42da2-d94b-40d3-916c-6d50cba60c5c.png)

Choose `Windows` and press `Qt Online Installer for Windows`
![12  QT Installer Installation](https://user-images.githubusercontent.com/121498420/234031378-04c569ad-4185-4153-86f9-d460866a0fc9.png)

The installer should look something like this
![13  QT Installer Login](https://user-images.githubusercontent.com/121498420/234031465-e41a45dc-1628-4679-baa8-08d6d6be9e14.png)

Create your account if you don’t have one, then press `Next`

Tick the box, and press `Next`
![14  QT Installer Obligations](https://user-images.githubusercontent.com/121498420/234031638-2523b5dd-d3b1-474c-a7cf-1cf8b1507cd1.png)

Press `Next`
![15  QT Setup](https://user-images.githubusercontent.com/121498420/234031708-95013821-8d7d-4dd4-9b92-38fce294bfc8.png)

Choose whichever option you prefer, and press `Next`
![16  QT Contribute Page](https://user-images.githubusercontent.com/121498420/234031836-c98f9a57-d028-4dab-9469-4008ff283d2a.png)

Specify your installation directory, preferably the one with more space, because the installation will take a lot of space. Then press `Next`.
![17  QT Installer Install Folder](https://user-images.githubusercontent.com/121498420/234031989-35a5ac0e-e492-4787-9e89-754380ed808b.png)

Choose Qt 6.4.3, including all the components within it (including the whole `Additional Libraries` too)
![18  QT Components 1](https://user-images.githubusercontent.com/121498420/234032090-d4c24759-4fb2-42ac-9dcb-003df23992c8.png)

![19  QT Components 2](https://user-images.githubusercontent.com/121498420/234032154-f66f2053-c353-4788-bd9c-98e4445a840f.png)

![20  QT Components 3](https://user-images.githubusercontent.com/121498420/234032230-4b34571d-2569-4797-9012-7c48322a0fc6.png)

And then press `Next`, and wait for the installation.

** Please note that the installation may take time since there are a lot to install.

After installation is complete, you can exit the installer.

According to your installation folder and Qt version, add a new system variable:
```
QT_PATH     C:\Qt\6.2.4\mingw_64
```

### Install MinGW-W64

If you do not have MinGW-W64 installed, download the 12.1.0 MSVCRT runtime version from here https://winlibs.com/

Extract the zip file to your C: drive, then add `C:\mingw64\bin` to PATH in system variables.

### Install Boost 1.79.0

Download boost zip folder from: https://www.boost.org/users/history/version_1_79_0.html

Extract to C: drive.

Now go to C: drive using Command Prompt, then run:

```
cd boost_1_79_0
bootstrap gcc
b2
b2 --build-dir=build/x64 address-model=64 threading=multi --build-type=complete --stagedir=./stage/x64 -j 4
```

** Preferably use command prompt, as Windows PowerShell might not work when running `bootstrap gcc` command

** Note that running the last two b2 commands may take time.

Add the following to your system variables:
```
BOOST_BUILD_PATH    "C:\boost_1_79_0\tools\build"
BOOST_INCLUDEDIR    "C:\boost_1_79_0"
BOOST_LIBRARYDIR    "C:\boost_1_79_0\stage\x64\lib"
BOOST_ROOT          "C:\boost_1_79_0"
```

### Build cpp-xpx-storage-user-app
From cmd, go to the cpp-xpx-storage-user-app directory, then do the following:
```
cd cpp-xpx-storage-client
mkdir _build
cd _build
cmake -G "MinGW Makefiles" -DSIRIUS_DRIVE_MULTI=ON ..
mingw32-make -j 6
```

### Run Storage Client
In the build folder, an executable with the name "cpp-xpx-storage-client.exe" would have been created after the build is complete.
Double click the exe file, and if it crashes, don't worry. A folder with the name "profile" would be created, and file called "settings.ini" will be there.
Open the file and modify the address and bootstrap_nodes settings as follows (assuming blockchain and rest server are running locally):
```
address=localhost:5555
bootstrap_nodes=localhost:3000
```


