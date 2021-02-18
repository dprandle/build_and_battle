cmake_minimum_required(VERSION 3.10.0)

project(build_and_battle)

set(TARGET_NAME build_and_battle)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(PLIST_FILE ${CMAKE_SOURCE_DIR}/cmake/Info.plist)


set(BUILD_AND_BATTLE_SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(BUILD_AND_BATTLE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(URHO3D_DIR ${CMAKE_SOURCE_DIR}/deps/Urho3D/build/Release)#${CMAKE_BUILD_TYPE})
set(URHO3D_INCLUDE_DIR ${URHO3D_DIR}/include)
set(THIRD_PARTY_INCLUDE_DIR ${URHO3D_INCLUDE_DIR}/Urho3D/ThirdParty)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set the src files for the project
file(GLOB SRC_FILES 
  "${BUILD_AND_BATTLE_SRC_DIR}/*.cpp"
  )

# Set project includes dir
include_directories(
  "${BUILD_AND_BATTLE_INCLUDE_DIR}"
  "${URHO3D_INCLUDE_DIR}"
  "${THIRD_PARTY_INCLUDE_DIR}"
  )

find_library(URHO3D_LIB
  NAMES Urho3D libUrho3D
  HINTS "${URHO3D_DIR}/lib"
)

if (${CMAKE_BUILD_TYPE} STREQUAL Debug)
  add_definitions(-DDEBUG_VERSION -DURHO3D_LOGGING)
else()
  add_definitions(-DRELEASE_VERSION)
endif()

add_definitions(-DURHO3D_ANGELSCRIPT)
add_definitions(-DURHO3D_LUA)
add_definitions(-DURHO3D_NETWORK)
add_definitions(-DURHO3D_PHYSICS)


if(WIN32)
elseif(APPLE)
  add_executable(${TARGET_NAME} MACOSX_BUNDLE ${SRC_FILES})
  set_target_properties(${THIS_TARGET_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${PLIST_FILE})
  set_target_properties(${THIS_TARGET_NAME} PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "${TARGET_NAME}")
  set_target_properties(${THIS_TARGET_NAME} PROPERTIES MACOSX_BUNDLE_BUNDLE_GUI_IDENTIFIER "com.${TARGET_NAME}")
else()
  add_executable(${TARGET_NAME} ${SRC_FILES})
endif()

# copy all the files in the deploy folder to bin
add_custom_target(deploy_files ALL
  COMMAND cmake -E copy_directory ${CMAKE_SOURCE_DIR}/deploy ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/build_and_battle.app/Contents/MacOS
  DEPENDS ${TARGET_NAME}
  )

# link with opengl (no debug version)
# find_package(OpenGL REQUIRED)
# if(NOT OPENGL_FOUND)
#   message(ERROR "Error: OpenGL not found on your system - please make sure you have it downloaded.")
# endif(NOT OPENGL_FOUND)

# Link in the unix only stuff
if (UNIX)
  target_link_libraries(${TARGET_NAME} pthread)
endif()

# link in the libraries for Urho3d
target_link_libraries(${TARGET_NAME}
  ${URHO3D_LIB}
# ${OPENGL_LIBRARIES}
)