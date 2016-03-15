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
OPTION(OSPRAY_MODULE_AMR_VOLUME "AMR Volume Module (testing)" ON)

IF (OSPRAY_MODULE_AMR_VOLUME)
  INCLUDE_DIRECTORIES(${EMBREE_INCLUDE_DIRS})
  INCLUDE_DIRECTORIES_ISPC(${EMBREE_INCLUDE_DIRS})

  SET(OSPRAY_SG_SOURCE_DIR ${PROJECT_SOURCE_DIR}/apps/qtViewer)

  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)
  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/)
  INCLUDE_DIRECTORIES(${OSPRAY_SG_SOURCE_DIR}/)
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

  # -------------------------------------------------------
  # data format loaders etc
  # -------------------------------------------------------
  OSPRAY_ADD_LIBRARY(ospray_amr SHARED
    amr/Array3D.cpp
    amr/FromArray3DBuilder.cpp
    amr/AMR.cpp
    amr/Octree.cpp
    )
  
  OSPRAY_LIBRARY_LINK_LIBRARIES(ospray_amr
    ospray
    )
  
  # -------------------------------------------------------
  # ospray plugin
  # -------------------------------------------------------
  OSPRAY_ADD_LIBRARY(ospray_module_amr SHARED
    ospray/AMRVolume.ispc
    ospray/AMRVolume.cpp
    )
  
  OSPRAY_LIBRARY_LINK_LIBRARIES(ospray_module_amr
    ospray_amr
    ospray
    )
  
  IF(NOT THIS_IS_MIC)
    # -----------------------------------------------------
    # scene graph plugin
    # -----------------------------------------------------
    ADD_LIBRARY(ospray_sg_amr SHARED
      sg/AMRVolume.cpp
      )
    OSPRAY_LIBRARY_LINK_LIBRARIES(ospray_sg_amr
      ospray_amr
      ospray_sg
      )
    
    # -----------------------------------------------------
		# raw-to-octree-amr converter: converts a structured
    # volume (in RAW format)
    # -----------------------------------------------------
    ADD_EXECUTABLE(ospRaw2OctAMR
      apps/raw2oamr
			)
    TARGET_LINK_LIBRARIES(ospRaw2OctAMR
      ospray_amr
			)

    # -----------------------------------------------------
		# octree-amr resampling tool
    # -----------------------------------------------------
    ADD_EXECUTABLE(ospOAmrResample
      apps/oamrResample.cpp
			)
		TARGET_LINK_LIBRARIES(ospOAmrResample
#		OSPRAY_EXE_LINK_LIBRARIES(ospOAmrResample
			ospray_amr
			)
  ENDIF()
ENDIF()
