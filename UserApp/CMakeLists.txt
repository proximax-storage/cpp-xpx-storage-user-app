cmake_minimum_required(VERSION 3.5)

project(StorageClientApp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-pthread -fno-strict-aliasing")

if ( APPLE)
    set(CMAKE_PREFIX_PATH "~/Qt/6.2.4/macos")

    find_package(Threads REQUIRED)
    find_package(Boost 1.71.0 REQUIRED)

    set(CMAKE_VERBOSE_MAKEFILE ON)
    set(BOOST_INCLUDE_DIR "/usr/local/include")
    set(BOOST_LIB_DIR "/usr/local/lib")
    set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
    set(OPENSSL_LIBRARIES "/usr/local/opt/openssl/lib")


#    include_directories(${Boost_INCLUDE_DIRS} /usr/local/opt/openssl@1.1/include)
endif()

add_definitions(-DSIRIUS_DRIVE_MULTI)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)

set(PROJECT_ROOT_DIR ..)
include_directories(${PROJECT_ROOT_DIR}/cpp-xpx-storage-sdk/include)
include_directories(${PROJECT_ROOT_DIR}/cpp-xpx-storage-sdk/cereal/include)
include_directories(${PROJECT_ROOT_DIR}/cpp-xpx-storage-sdk/cpp-xpx-rpclib/include)
include_directories(${PROJECT_ROOT_DIR}/cpp-xpx-storage-sdk/libtorrent/include)

set(TS_FILES StorageClientApp_en_US.ts)

set(PROJECT_SOURCES
        main.cpp
        mainwin.h               mainwin.cpp                 mainwin.ui
        SettingsDialog.h        SettingsDialog.cpp          SettingsDialog.ui
        PrivKeyDialog.h         PrivKeyDialog.cpp           PrivKeyDialog.ui
        Model.h                 Model.cpp
        Settings.h              Settings.cpp
        StorageEngine.h         StorageEngine.cpp
        FsTreeTableModel.h      FsTreeTableModel.cpp
        DownloadsTableModel.h   DownloadsTableModel.cpp
        DriveTreeModel.h        DriveTreeModel.cpp
        DiffTableModel.h        DiffTableModel.cpp
        Diff.h                  Diff.cpp
        LocalDriveItem.h        BlockchainEngine.cpp
        BlockchainEngine.h      Worker.cpp
        Worker.h
        ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(StorageClientApp
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET StorageClientApp APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(StorageClientApp SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(StorageClientApp
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(StorageClientApp PRIVATE Qt${QT_VERSION_MAJOR}::Widgets torrent-sirius drive connection sdk_external nodediscovery ionet crypto net model utils xpxchaincpp)

set_target_properties(StorageClientApp PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

set(simpletreemodel_resource_files
    "tree.txt"
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(StorageClientApp)
endif()
