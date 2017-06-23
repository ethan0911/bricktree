# -----------------------------------------------------
# raw to bricktree converter tool
# -----------------------------------------------------
OSPRAY_CREATE_APPLICATION(ospRaw2Bricks
  raw2bricks.cpp
  
  LINK
  ospray_bricktree
  )
