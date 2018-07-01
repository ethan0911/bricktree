# -------------------------------------------------------
# data format loaders etc
# -------------------------------------------------------
OSPRAY_CREATE_LIBRARY(ospray_module_bricktree_core
  BrickTree.cpp
  BrickTreeBuilder.cpp
  LINK
  ospray_common
  ospray  
  )


