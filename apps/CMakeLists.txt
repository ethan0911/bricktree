# -----------------------------------------------------
# raw to bricktree converter tool
# -----------------------------------------------------
ADD_EXECUTABLE(ospRaw2Bricks
  raw2bricks.cpp
  )
TARGET_LINK_LIBRARIES(ospRaw2Bricks
  ospray_bricktree
  )
