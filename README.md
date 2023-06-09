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

## User Manual

### Log In
When you open the storage tool application for your first time, you will be prompted to create a storage tool account.
![image](https://github.com/proximax-storage/cpp-xpx-storage-user-app/assets/121498420/d079346f-8c39-44e3-9c9e-05387028ff5d)
<br>Enter your preferred storage tool `Account name`, your `Private key` from your wallet account that have XPX in it, and `Save`.<br><br>

### User Interface Overview
When you have successfully log in, this is what the application will look like:
![image](https://github.com/proximax-storage/cpp-xpx-storage-user-app/assets/121498420/1826bb44-58d7-440c-ae22-ef3373d60c59)

The upper left corner indicated that the application is running on `publicTest` network.

On the upper right corner, we have our wallet `Balance` in XPX, which comes from the wallet account in the `publicTest` network.

We also have the `Gear` icon on the upper right corner of the application interface, which is the `Settings` tab.<br><br>

### Settings
The `Settings` tab will looks like this:
![image](https://github.com/proximax-storage/cpp-xpx-storage-user-app/assets/121498420/3b37dd61-dbdc-475d-84f1-947dc884803e)

Enter the `REST Server Address`, `Replicator Bootstrap Address`, and `Local UDP Port`.

As for the `Account name`, this has usually been set up for you once you have logged in. You can create a `New Account`, and `Copy Account Key` as well.

If the `Download folder` column is empty. You can `Choose directory` from folder/directory from your PC (local folder) to be set up as folder for downloading files within the drive application. This will be used in `Downloads` tab later on.

For the Network section, enter your `Fee Multiplier`.
For now, the application only works if you put `200000`.

You can also choose to set the drive structure style to `Drive structure as tree`.

After that, `Save` your changes and the application will restart.


