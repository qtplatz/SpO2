
project (spo2)

cmake_minimum_required (VERSION 2.8.11)
cmake_policy( SET CMP0020 NEW )

if ( NOT CMAKE_CROSSCOMPILING )
  set( CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/.. )
endif()

find_path( qtplatz_DIR NAMES qtplatz-config.cmake  HINTS
  /qtplatz/build  /qtplatz/release_build  /qtplatz )

if ( qtplatz_DIR-NOTFIND )
  message( FATAL_ERROR "QtPlatz build dir not found" )
endif()

include(${qtplatz_DIR}/qtplatz-config.cmake)

set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY         ${QTPLATZ_ARCHIVE_OUTPUT_DIRECTORY} ) # static libs, dll's import lib/exp
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY         ${QTPLATZ_LIBRARY_OUTPUT_DIRECTORY} ) # plugins
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG   ${QTPLATZ_LIBRARY_OUTPUT_DIRECTORY} ) # plugins
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${QTPLATZ_LIBRARY_OUTPUT_DIRECTORY} ) # plugins
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY         ${QTPLATZ_RUNTIME_OUTPUT_DIRECTORY} )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${QTPLATZ_RUNTIME_OUTPUT_DIRECTORY} )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${QTPLATZ_RUNTIME_OUTPUT_DIRECTORY} )

set( CMAKE_POSITION_INDEPENDENT_CODE ON )

if ( WIN32 )
  configure_file( ${CMAKE_SOURCE_DIR}/makefile.in ${CMAKE_BINARY_DIR}/makefile )
elseif ( ${CMAKE_SYSTEM} MATCHES "Linux" )
  configure_file( ${CMAKE_SOURCE_DIR}/postinst.in ${CMAKE_BINARY_DIR}/postinst @ONLY ) 
  configure_file( ${CMAKE_SOURCE_DIR}/qt.conf.in ${CMAKE_BINARY_DIR}/qt.conf @ONLY ) 
endif()

find_package( Boost 1.57 REQUIRED )

add_definitions(-DUNICODE -D_UNICODE)

if (WIN32)

  set(CMAKE_INSTALL_PREFIX C:/SpO2 )
  add_definitions(-D_SCL_SECURE_NO_WARNINGS)

elseif ( ${CMAKE_SYSTEM_NAME} MATCHES "Linux" )  

  if ( CMAKE_CROSSCOMPILING )
    set( rootfs /mnt/raspi-rootfs )
  endif()

  set( CMAKE_INSTALL_PREFIX ${rootfs}/opt/raspi/SpO2 ) 
  set( QTDIR ${rootfs}/opt/raspi/qt5pi )
  
else()

  set(CMAKE_INSTALL_PREFIX /opt/local/SpO2 )  

endif()

set(CMAKE_DEBUG_POSTFIX d)

set(QT_MANHATTAN_STYLE_DIR ${CMAKE_SOURCE_DIR}/qt-manhattan-style)

SET(CMAKE_INSTALL_NAME_DIR ./bin )

include_directories(${Boost_INCLUDE_DIRS} ${QTPLATZ_SOURCE_DIR}/src/libs ${QT_MANHATTAN_STYLE_DIR} )
link_directories(${Boost_LIBRARY_DIRS} ${CMAKE_BINARY_DIR}/lib)

add_subdirectory ( src )
add_subdirectory ( ${QT_MANHATTAN_STYLE_DIR} ${CMAKE_BINARY_DIR}/qt-manhattan-style )

