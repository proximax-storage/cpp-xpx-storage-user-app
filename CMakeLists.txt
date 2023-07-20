cmake_minimum_required(VERSION 3.5)

project(cpp-xpx-storage-user-app LANGUAGES CXX)

if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SKIP_GRPC ON)

if (NOT MSVC)
    set(CMAKE_CXX_FLAGS "-pthread -fno-strict-aliasing")
endif ()

if (MSVC)
    add_definitions(-D_BOOST_LOG_DLL)
    add_definitions(-D_BOOST_ALL_DYN_LINK)
    add_definitions(-D_BOOST_USE_WINAPI_VERSION=0x0A00)
    add_definitions(-D_WIN32_WINNT=0x0A00)
endif ()

option(XPX_STORAGE_SDK_BUILD_EMULATOR "Build XPX-STORAGE SDK emulator." OFF)

option(XPX_STORAGE_SDK_NOT_BUILD_EXAMPLES "Not Build XPX-STORAGE SDK examples." ON)

add_definitions(-DSIRIUS_DRIVE_MULTI)

# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check https://doc.qt.io/qt/deployment-android.html for more information.
# They need to be set before the find_package( ...) calls below.

#if(ANDROID)
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#    if (ANDROID_ABI STREQUAL "armeabi-v7a")
#        set(ANDROID_EXTRA_LIBS
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libcrypto.so
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libssl.so)
#    endif()
#endif()

if (APPLE)
    #set(CMAKE_PREFIX_PATH "~/Qt/6.2.4/macos")
    set(CMAKE_PREFIX_PATH "~/Qt/6.4.0/macos")
    set(BOOST_INCLUDE_DIR "/usr/local/include")
    set(BOOST_LIB_DIR "/usr/local/lib")
    set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
    set(OPENSSL_LIBRARIES "/usr/local/opt/openssl/lib")
    
    # for -lcrypto (I do not know who uses it!)
    link_directories(/usr/local/opt/openssl/lib)
endif()

find_package(Threads REQUIRED)
find_package(Boost 1.71.0 REQUIRED)
find_package(QT NAMES Qt6 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

add_subdirectory(${PROJECT_SOURCE_DIR}/cpp-xpx-storage-sdk)
include_directories(${PROJECT_SOURCE_DIR}/cpp-xpx-storage-sdk/include)
include_directories(${PROJECT_SOURCE_DIR}/cpp-xpx-storage-sdk/cereal/include)
include_directories(${PROJECT_SOURCE_DIR}/cpp-xpx-storage-sdk/cpp-xpx-rpclib/include)
include_directories(${PROJECT_SOURCE_DIR}/cpp-xpx-storage-sdk/libtorrent/include)

add_subdirectory(${PROJECT_SOURCE_DIR}/cpp-xpx-chain-sdk)
include_directories(${PROJECT_SOURCE_DIR}/cpp-xpx-chain-sdk/include)

add_subdirectory(UserApp)

# copy resources
if(APPLE)
    file(COPY resources DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/UserApp/StorageClientApp.app/Contents/MacOS)
    file(COPY resources DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/UserApp/Debug/StorageClientApp.app/Contents/)
    file(COPY resources DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/UserApp/StorageClientApp.app/Contents/)
endif()

if(UNIX AND NOT APPLE)
    file(COPY resources DESTINATION ${CMAKE_BINARY_DIR}/UserApp)
endif()
