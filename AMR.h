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
#include "Array3D.h"

namespace ospray {
  namespace amr {

    template<typename voxel_t>
    struct AMR {
      
      struct Grid {
        struct Cell {
          //! ID of child grid, if >= 0; or "this is a leaf" flag if < 0
          int32 childID; 
          voxel_t lo, hi;
        };

        vec3i    cellDims;
        vec3i    voxelDims;
        size_t   numVoxels;
        size_t   numCells;
        Cell    *cell;
        voxel_t *voxel;
      };
      
      Grid *rootGrid;
      std::vector<Grid *> gridVec;

      void writeTo(const std::string &outFileName);
    };
    
  }
}
