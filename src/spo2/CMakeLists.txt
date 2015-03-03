cmake_policy( SET CMP0028 NEW )

project( spo2 )

add_definitions(-DQWT_NO_OPENGL)

set( spo2_SOURCES 
  outputwidget.cpp
  outputwidget.hpp 
  document.cpp
  document.hpp	    
  main.cpp
  mainwindow.cpp
  mainwindow.hpp  
  masterobserver.hpp
  plot.cpp
  plot3.cpp  
  wheelbox.cpp
  knob.cpp
  signaldata.cpp
  curvedata.cpp
  samplingthread.cpp
  circularbuffer.cpp
  lcdbox.cpp
  )

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

file(GLOB UIs *.ui)
file(GLOB Resources *.qrc)
find_package( qwt )

message( "##### CMAKE_SOURCE_DIR = " ${CMAKE_SOURCE_DIR} )

include_directories(
  ${CMAKE_SOURCE_DIR}/src/libs
  ${QWT_INCLUDE_DIRS}
  ${Qt5Core_INCLUDE_DIRS}
  ${Qt5PrintSupport_INCLUDE_DIRS}
  )


find_package(Qt5 CONFIG REQUIRED Core Widgets Gui Svg PrintSupport )

find_package(Boost COMPONENTS program_options system )

if ( WIN32 )
  add_executable( spo2 WIN32 ${spo2_SOURCES} ${Resources} )
else()
  add_executable( spo2 ${spo2_SOURCES} ${Resources} )
endif()

target_link_libraries( spo2 LINK_PUBLIC
  adplot
  adportable
  adutils
  qt-manhattan-style
  ${QWT_LIBRARIES}
  Qt5::Widgets
  Qt5::Svg
  Qt5::PrintSupport
  ${Boost_LIBRARIES}
  )

install( TARGETS spo2 RUNTIME DESTINATION bin COMPONENT applications )

