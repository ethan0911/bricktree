// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

// ospray
#include "Octree.h"

namespace ospray {
  namespace amr {

    /*! out multi-octree data structure - one root grid with one octree per cell */
    template<typename voxel_t>
    struct AMR {
      
      typedef typename Octree<voxel_t>::Cell Cell;
      typedef typename Octree<voxel_t>::OctCell OctCell;
      
      AMR() : dimensions(0) {}
      
      void allocate(const vec3i &newDims) { dimensions = newDims; rootCell.resize(numCells()); }
      size_t numCells() const { return dimensions.x*dimensions.y*dimensions.z; }
      
      // 3D array of root nodes
      std::vector<Cell> rootCell;
      // array of (all) child cells, across all octrees
      std::vector<OctCell> octCell;
      // dimensions of root grid
      vec3i dimensions;
      
      void writeTo(const std::string &outFileName);
    };
    
  }
}
