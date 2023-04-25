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

In case you are using Visual Studio Code (VS Code), and you are not able to enable C++20 support. You can try to download Microsoft Visual Studio (Visual Studio) instead (see `Troubleshooting (for Windows)` > `C++20 related Issues` section below).

### Install CMake
Open the link: https://cmake.org/download/ 

This is what the page looks like:
![image](https://user-images.githubusercontent.com/121498420/234049026-6b57e98c-d80f-4bbb-8372-c439558bd3ff.png)

Choose Windows x64 Installer: `cmake-3.26.3-windows-x86_64.msi` (you need to download the 32 bit version if your PC is 32 bit).

Run the .msi file and install CMake.

This is what it looks like after installation:

![image](https://user-images.githubusercontent.com/121498420/234049096-22dc1694-3691-4f03-95cf-ed829f6c2661.png)

![image](https://user-images.githubusercontent.com/121498420/234049197-8c47c4c8-2461-4e12-8b6e-6aa09af8936b.png)

Using Command Prompt to check if it has been successfully installed:
![5  CMake Check](https://user-images.githubusercontent.com/121498420/234030513-365341fa-7cf6-4890-a1e5-e64bef28aacc.png)

### Install Conan
Open the link: https://conan.io/

This is what the page looks like:
![image](https://user-images.githubusercontent.com/121498420/234049829-b88d8473-099e-44fc-990c-af5390682dcd.png)

Press `Downloads`

Choose Windows version (32 bit or 64 bit, depending on your PC), and start the download.
![image](https://user-images.githubusercontent.com/121498420/234049736-e1b4078e-6828-4a7d-af43-0fef89d9fb98.png)

Run the installation file and install Conan.

This is what it looks like after installation:

![image](https://user-images.githubusercontent.com/121498420/234049910-54bc6f02-5dfc-4a1f-a12d-89179bd1bc8a.png)

![image](https://user-images.githubusercontent.com/121498420/234050080-d9c22004-7cf4-49ff-b7a3-71c4ed0f3817.png)

Using Command Prompt to check whether it has been successfully installed:

![10  Conan Check](https://user-images.githubusercontent.com/121498420/234031116-0e3f53c8-0af4-46f4-8242-ade0ca77f79b.png)

### Install QT
Open the link: https://www.qt.io/download-open-source 

Scroll down and press `Download the Qt Online Installer`

![image](https://user-images.githubusercontent.com/121498420/234043992-ed5d7f33-0ce6-441b-bf9e-4ac4c543b405.png)

Choose `Windows` and press `Qt Online Installer for Windows`

![image](https://user-images.githubusercontent.com/121498420/234044366-3b1921b5-d8da-427f-bea1-3db2c9caf0db.png)

The installer should look something like this:
![image](https://user-images.githubusercontent.com/121498420/234045148-465cde14-5672-4bd5-92d7-a4c98cf93f3e.png)

Create an account if you don’t have one, then press `Next`

Tick the box, and press `Next`

![image](https://user-images.githubusercontent.com/121498420/234045508-a368ab8b-60bb-43f9-846f-ba76196d613e.png)

Press `Next`
![image](https://user-images.githubusercontent.com/121498420/234045663-6eddb4ee-9e57-49fa-ba92-4fb6e6bcab13.png)

Choose whichever option you prefer, and press `Next`
![image](https://user-images.githubusercontent.com/121498420/234046341-3d35ab19-67a2-44a5-80b1-1e13b8e83e62.png)

Specify your installation directory, preferably the one with more space, because the installation will take a lot of space. Then press `Next`
![image](https://user-images.githubusercontent.com/121498420/234046566-321f8192-10b3-453c-a3f3-8537b665a666.png)

Choose Qt 6.4.3, including all the components within it (including the whole `Additional Libraries` too)
![image](https://user-images.githubusercontent.com/121498420/234048012-c3e60e55-a8fd-47e8-a343-d8cbab517cad.png)

![image](https://user-images.githubusercontent.com/121498420/234048192-8a186e5a-b360-4e91-8154-10f093f53873.png)

![image](https://user-images.githubusercontent.com/121498420/234048415-31a4fea0-3e3e-45e4-8935-1460151b3199.png)

And then press `Next`, and wait for the installation.

** Please note that the installation may take time since there are a lot to install.

After the installation is complete, you can exit the installer.

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

** Preferably use command prompt, as Windows PowerShell might not work when running `bootstrap gcc` command.

** Note that running the last two b2 commands may take time.

Add the following to your system variables:
```
BOOST_BUILD_PATH    "C:\boost_1_79_0\tools\build"
BOOST_INCLUDEDIR    "C:\boost_1_79_0"
BOOST_LIBRARYDIR    "C:\boost_1_79_0\stage\x64\lib"
BOOST_ROOT          "C:\boost_1_79_0"
```

### Cereal

No need to install Cereal as it has been included in this repository.

### Build cpp-xpx-storage-user-app
From cmd, go to the cpp-xpx-storage-user-app directory, then do the following:
```
cd cpp-xpx-storage-user-app
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

## Troubleshooting (for Windows)

### Persisting Errors

If you have just done installations and encounter errors when running any command. Try to close your command prompt, restart your PC, and try again.

In case you encounter the same build error after trying out some solutions, you can try to delete the build folder and then re-build again.

### C++20 related Issues

This link can be useful to enable C++20 support for Visual Studio Code and Microsoft Visual Studio:

https://codevoweb.com/set-up-vs-code-to-write-and-debug-cpp-programs/ 

This link can be useful to enable C++20 support for Microsoft Visual Studio:

https://www.youtube.com/watch?v=XsDR01GMxEI&list=PLmFgNcAR5bdLavrHkKFcwfjeeMw8AnUEn&index=9 

(Video Title: `Using Microsoft Visual Studio 2019 for C++ 20`)

### QT related Issues

Online Installer Error
- If during installation, you encounter an installation error for a particular file. You can try to press `Retry` to re-install that particular file that rises the error. The installation will usually still run as long as there is no error message (including when you just keep `Retry` re-installing the same file that raises an error).

- If the same installation error message keep appearing after so many times, and it stops the installation. You may try to install Qt version 6.5.0 instead, or the latest version.

Offline Installer Error
- If you encounter any issue with the online installer, you can use an offline installer instead. Sometimes the online installer may have some issue, and you may need to wait for the Qt team to fix it. (Note: It is recommended to install Qt via Online installer whenever possible, since adding components later using the Maintenance Tool from the offline installer might be a bit more difficult to do because you need to find the repository for installing additional components)

- Open this link and try to install the offline installer (if possible, try to find and install offline installer version 6 instead): https://download.qt.io/archive/qt/5.12/5.12.12/qt-opensource-windows-x86-5.12.12.exe.mirrorlist 

- Tick the components (similar to set of components in online installer, including Qt State Machine as this can be another issue later if not installed)

- If during installation, you encounter any file installation error. Keep press `Retry`. If the same error still persists after several times, you can try to press `Ignore`. One possible error may look like this (issue with strawberry-perl installation):

&emsp;&emsp;![image](https://user-images.githubusercontent.com/121498420/234160337-a2b9cded-4d00-48c6-86f9-3a0f3b471b97.png)

- In this case, you can try to install strawberry-perl manually:
https://strawberryperl.com/releases.html 

&emsp;&emsp;But if you encounter other file installation error, you can try to search up the file online.

- If you encounter error related to Qt versioning, and you installed Qt version 5.
![image](https://user-images.githubusercontent.com/121498420/234162042-39d7c715-56e4-4726-85f0-c67a956aa339.png)

&emsp;&emsp;You can try to change line of code in `cpp-xpx-storage-user-app/CMakeLists.txt` from:

&emsp;&emsp;`find_package(QT NAMES Qt6 COMPONENTS Widgets REQUIRED)`

&emsp;&emsp;Into:

&emsp;&emsp;`find_package(QT NAMES Qt5 COMPONENTS Widgets REQUIRED)`

&emsp;&emsp;![image](https://user-images.githubusercontent.com/121498420/234164452-7f8ddc2f-f2d1-4f31-8eb6-095d9487b684.png)

### Boost related Issues

**Build and libraries-build issues**

When re-building the Boost or Boost libraries:
- Try to clear the cache for Boost to detect again, or try cleaning up before re-building (b2 clean). And make sure everything is build
- Make sure that the Boost version you built was properly installed
- Remember to reset caches (delete CMake cache) when running CMake
- If you get `missing stacktrace_backtrace` error, and you are still not able to build the stacktrace library by using b2 command, then refer to `Missing stacktrace_backtrace` section below.

**Missing stacktrace_backtrace error**

![image](https://user-images.githubusercontent.com/121498420/234168962-3dde7a51-f45c-44d6-aa01-79941942e01c.png)

If you encounter `missing: stacktrace_backtrace` error on Boost, try the following solution:

1. Exclude/disable stacktrace_backtrace from the required packages in cpp-xpx-storage-user-app/cpp-xpx-storage-sdk/CMakeLists.txt

&emsp;&emsp;You can do this by changing the following:

&emsp;&emsp;`find_package(Boost COMPONENTS atomic system date_time regex timer chrono log thread filesystem program_options random stacktrace_backtrace REQUIRED)`

&emsp;&emsp;Into:

&emsp;&emsp;`find_package(Boost COMPONENTS atomic system date_time regex timer chrono log thread filesystem program_options random REQUIRED)`

&emsp;&emsp;And comment out:

&emsp;&emsp;`add_definitions(-DBOOST_STACKTRACE_USE_BACKTRACE)`

&emsp;&emsp;`set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DSHOW_BACKTRACE")`

&emsp;&emsp;![image](https://user-images.githubusercontent.com/121498420/234169312-621a1baf-288a-4d9a-bfb7-1683d121a161.png)

2. **(Do not use this solution, refer to option 1 instead)** Install libbacktrace (https://github.com/ianlancetaylor/libbacktrace) (instructions to install is written below on section `Libbacktrace installation`)

**Libbacktrace installation** **(Do NOT use this! We need to skip using libbacktrace/stacktrace since there are reports about libbacktrace that causes deadlocks. i.e. Skip this, no need to install libbacktrace. Instead we need to disable the use of backtrace in the project as described in option 1. This is provided only for reference in case it is needed in the future.)**

If your Boost is not able to detect stacktrace library (stacktrace is not found). You can try to install libbacktrace separately.

Open the link:
https://github.com/ianlancetaylor/libbacktrace 

Go to `Code > Download ZIP`

![image](https://user-images.githubusercontent.com/121498420/234170469-7b461605-a847-41ba-b93f-0daa04c70e06.png)

Extract the Zip file on your C drive.

Download MSYS2 by following instructions in this link:
https://www.msys2.org/ 

Just follow step 1 to 5. Step 6 onwards is about downloading MinGW. We do not need to follow it because we already install MinGW version we wanted earlier.

After that, follow these steps:
1. Open MSYS2 UCRT64
2. Navigate to your extracted libbacktrace folder. For example, if you extract it at C:/libbacktrace-master, then use cd /c/libbacktrace-master
3. Run `./configure` by specifying the gcc and g++ file path from the MinGW you installed earlier. For example, `./configure CC=/c/mingw64/bin/gcc.exe CXX=/c/mingw64/bin/g++.exe`
4. Run `make` command. Make sure `libbacktrace.la` is produced, because it is required for the next step.

&emsp;&emsp;If your MSYS does not have `make` command yet, install it first. (Other command installation is similar to this, please search it up on the internet for relevant command installation on MSYS)

&emsp;&emsp;![image](https://user-images.githubusercontent.com/121498420/234170671-0a4d754a-71da-4111-befe-0c469e228848.png)

5. After that, run `./libtool --mode=install /usr/bin/install -c libbacktrace.la '/c/libbacktrace-master'`
6. Go to your installed Boost folder, and find `project-config.jam`
And paste this:
`using gcc : 6 : "C:/mingw64/bin/g++.exe" : <compileflags>-I"C:/libbacktrace-master/" <linkflags>-L"C:/libbacktrace-master/" ;`
7. In Command Prompt, you can run this:
`b2.exe toolset=gcc-6 --with-stacktrace`

The link below is used as reference only for the instructions written above (cited from instructions under the section `MinGW and MinGW-w64 specific notes`). Please follow the instructions written above instead.

https://www.boost.org/doc/libs/1_70_0/doc/html/stacktrace/configuration_and_build.html 

![image](https://user-images.githubusercontent.com/121498420/234170825-edf199bc-4560-4eee-8ae9-d0475ad877ce.png)



