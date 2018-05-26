ospray_disable_compiler_warnings()

find_package(OpenGL)
if (NOT OPENGL_FOUND) 
  set(OSPRAY_MODULE_BRICKTREE_WIDGET OFF)
endif ()

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
    ${TFN_MODULE_ROOT}/src/common/jsoncpp.cpp
    ${TFN_MODULE_ROOT}/src/widgets/TransferFunctionWidget.cpp)
endif (TFN_MODULE)

include_directories(
  external
  ${PROJECT_SOURCE_DIR}/apps/exampleViewer/common/imgui
  ${TFN_MODULE_INCLUDE_DIR}
  ${OPENGL_INCLUDE_DIR})
ospray_create_library(ospray_app_viewer
  scenegraph/properties.cpp
  viewer.cpp
  engine.cpp
  widgets.cpp
  external/glad.c
  external/imgui_glfw_impi.cpp
  ${TFN_MODULE_SRC}
  LINK
  ospray
  ospray_common
  ospray_imgui
  ospray_imgui3d_sg
  ${OPENGL_LIBRARIES})
set_target_properties(ospray_app_viewer
  PROPERTIES CXX_STANDARD 14)
