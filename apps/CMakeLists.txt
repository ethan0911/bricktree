# -----------------------------------------------------
# raw-to-octree-amr converter: converts a structured
# volume (in RAW format)
# -----------------------------------------------------
ADD_EXECUTABLE(ospRaw2OctAMR
  raw2oamr
  )
TARGET_LINK_LIBRARIES(ospRaw2OctAMR
  ospray_amr
  )

# -----------------------------------------------------
# octree-amr resampling tool
# -----------------------------------------------------
ADD_EXECUTABLE(ospOAmrResample
  oamrResample.cpp
  )
TARGET_LINK_LIBRARIES(ospOAmrResample
  ospray_amr
  )

# -----------------------------------------------------
# raw to sumerian converter tool
# -----------------------------------------------------
ADD_EXECUTABLE(ospRaw2Sum
  raw2sum.cpp
  )
TARGET_LINK_LIBRARIES(ospRaw2Sum
  ospray_amr
  )

# -----------------------------------------------------
# raw to bricktree converter tool
# -----------------------------------------------------
ADD_EXECUTABLE(ospRaw2Bricks
  raw2bricks.cpp
  )
TARGET_LINK_LIBRARIES(ospRaw2Bricks
  ospray_amr
  )

# -----------------------------------------------------
# raw to bricktree converter tool
# -----------------------------------------------------
ADD_EXECUTABLE(ospChombo2Raw
  chombo2raw.cpp
  )
TARGET_LINK_LIBRARIES(ospChombo2Raw
  ospray_sg_amr
  )

# -----------------------------------------------------
# experimental chombo reader (hack)
# -----------------------------------------------------
option(OSPRAY_MODULE_AMR_CHOMBO "AMR Chombo file reader" OFF)
if (OSPRAY_MODULE_AMR_CHOMBO)
  set(CHOMBO_BOXTOOLS_LIBRARY "CHOMBO_BOXTOOLS_LIBRARY_NOTFOUND" CACHE FILEPATH "chombo boxtools library")
  set(CHOMBO_BASETOOLS_LIBRARY "CHOMBO_BASETOOLS_LIBRARY_NOTFOUND" CACHE FILEPATH "chombo basetools library")
  set(CHOMBO_AMRTOOLS_LIBRARY "CHOMBO_AMRTOOLS_LIBRARY_NOTFOUND" CACHE FILEPATH "chombo amr library")
  set(CHOMBO_INCLUDE "CHOMBO_INCLUDE_NOTFOUND" CACHE PATH "chombo include")

  if(${CHOMBO_INCLUDE} STREQUAL "CHOMBO_INCLUDE_NOTFOUND")
    message(ERROR "Please set CHOMBO_* to built 3D chombo library")
  endif()

  find_package(HDF5 REQUIRED)

  include_directories(${CHOMBO_INCLUDE} ${HDF5_INCLUDE_DIRS})

  add_executable(chombo2oamr chombo2oamr.cpp)
  target_link_libraries(chombo2oamr
    ${CHOMBO_BOXTOOLS_LIBRARY}
    ${CHOMBO_BASETOOLS_LIBRARY}
    ${CHOMBO_AMRTOOLS_LIBRARY}
	${HDF5_LIBRARIES})
endif()
