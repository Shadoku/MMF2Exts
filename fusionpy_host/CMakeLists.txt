cmake_minimum_required(VERSION 3.20)
project(fusionpy_host LANGUAGES CXX VERSION 0.1.0)

option(FUSIONPY_BUILD_XP "Enable Windows XP compatible toolset" OFF)

if (MSVC)
  if (FUSIONPY_BUILD_XP)
    add_definitions(-DWINVER=0x0501 -D_WIN32_WINNT=0x0501)
    set(CMAKE_GENERATOR_TOOLSET "v142_xp" CACHE STRING "" FORCE)
  else()
    add_definitions(-DWINVER=0x0600 -D_WIN32_WINNT=0x0600)
  endif()
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(GNUInstallDirs)

set(HOST_SOURCES
    src/RuntimeBridge.cpp
    src/HostEntrypoint.cpp
    src/Injector.cpp
)

add_library(fusionpy_host SHARED ${HOST_SOURCES})
target_include_directories(fusionpy_host PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(fusionpy_host PRIVATE FUSIONPY_HOST_VERSION=\"${PROJECT_VERSION}\")

add_executable(fusionpy_injector src/InjectorMain.cpp)
target_link_libraries(fusionpy_injector PRIVATE fusionpy_host)

set_target_properties(fusionpy_host PROPERTIES
    OUTPUT_NAME "fusionpy_host"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

set_target_properties(fusionpy_injector PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

install(TARGETS fusionpy_host fusionpy_injector
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

include(CPack)
set(CPACK_PACKAGE_NAME "fusionpy_host")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR "ZIP")
set(CPACK_OUTPUT_FILE_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/releases)
include(CPack)
