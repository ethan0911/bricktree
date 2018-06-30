# ======================================================================== ##
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

#CONFIGURE_OSPRAY()

# mini-scene graph viewer for _geometry_ (intentionally simple)
OPTION(OSPRAY_MODULE_BRICKTREE "BrickTree (testing)" OFF)
OPTION(OSPRAY_MODULE_BRICKTREE_BENCH
  "Build the benchmarker for bricktree volume" ON)
OPTION(OSPRAY_MODULE_BRICKTREE_WIDGET
  "Build the viewer for bricktree volume" ON)

IF (OSPRAY_MODULE_BRICKTREE)

  FIND_PACKAGE(HDF5 REQUIRED)
  IF (HDF5_FOUND)
    INCLUDE_DIRECTORIES(${HDF5_INCLUDE_DIRS})
    ADD_DEFINITIONS(${HDF5_DEFINITIONS})
  ENDIF()

  SET(OSPRAY_DEFAULT_COMPONENT mpi)
  OSPRAY_CONFIGURE_MPI()
  OSPRAY_BUILD_COMPONENT(mpiCommon)
  OSPRAY_BUILD_COMPONENT(mpiMessageLayer)
  
  INCLUDE_DIRECTORIES_ISPC(${EMBREE_INCLUDE_DIRS}/embree3)
  INCLUDE_DIRECTORIES(${EMBREE_INCLUDE_DIRS}/embree3)

  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})
  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)
  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/apps)
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

  # -------------------------------------------------------
  # bricktree kernel
  # -------------------------------------------------------
  ADD_SUBDIRECTORY(bt)

  # -------------------------------------------------------
  # ospray plugin
  # -------------------------------------------------------
  ADD_SUBDIRECTORY(ospray)
  
  IF(NOT THIS_IS_MIC)
    # -----------------------------------------------------
    # converter tools
    # -----------------------------------------------------
    ADD_SUBDIRECTORY(apps)
  ENDIF()

ENDIF()
