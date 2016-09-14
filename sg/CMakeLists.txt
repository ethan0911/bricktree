# -----------------------------------------------------
# scene graph plugin
# -----------------------------------------------------
ADD_LIBRARY(ospray_sg_bricktree SHARED
  SGBrickTree.cpp
  )
OSPRAY_LIBRARY_LINK_LIBRARIES(ospray_sg_bricktree
  ospray_bricktree
  ospray_sg
  ${HDF5_LIBRARIES}
  )
