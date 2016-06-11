OSPRAY_ADD_LIBRARY(ospray_module_bricktree SHARED
#  BrickTree.ispc
  BrickTree.cpp

#  MSNearestNeighbor.ispc
#  MSFinestLevel.ispc
  )

OSPRAY_LIBRARY_LINK_LIBRARIES(ospray_module_bricktree
  ospray_bricktree
  ospray
  )
