cmake_minimum_required(VERSION 3.5)

if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()

project(StorageClientApp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(Boost_NO_WARN_NEW_VERSIONS 1)

option(CLIENT_APP "hides dht_pkt logs" ON)

if (NOT MSVC)
    set(CMAKE_CXX_FLAGS "-pthread -fno-strict-aliasing")
endif ()

if (MSVC)
    add_definitions(-D_BOOST_LOG_DLL)
    add_definitions(-D_BOOST_ALL_DYN_LINK)
    add_definitions(-D_BOOST_USE_WINAPI_VERSION=0x0A00)
    add_definitions(-D_BOOST_ALL_NO_LIB)
    add_definitions(-D_WIN32_WINNT=0x0601 /w44287 /w44388)
    add_definitions(-DNOMINMAX)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-D _WIN32_WINNT=0x0601)

    set(Boost_USE_STATIC_LIBS OFF)
    set(Boost_USE_MULTITHREADED ON)
    set(Boost_USE_STATIC_RUNTIME OFF)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /EHsc /DBOOST_LOG_DYN_LINK")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd /D_SCL_SECURE_NO_WARNINGS /wd4701")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MD /Zi")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")

    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG:FASTLINK")
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /DEBUG")
endif ()

if (MSVC)
    set(BOOST_STACKTRACE_BACKTRACE_INCLUDE_FILE "${Bost_INCLUDE_DIR}boost/stacktrace.hpp")
endif()

find_package(Threads REQUIRED)
find_package(Boost 1.71.0 REQUIRED COMPONENTS atomic system date_time regex timer chrono log log_setup thread filesystem program_options random REQUIRED)
if (MSVC)
else()
    find_package(OpenSSL 1.1.1 REQUIRED)
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath -Wl,./lib")
endif()

if (APPLE)
    # NOTE: Don't include the path in MACOSX_BUNDLE_ICON_FILE -- this is
    # the property added to Info.plist
    set(MACOSX_BUNDLE_ICON_FILE icon.png)

    # And this part tells CMake where to find and install the file itself
    set(APP_ICON_MACOSX ./resources/icons/icon.png)
    set_source_files_properties(${APP_ICON_MACOSX} PROPERTIES
            MACOSX_PACKAGE_LOCATION "resources/icons")

    set(CMAKE_VERBOSE_MAKEFILE ON)
    set(BOOST_INCLUDE_DIR "/usr/local/include")
    set(BOOST_LIB_DIR "/usr/local/lib")
    set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
    set(OPENSSL_LIBRARIES "/usr/local/opt/openssl/lib")


#    include_directories(${Boost_INCLUDE_DIRS} /usr/local/opt/openssl@1.1/include)
endif()

add_definitions(-DSIRIUS_DRIVE_MULTI)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Multimedia MultimediaWidgets Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Multimedia MultimediaWidgets Widgets LinguistTools)

set(PROJECT_ROOT_DIR ..)
include_directories(${PROJECT_ROOT_DIR}/cpp-xpx-storage-sdk/include)
include_directories(${PROJECT_ROOT_DIR}/cpp-xpx-storage-sdk/cereal/include)
include_directories(${PROJECT_ROOT_DIR}/cpp-xpx-storage-sdk/cpp-xpx-rpclib/include)
include_directories(${PROJECT_ROOT_DIR}/cpp-xpx-storage-sdk/libtorrent/include)

set(TS_FILES StorageClientApp_en_US.ts)

file(GLOB PROJECT_SOURCES
        "*.h"
        "*.cpp"
        "*.ui"
        "Dialogs/*.h"
        "Dialogs/*.cpp"
        "Dialogs/*.ui"
        "Models/*.h"
        "Models/*.cpp"
        "Engines/*.h"
        "Engines/*.cpp"
        "Entities/*.h"
        "Entities/*.cpp"
        "ErrorCodeTranslator/*.h"
        "ErrorCodeTranslator/*.cpp"
        "ErrorCodeTranslator/*.txt"
        "SignalEmitters/*.h"
        "SignalEmitters/*.cpp"

        ${TS_FILES}
)

file(GLOB PROJECT_UI_SOURCES
        "*.ui"
)
qt_wrap_ui(SOURCES ${PROJECT_UI_SOURCES} mainwindow.ui)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(StorageClientApp
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES} main.cpp ${APP_ICON_MACOSX}
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
        add_executable(StorageClientApp ${PROJECT_SOURCES} main.cpp ${APP_ICON_MACOSX})
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

if (WIN32)
    target_link_libraries(StorageClientApp PRIVATE
            ${Boost_LIBRARIES}
            Qt${QT_VERSION_MAJOR}::Widgets
            Qt${QT_VERSION_MAJOR}::Multimedia
            Qt${QT_VERSION_MAJOR}::MultimediaWidgets
            wsock32
            ws2_32
            torrent-sirius
            drive
            connection
            sdk_external
            nodediscovery
            ionet
            storage-crypto
            net
            model
            utils
            xpxchaincpp)

    target_compile_definitions(StorageClientApp
            PRIVATE
            BOOST_EXCEPTION_DISABLE
            )
else()
    target_link_libraries(StorageClientApp PRIVATE ${Boost_LIBRARIES} Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Multimedia Qt${QT_VERSION_MAJOR}::MultimediaWidgets torrent-sirius drive connection sdk_external nodediscovery ionet storage-crypto net model utils xpxchaincpp)
endif()

set_target_properties(StorageClientApp PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
)

if (MSVC)
    target_compile_options(StorageClientApp PUBLIC "/Zc:__cplusplus")
endif ()

set(simpletreemodel_resource_files
    "tree.txt"
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(StorageClientApp)
endif()
