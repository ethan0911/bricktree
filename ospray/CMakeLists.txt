



OSPRAY_ADD_LIBRARY(ospray_module_amr SHARED
  ModuleInit.cpp
  
  ChomboVolume.cpp
  ChomboVolume.ispc

  initialFind.ispc
  CellRef.ispc
  CompOctant.ispc

  lerpOnFinestLevel.ispc
  octIgnoreBoundary.ispc
  octFromCoarse.ispc
  )

OSPRAY_LIBRARY_LINK_LIBRARIES(ospray_module_amr
  ospray_amr
  ospray
  )
