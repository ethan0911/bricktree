# -----------------------------------------------------
# scene graph plugin
# -----------------------------------------------------
OSPRAY_CREATE_LIBRARY(ospray_sg_bricktree
  SGBrickTree.cpp

  LINK
  ospray_bricktree
  ospray_sg
  ${HDF5_LIBRARIES}
  )
