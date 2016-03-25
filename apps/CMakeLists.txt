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
