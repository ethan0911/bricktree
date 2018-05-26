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
  add_subdirectory(opengl)
  # HDF5 
  find_package(HDF5 REQUIRED)
  include_directories(${HDF5_INCLUDE_DIRS})  
  # build
  ospray_create_application(ospBrickWidget
    ospBrickTreeApp.cpp
    ospBrickTreeTools.cpp
    LINK
    ospray
    ospray_common
    ospray_mpi_common
    ospray_imgui
    ospray_imgui3d_sg
    ospray_app_viewer
    ${HDF5_C_LIBRARIES}
    ${HDF5_C_HL_LIBRARIES})
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
