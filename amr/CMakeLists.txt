# -------------------------------------------------------
# data format loaders etc
# -------------------------------------------------------
OSPRAY_ADD_LIBRARY(ospray_amr SHARED
  Array3D.cpp
  FromArray3DBuilder.cpp
  AMR.cpp
  Octree.cpp
  Sumerian.cpp
  MultiSumBuilder.cpp

  BrickTree.cpp
  BrickTreeBuilder.cpp
  )

OSPRAY_LIBRARY_LINK_LIBRARIES(ospray_amr
  ospray
  )

