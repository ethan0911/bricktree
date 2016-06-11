# -----------------------------------------------------
# raw to bricktree converter tool
# -----------------------------------------------------
ADD_EXECUTABLE(ospRaw2Bricks
  raw2bricks.cpp
  )
TARGET_LINK_LIBRARIES(ospRaw2Bricks
  ospray_bricktree
  )

# -----------------------------------------------------
# raw to slice-ppm converter tool
# -----------------------------------------------------
ADD_EXECUTABLE(ospRawSlice
  raw2slice.cpp
  )
TARGET_LINK_LIBRARIES(ospRawSlice
  ospray_bricktree
  )

