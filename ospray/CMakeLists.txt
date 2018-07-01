OSPRAY_CREATE_LIBRARY(ospray_module_bricktree 
  BrickTreeVolume.ispc
  BrickTreeVolume.cpp
  Module.cpp
  # MSNearestNeighbor.ispc
  # MSFinestLevel.ispc
  LINK
  ospray
  ospray_common
  ospray_module_ispc
  ospray_module_bricktree_core)
