OSPRAY_CREATE_LIBRARY(ospray_module_bricktree 
  BrickTreeVolume.ispc
  BrickTreeVolume.cpp
  Module.cpp
  # MSNearestNeighbor.ispc
  # MSFinestLevel.ispc
  LINK
  ospray
  ospray_common
  ospray_bricktree
  ospray_module_ispc)
