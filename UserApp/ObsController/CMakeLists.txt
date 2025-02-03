cmake_minimum_required(VERSION 3.14)

project(ObsConnection LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

#set(CMAKE_AUTOUIC ON)
#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BOOST_INCLUDE_DIR "/usr/local/include")
set(BOOST_LIB_DIR "/usr/local/lib")
set(BOOST_INCLUDE_DIR "/opt/local/include")
set(BOOST_LIB_DIR "/opt/local/lib")
set(CMAKE_PREFIX_PATH "~/Qt/6.4.0/macos")
set(CMAKE_PREFIX_PATH "~/Qt/6.5.3/macos")

# Find Qt
find_package(Qt6 REQUIRED Core Widgets Svg Network)

#find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core)
#find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

add_definitions(-DDEBUG)

add_executable(TestObsController
  main.cpp
  ObsController.cpp
  ObsController.h
  Logs.h
)

include_directories(SYSTEM /opt/homebrew/Cellar/jsoncpp/1.9.6/include)

#link_directories(/usr/local/Cellar/jsoncpp/1.9.6/lib)
#target_link_libraries(TestObsController /usr/local/Cellar/jsoncpp/1.9.6/lib/libjsoncpp.dylib Qt::Core)
target_link_libraries(TestObsController /opt/homebrew/Cellar/jsoncpp/1.9.6/lib/libjsoncpp.dylib Qt::Core)

include_directories("/usr/local/include")

