cmake_minimum_required(VERSION 3.10.0)

project(build_and_battle)

set(TOOLKIT_VERSION "1.0.0")
set(TOOLKIT_BASE_NAME "BBToolkit")
set(GAME_VERSION "1.0.0")
set(GAME_BASENAME "Build_and_Battle")

set(GAME_TARGET_NAME "${GAME_BASENAME}-v${GAME_VERSION}")
set(TOOLKIT_TARGET_NAME "${TOOLKIT_BASE_NAME}-v${TOOLKIT_VERSION}")
set(PLIST_FILE ${CMAKE_SOURCE_DIR}/cmake/Info.plist)


set(BUILD_AND_BATTLE_SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(URHO3D_DIR ${CMAKE_SOURCE_DIR}/deps/Urho3D/build/${CMAKE_BUILD_TYPE})
set(URHO3D_INCLUDE_DIR ${URHO3D_DIR}/include)
set(THIRD_PARTY_INCLUDE_DIR ${URHO3D_INCLUDE_DIR}/Urho3D/ThirdParty)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set the src files for the project
file(GLOB GAME_SRC_FILES
  "${BUILD_AND_BATTLE_SRC_DIR}/*.cpp"
  "${BUILD_AND_BATTLE_SRC_DIR}/game/*.cpp"
  )

  # Set the src files for the project
file(GLOB TOOLKIT_SRC_FILES 
"${BUILD_AND_BATTLE_SRC_DIR}/*.cpp"
"${BUILD_AND_BATTLE_SRC_DIR}/toolkit/*.cpp"
)

# Set project includes dir
include_directories(
  "${BUILD_AND_BATTLE_SRC_DIR}"
  "${URHO3D_INCLUDE_DIR}"
  "${THIRD_PARTY_INCLUDE_DIR}"
  )

find_library(URHO3D_LIB
  NAMES Urho3D libUrho3D
  HINTS "${URHO3D_DIR}/lib"
)

add_definitions(-DURHO3D_ANGELSCRIPT)
add_definitions(-DURHO3D_LUA)
add_definitions(-DURHO3D_NETWORK)
add_definitions(-DURHO3D_PHYSICS)

if(WIN32)
elseif(APPLE)
  add_executable(${GAME_TARGET_NAME} MACOSX_BUNDLE ${GAME_SRC_FILES})
  add_executable(${TOOLKIT_TARGET_NAME} MACOSX_BUNDLE ${TOOLKIT_SRC_FILES})
  target_compile_features(${GAME_TARGET_NAME} PUBLIC cxx_std_14)
  target_compile_features(${TOOLKIT_TARGET_NAME} PUBLIC cxx_std_14)
  target_compile_definitions(${GAME_TARGET_NAME} PUBLIC $<$<CONFIG:Debug>:DEBUG_VERSION> $<$<CONFIG:Debug>:URHO3D_LOGGING> $<$<CONFIG:Release>:RELEASE_VERSION> BBGAME_CURRENT_VERSION=${GAME_VERSION})
  target_compile_definitions(${TOOLKIT_TARGET_NAME} PUBLIC $<$<CONFIG:Debug>:DEBUG_VERSION> $<$<CONFIG:Debug>:URHO3D_LOGGING> $<$<CONFIG:Release>:RELEASE_VERSION> BBTOOLKIT_CURRENT_VERSION=${TOOLKIT_VERSION})
  set_target_properties(${GAME_TARGET_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${PLIST_FILE})
  set_target_properties(${GAME_TARGET_NAME} PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "${GAME_BASENAME}")
  set_target_properties(${GAME_TARGET_NAME} PROPERTIES MACOSX_BUNDLE_BUNDLE_GUI_IDENTIFIER "com.${GAME_BASENAME}")
  set_target_properties(${TOOLKIT_TARGET_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${PLIST_FILE})
  set_target_properties(${TOOLKIT_TARGET_NAME} PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "${TOOLKIT_BASE_NAME}")
  set_target_properties(${TOOLKIT_TARGET_NAME} PROPERTIES MACOSX_BUNDLE_BUNDLE_GUI_IDENTIFIER "com.${TOOLKIT_BASE_NAME}")
else()
  add_executable(${GAME_TARGET_NAME} ${GAME_SRC_FILES})
  add_executable(${TOOLKIT_TARGET_NAME} ${TOOLKIT_SRC_FILES})
endif()

# copy all the files in the deploy folder to bin
add_custom_target(deploy_files ALL
  COMMAND cmake -E copy_directory ${CMAKE_SOURCE_DIR}/deploy ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
  DEPENDS ${GAME_TARGET_NAME}
  )

# link with opengl (no debug version)
# find_package(OpenGL REQUIRED)
# if(NOT OPENGL_FOUND)
#   message(ERROR "Error: OpenGL not found on your system - please make sure you have it downloaded.")
# endif(NOT OPENGL_FOUND)

# Link in the unix only stuff
if (UNIX)
  target_link_libraries(${GAME_TARGET_NAME} pthread)
  target_link_libraries(${TOOLKIT_TARGET_NAME} pthread)
endif()

# link in the libraries for Urho3d
target_link_libraries(${GAME_TARGET_NAME}
  ${URHO3D_LIB}
# ${OPENGL_LIBRARIES}
)

# link in the libraries for Urho3d
target_link_libraries(${TOOLKIT_TARGET_NAME}
  ${URHO3D_LIB}
# ${OPENGL_LIBRARIES}
)
