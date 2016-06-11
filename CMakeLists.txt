## ======================================================================== ##
## Copyright 2009-2015 Intel Corporation                                    ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

CONFIGURE_OSPRAY()

# mini-scene graph viewer for _geometry_ (intentionally simple)
OPTION(OSPRAY_MODULE_BRICKTREE "BrickTree (testing)" ON)

IF (OSPRAY_MODULE_BRICKTREE)
  INCLUDE_DIRECTORIES(${EMBREE_INCLUDE_DIRS})
  INCLUDE_DIRECTORIES_ISPC(${EMBREE_INCLUDE_DIRS})

  SET(OSPRAY_SG_SOURCE_DIR ${PROJECT_SOURCE_DIR}/apps/qtViewer)

  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)
  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/)
  INCLUDE_DIRECTORIES(${OSPRAY_SG_SOURCE_DIR}/)
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

  ADD_SUBDIRECTORY(amr)

  # -------------------------------------------------------
  # ospray plugin
  # -------------------------------------------------------
  ADD_SUBDIRECTORY(ospray)
  
  IF(NOT THIS_IS_MIC)
    # -----------------------------------------------------
    # scene graph plugin
    # -----------------------------------------------------
    ADD_LIBRARY(ospray_sg_bricktree SHARED
      sg/SGBrickTree.cpp
      )
    OSPRAY_LIBRARY_LINK_LIBRARIES(ospray_sg_bricktree
      ospray_bricktree
      ospray_sg
      hdf5
      )
    ADD_SUBDIRECTORY(apps)
  ENDIF()

ENDIF()
