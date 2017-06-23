OSPRAY_CREATE_LIBRARY(ospray_module_bricktree
  BrickTree.ispc
  BrickTree.cpp
  Module.cpp
  # MSNearestNeighbor.ispc
  # MSFinestLevel.ispc

  LINK
  ospray_bricktree
  ospray
)
