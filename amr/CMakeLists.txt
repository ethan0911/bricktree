# -------------------------------------------------------
# data format loaders etc
# -------------------------------------------------------
OSPRAY_ADD_LIBRARY(ospray_amr SHARED
  Array3D.cpp
  )

OSPRAY_LIBRARY_LINK_LIBRARIES(ospray_amr
  ospray
  )

