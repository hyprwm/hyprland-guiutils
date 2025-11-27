cmake_minimum_required(VERSION 3.12)

file(READ "${CMAKE_SOURCE_DIR}/VERSION" VER_RAW)
string(STRIP ${VER_RAW} VERSION)

project(
  hyprland-guiutils
  DESCRIPTION "Hyprland GUI utilities"
  VERSION ${VERSION})

set(CMAKE_MESSAGE_LOG_LEVEL "STATUS")

message(STATUS "Configuring hyprland-guiutils!")

# Get git info hash and branch
execute_process(
  COMMAND git rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_BRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
  COMMAND git rev-parse HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
  COMMAND bash -c "git show ${GIT_COMMIT_HASH} | head -n 5 | tail -n 1 | sed -s 's/\#//g; s/^[[:space:]]*//'"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_MESSAGE
  OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
  COMMAND bash -c "git diff-index --quiet HEAD -- || echo \"dirty\""
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_DIRTY
  OUTPUT_STRIP_TRAILING_WHITESPACE)

include_directories(.)
set(CMAKE_CXX_STANDARD 23)
add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-unused-value
                    -Wno-missing-field-initializers -Wno-narrowing)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

if(CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES DEBUG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg -no-pie -fno-builtin")
  set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} -pg -no-pie -fno-builtin")
  set(CMAKE_SHARED_LINKER_FLAGS
      "${CMAKE_SHARED_LINKER_FLAGS} -pg -no-pie -fno-builtin")
endif(CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES DEBUG)

pkg_check_modules(
  deps
  REQUIRED
  IMPORTED_TARGET
  hyprlang>=0.6.0
  hyprutils>=0.2.4
  hyprtoolkit>=0.4.0
  pixman-1
  libdrm)

add_subdirectory(utils/dialog)
add_subdirectory(utils/donate-screen)
add_subdirectory(utils/update-screen)
add_subdirectory(utils/welcome)
add_subdirectory(utils/run)
