
ADD_LIBRARY(ospray_sg_amr SHARED
  AMRVolume.cpp
  ChomboReader.cpp
  ChomboVolume.cpp
  )
OSPRAY_LIBRARY_LINK_LIBRARIES(ospray_sg_amr
  ospray_amr
  ospray_sg
  hdf5
  )

