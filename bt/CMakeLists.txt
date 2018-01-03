# -------------------------------------------------------
# data format loaders etc
# -------------------------------------------------------
OSPRAY_CREATE_LIBRARY(ospray_bricktree 
  BrickTree.cpp
  BrickTreeBuilder.cpp

  LINK
  
  ospray_common
  ospray_xml
  ospray
  )


