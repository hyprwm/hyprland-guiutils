cmake_minimum_required(VERSION 3.12)

file(READ "${CMAKE_SOURCE_DIR}/VERSION" VER_RAW)
string(STRIP ${VER_RAW} VERSION)

project(
  hyprland-update-screen
  DESCRIPTION "A hyprland update screen"
  VERSION ${VERSION})

set(CMAKE_MESSAGE_LOG_LEVEL "STATUS")

file(GLOB_RECURSE SRCFILES "src/*.cpp")

add_executable(hyprland-update-screen ${SRCFILES})

string(REPLACE "\"" " " GIT_COMMIT_MESSAGE_ESCAPED "${GIT_COMMIT_MESSAGE}")
target_compile_definitions(hyprland-update-screen
                           PRIVATE "-DGIT_COMMIT_HASH=\"${GIT_COMMIT_HASH}\"")
target_compile_definitions(hyprland-update-screen PRIVATE "-DGIT_BRANCH=\"${GIT_BRANCH}\"")
target_compile_definitions(hyprland-update-screen
                           PRIVATE "-DGIT_COMMIT_MESSAGE=\"${GIT_COMMIT_MESSAGE_ESCAPED}\"")
target_compile_definitions(hyprland-update-screen PRIVATE "-DGIT_DIRTY=\"${GIT_DIRTY}\"")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

target_link_libraries(hyprland-update-screen PkgConfig::deps)

target_link_libraries(
  hyprland-update-screen
  pthread
  ${CMAKE_THREAD_LIBS_INIT})

include(GNUInstallDirs)

install(TARGETS hyprland-update-screen)
