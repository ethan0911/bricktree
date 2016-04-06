
# -----------------------------------------------------
# raw to bricktree converter tool
# -----------------------------------------------------
ADD_EXECUTABLE(ospChombo2Raw
  chombo2raw.cpp
  )
TARGET_LINK_LIBRARIES(ospChombo2Raw
  ospray_sg_amr
  )

