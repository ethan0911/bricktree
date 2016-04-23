



OSPRAY_ADD_LIBRARY(ospray_module_amr SHARED
  ModuleInit.cpp

  # the input data type - list of chombo bricks
  ChomboData.cpp
  # acceleration structure - kdtree over input 
  ChomboAccel.cpp
  # the actual volume object
  ChomboVolume.cpp
  ChomboVolume.ispc

  CellRef.ispc
  Octant.ispc

  lerpOnFinestLevel.ispc
  blendFromCoarser.ispc

  octIgnoreBoundary.ispc
  # general oct-from-coarse method
  octFromCoarseGeneral.ispc
  octFromCoarseLevel.ispc
  )

OSPRAY_LIBRARY_LINK_LIBRARIES(ospray_module_amr
  ospray_amr
  ospray
  )
