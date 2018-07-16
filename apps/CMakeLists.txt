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
  ospray_module_bricktree_core)

## ====================================================================== ##
## Benchmarker Widget
## ====================================================================== ##
if (OSPRAY_MODULE_BRICKTREE_WIDGET)
  set(APP_VIEWER_PREFIX "ospray_module_bricktree")
  add_subdirectory(opengl)
  ospray_create_application(ospBrickWidget
    ospBrickTreeApp.cpp
    ospBrickTreeTools.cpp
    LINK
    ospray
    ospray_common
    ospray_mpi_common
    ospray_imgui
    ospray_imgui3d_sg
    ospray_module_ispc
    ospray_module_bricktree_core
    ${APP_VIEWER_PREFIX}_app_viewer
    ${HDF5_LIBRARIES})
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
    ospray_module_ispc
    ospray_module_bricktree_core)
  set_target_properties(ospBrickBench
    PROPERTIES 
    CXX_STANDARD 14
    COMPILE_DEFINITIONS USE_VIEWER=0)
endif(OSPRAY_MODULE_BRICKTREE_BENCH)