FILE( GLOB QTPLATZ_LIBRARIES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${QTPLATZ_BINARY_DIR}/bin/*.dll )
foreach( libname ${QTPLATZ_LIBRARIES} )
  string( REGEX MATCH "^.+d\\.dll$" output ${libname} )
  if ( NOT output )
    list( APPEND DEPENDENT_LIBRARIES ${libname} )
  endif()
endforeach()

FILE( GLOB QT_LIBRARIES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} $ENV{QTDIR}/bin/*.dll )
foreach( libname ${QT_LIBRARIES} )
  string( REGEX MATCH "^.+d\\.dll$" output ${libname} )
  if ( NOT output )
    list( APPEND DEPENDENT_LIBRARIES ${libname} )
  endif()
endforeach()

FILE( GLOB MSVC_LIBRARIES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} $ENV{VSINSTALLDIR}/redist/x64/Microsoft.VC120.CRT/*.dll )
foreach( libname ${MSVC_LIBRARIES} )
  list( APPEND DEPENDENT_LIBRARIES ${libname} )
endforeach()

install( FILES ${DEPENDENT_LIBRARIES} DESTINATION bin COMPONENT applications )

######## CPack
#
execute_process( COMMAND git describe
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  RESULT_VARIABLE git_result
  OUTPUT_VARIABLE git_describe
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )

set( _major 0 )
set( _minor 0 )
set( _patch 0 )

string( REGEX MATCHALL "[0-9]+|[\\.\\-][0-9]+" _list ${git_describe} )
list( LENGTH _list _count )
if ( _count GREATER 1 ) # count >= 2
  string( REGEX REPLACE "v([0-9]+)\\.[0-9]+.*$" "\\1" _major ${git_describe} )
  string( REGEX REPLACE "v[0-9]+\\.([0-9]+).*$" "\\1" _minor ${git_describe} )
endif()
if ( _count GREATER 2 ) # count >= 3
  string( REGEX REPLACE "v[0-9]+\\.[0-9]+[\\.-]([0-9]+).*$" "\\1" _micro ${git_describe} )  
endif()
if ( _count GREATER 3 ) # count >= 4
  string( REGEX REPLACE "v[0-9]+\\.[0-9]+[\\.-][0-9]+-([0-9]+)-.*$" "\\1" _patch ${git_describe} )
  string( REGEX REPLACE "v[0-9]+\\.[0-9]+[\\.-][0-9]+-[0-9]+-(.*)$" "\\1" _hash ${git_describe} )    
endif()

include( CPackComponent )

if ( WIN32 )
  set( CPACK_GENERATOR WIX )
  set( CPACK_WIX_UPGRADE_GUID 2F5649B0-CAA6-4941-96E8-F8AE7C709692 )
  
endif()

if ( ${CMAKE_SYSTEM_NAME} MATCHES "Linux" )

  set( CPACK_GENERATOR DEB )
  set( CPACK_DEBIAN_PACKAGE_MAINTAINER "T. Hondo <toshi.hondo@qtplatz.com>" )
  set( CPACK_DEBIAN_PACKAGE_VERSION "${VERSION}" )
  set( CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_BINARY_DIR}/postinst" )
  if ( RTC_ARCH_ARM )
    set( CPACK_DEBIAN_PACKAGE_ARCHITECTURE armhf )
  endif()
  
endif()

set( CPACK_PACKAGE_NAME "spo2" )
set( CPACK_PACKAGE_VENDOR "ms-cheminfo.com" )
set( CPACK_PACKAGE_VERSION ${_major}.${_minor}.${_micro}.${_patch} )
set( CPACK_PACKAGE_VERSION_MAJOR ${version_major} )
set( CPACK_PACKAGE_VERSION_MINOR ${version_minor} )
set( CPACK_PACKAGE_VERSION_PATCH ${version_micro} )
set( CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/license.txt )

set( CPACK_PACKAGE_EXECUTABLES spo2 "SpO2" )
set( CPACK_CREATE_DESKTOP_LINKS spo2 )

if(WIN32)

  set( CPACK_INSTALL_PREFIX "C:/${CMAKE_PROJECT_NAME}" )
  install( FILES
    $ENV{VCINSTALLDIR}/redist/x64/Microsoft.VC120.CRT/msvcp120.dll
    $ENV{VCINSTALLDIR}/redist/x64/Microsoft.VC120.CRT/msvcr120.dll
    $ENV{VCINSTALLDIR}/redist/x64/Microsoft.VC120.CRT/vccorlib120.dll
    DESTINATION bin )
  string( REGEX REPLACE "\\\\" "/" QTDIR $ENV{QTDIR} )
  
else()
  
  set( CPACK_SET_DESTDIR ON )
  set( CPACK_INSTALL_PREFIX "/opt/${CMAKE_PROJECT_NAME}" )

endif()

if ( CMAKE_CROSSCOMPILING )
  # whole qtplatz libraries to be installed to lib, which actually lib/qtplatz
  install( DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} DESTINATION lib )
  install( FILES ${CMAKE_BINARY_DIR}/qt.conf DESTINATION bin )
else()
  install( FILES ${QTDIR}/bin/qt.conf DESTINATION bin )
endif()
            
include( CPack )
