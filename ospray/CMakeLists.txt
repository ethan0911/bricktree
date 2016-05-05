OSPRAY_ADD_LIBRARY(ospray_module_amr SHARED
  MultiSum.ispc
  MultiSum.cpp

  MSNearestNeighbor.ispc
  MSCurrentLevel.ispc
  MSFinestLevel.ispc
  )

OSPRAY_LIBRARY_LINK_LIBRARIES(ospray_module_amr
  ospray_amr
  ospray
  )
