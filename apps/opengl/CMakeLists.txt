# define package name
if (NOT APP_VIEWER_PREFIX)
  set(APP_VIEWER_PREFIX ospray)
endif ()

# find packages
include(packages.cmake)

# transfer function widget
set(TFN_MODULE_DEFAULT_ROOT 
  ${PROJECT_SOURCE_DIR}/apps/exampleViewer/widgets/TransferFunctionModule) 
if(TFN_MODULE_ROOT)
  set(TFN_MODULE 1)
elseif (EXISTS ${TFN_MODULE_DEFAULT_ROOT})
  set(TFN_MODULE_ROOT ${TFN_MODULE_DEFAULT_ROOT})
  set(TFN_MODULE 1)
else ()
  set(TFN_MODULE 0)
endif ()
if (TFN_MODULE)
  set(TFN_MODULE_INCLUDE_DIR ${TFN_MODULE_ROOT}/src)
  set(TFN_MODULE_SRC 
    ${TFN_MODULE_ROOT}/src/TransferFunctionModule.cpp
    ${TFN_MODULE_ROOT}/src/widgets/TransferFunctionWidget.cpp)
endif (TFN_MODULE)

# build everything as a library
add_library(${APP_VIEWER_PREFIX}_app_viewer
  ${TFN_MODULE_SRC}
  external/imgui_glfw_impi.cpp
  src/common/trackball.cpp
  src/scene/properties.cpp
  src/camera.cpp
  src/engine.cpp
  viewer.cpp)
set_target_properties(${APP_VIEWER_PREFIX}_app_viewer
  PROPERTIES CXX_STANDARD 11)
target_include_directories(${APP_VIEWER_PREFIX}_app_viewer
  PRIVATE
  "$<BUILD_INTERFACE:"
  "${TFN_MODULE_INCLUDE_DIR};"
  "${CMAKE_CURRENT_LIST_DIR}/src;"
  "${CMAKE_CURRENT_LIST_DIR}/external;"
  ">")
target_link_libraries(${APP_VIEWER_PREFIX}_app_viewer
  glad glfw OpenGL)
if (TARGET ospray_package) # if the viewer is built outside ospray source
  target_link_libraries(${APP_VIEWER_PREFIX}_app_viewer
    imgui ospray_package)
else () # if the package is built within an ospray module
  target_include_directories(${APP_VIEWER_PREFIX}_app_viewer
    PUBLIC "$<BUILD_INTERFACE:"
    "${PROJECT_SOURCE_DIR}/apps/exampleViewer/common/imgui;>")
  target_link_libraries(${APP_VIEWER_PREFIX}_app_viewer
    ospray
    ospray_common
    ospray_imgui
    ospray_imgui3d_sg)
endif ()
