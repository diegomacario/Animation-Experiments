cmake_minimum_required(VERSION 3.11.1 FATAL_ERROR)

project(Animation-Experiments)

include_directories("${CMAKE_SOURCE_DIR}/inc"
                    "${CMAKE_SOURCE_DIR}/dependencies/win/inc/desktop_and_web")

set(project_headers
    inc/AnimatedMesh.h
    inc/Blending.h
    #inc/camera.h
    inc/Camera3.h
    inc/CCDSolver.h
    inc/Clip.h
    inc/CrossFadeControllerMultiple.h
    #inc/CrossFadeControllerQueue.h
    #inc/CrossFadeControllerSingle.h
    inc/CrossFadeTarget.h
    inc/FABRIKSolver.h
    inc/finite_state_machine.h
    inc/Frame.h
    inc/game.h
    inc/GLTFLoader.h
    inc/IKCrossFadeController.h
    inc/IKCrossFadeTarget.h
    inc/IKLeg.h
    inc/IKMovementState.h
    inc/IKState.h
    inc/Interpolation.h
    inc/Intersection.h
    inc/ModelViewerState.h
    inc/MovementState.h
    inc/Pose.h
    inc/quat.h
    inc/Ray.h
    inc/RearrangeBones.h
    inc/resource_manager.h
    inc/shader.h
    inc/shader_loader.h
    inc/Skeleton.h
    inc/SkeletonViewer.h
    inc/state.h
    inc/texture.h
    inc/texture_loader.h
    inc/Track.h
    inc/Transform.h
    inc/TransformTrack.h
    inc/Triangle.h
    inc/window.h)

set(project_sources
    src/AnimatedMesh.cpp
    src/Blending.cpp
    #src/camera.cpp
    src/Camera3.cpp
    src/CCDSolver.cpp
    src/Clip.cpp
    src/CrossFadeControllerMultiple.cpp
    #src/CrossFadeControllerQueue.cpp
    #src/CrossFadeControllerSingle.cpp
    src/CrossFadeTarget.cpp
    src/FABRIKSolver.cpp
    src/finite_state_machine.cpp
    src/game.cpp
    src/GLTFLoader.cpp
    src/IKCrossFadeController.cpp
    src/IKCrossFadeTarget.cpp
    src/IKLeg.cpp
    src/IKMovementState.cpp
    src/IKState.cpp
    src/Intersection.cpp
    src/main.cpp
    src/ModelViewerState.cpp
    src/MovementState.cpp
    src/Pose.cpp
    src/quat.cpp
    src/Ray.cpp
    src/RearrangeBones.cpp
    src/shader.cpp
    src/shader_loader.cpp
    src/Skeleton.cpp
    src/SkeletonViewer.cpp
    src/texture.cpp
    src/texture_loader.cpp
    src/Track.cpp
    src/Transform.cpp
    src/TransformTrack.cpp
    src/Triangle.cpp
    src/window.cpp
    dependencies/win/src/cgltf/cgltf.c
    dependencies/win/src/imgui/imgui.cpp
    dependencies/win/src/imgui/imgui_demo.cpp
    dependencies/win/src/imgui/imgui_draw.cpp
    dependencies/win/src/imgui/imgui_impl_glfw.cpp
    dependencies/win/src/imgui/imgui_impl_opengl3.cpp
    dependencies/win/src/imgui/imgui_widgets.cpp
    dependencies/win/src/stb_image/stb_image.cpp)

# This path must be relative to the location of the build folder
set(project_resources "../resources@resources")

set(CMAKE_EXECUTABLE_SUFFIX ".html")

set(CMAKE_CXX_FLAGS "-O3 -s USE_WEBGL2=1 -s FULL_ES3=1 -s USE_GLFW=3 -s WASM=1 -s ASSERTIONS=1 -s ALLOW_MEMORY_GROWTH=1 -o index.html --preload-file ${project_resources} --use-preload-plugins")

add_definitions(-DUSE_THIRD_PERSON_CAMERA)
add_executable(${PROJECT_NAME} ${project_headers} ${project_sources})
