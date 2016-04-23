



OSPRAY_ADD_LIBRARY(ospray_module_amr SHARED
  ModuleInit.cpp
  
  ChomboVolume.cpp
  ChomboVolume.ispc

  initialFind.ispc
  CellRef.ispc
  CompOctant.ispc

  lerpOnFinestLevel.ispc
  blendFromCoarser.ispc

  octIgnoreBoundary.ispc
  octFromCoarseLevel.ispc
  octFromCoarseCorners.ispc
  )

OSPRAY_LIBRARY_LINK_LIBRARIES(ospray_module_amr
  ospray_amr
  ospray
  )
