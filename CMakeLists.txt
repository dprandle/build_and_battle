cmake_minimum_required(VERSION 2.8.11)

project(build_and_battle)

set(EXEC_NAME build_and_battle)

# Set the source directories
set(BUILD_AND_BATTLE_SRC_DIR ${CMAKE_SOURCE_DIR}/src)

# Set the include directory
set(BUILD_AND_BATTLE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(URHO3D_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/deps/Urho3D/build/${CMAKE_BUILD_TYPE}/include)
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

if (${CMAKE_BUILD_TYPE} STREQUAL Debug)
  add_definitions(-DDEBUG_VERSION)
else()
  add_definitions(-DRELEASE_VERSION)
endif()

if (UNIX)
  add_definitions(-std=c++11)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0")
elseif (WIN32)
  set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} /WX /NODEFAULTLIB:MSVCRT /NODEFAULTLIB:LIBCMT" )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
endif()

add_executable(${EXEC_NAME} ${SRC_FILES})

# copy all the files in the deploy folder to bin
add_custom_target(deploy_files ALL
  COMMAND cmake -E copy_directory ${CMAKE_SOURCE_DIR}/deploy ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
  DEPENDS ${TARGET_NAME}
  )

  # Create a shortcut for the compile commands
add_custom_target(move_compile_commands ALL
  COMMAND cmake -E create_symlink ${CMAKE_BINARY_DIR}/compile_commands.json ${CMAKE_SOURCE_DIR}/compile_commands.json
  DEPENDS ${TARGET_NAME}
)

find_library(URHO3D Urho3D PATHS ${CMAKE_SOURCE_DIR}/deps/Urho3D/build/${CMAKE_BUILD_TYPE}/lib)

# link with opengl (no debug version)
find_package(OpenGL REQUIRED)
if(NOT OPENGL_FOUND)
  message(ERROR "Error: OpenGL not found on your system - please make sure you have it downloaded.")
endif(NOT OPENGL_FOUND)

# Link in the unix only stuff
if (UNIX)
  target_link_libraries(${EXEC_NAME} pthread)
endif()

# link in the libraries for Urho3d
target_link_libraries(${EXEC_NAME}
  ${URHO3D}
  ${OPENGL_LIBRARIES}
  )