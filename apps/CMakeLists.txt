ospray_disable_compiler_warnings()

find_package(OpenGL)
if (NOT OPENGL_FOUND) 
  set(OSPRAY_MODULE_BRICKTREE_WIDGET OFF)
endif ()

# -----------------------------------------------------
# raw to bricktree converter tool
# -----------------------------------------------------
OSPRAY_CREATE_APPLICATION(ospRaw2Bricks
  ospRaw2Bricks.cpp  
  LINK
  ospray_bricktree)

## ====================================================================== ##
## Benchmarker Widget
## ====================================================================== ##
if (OSPRAY_MODULE_BRICKTREE_WIDGET)
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
  # HDF5 
  find_package(HDF5 REQUIRED)
  # build
  include_directories(
    ${HDF5_INCLUDE_DIRS}
    opengl/external
    ${PROJECT_SOURCE_DIR}/apps/exampleViewer/common/imgui
    ${TFN_MODULE_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIR})
  ospray_create_application(ospBrickWidget
    ospBrickTreeApp.cpp
    ospBrickTreeTools.cpp
    opengl/viewer.cpp
    opengl/external/glad.c
    opengl/external/imgui_glfw_impi.cpp
    ${TFN_MODULE_SRC}
    LINK
    ospray
    ospray_common
    ospray_mpi_common
    ospray_imgui
    ospray_imgui3d_sg
    ${HDF5_C_LIBRARIES}
    ${HDF5_C_HL_LIBRARIES}
    ${OPENGL_LIBRARIES})
  set_target_properties(ospBrickWidget
    PROPERTIES 
    CXX_STANDARD 14
    COMPILE_DEFINITIONS USE_VIEWER=1)
endif (OSPRAY_MODULE_BRICKTREE_WIDGET)

## ==================================================================== ##
## Benchmarker
## ==================================================================== ##
if(OSPRAY_MODULE_BRICKTREE_BENCH)
  OSPRAY_CREATE_APPLICATION(ospBrickBench
    ospBrickTreeApp.cpp
    ospBrickTreeTools.cpp
    LINK
    ospray
    ospray_common
    ospray_mpi_common
    ospray_bricktree)
  set_target_properties(ospBrickBench
    PROPERTIES 
    CXX_STANDARD 14
    COMPILE_DEFINITIONS
    USE_VIEWER=0)
endif(OSPRAY_MODULE_BRICKTREE_BENCH)
