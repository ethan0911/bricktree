# -------------------------------------------------------
# data format loaders etc
# -------------------------------------------------------
OSPRAY_ADD_LIBRARY(ospray_bricktree SHARED
  Array3D.cpp
  BrickTree.cpp
  BrickTreeBuilder.cpp
  )

OSPRAY_LIBRARY_LINK_LIBRARIES(ospray_bricktree
  ospray_common
  ospray
  )

