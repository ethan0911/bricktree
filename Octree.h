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

    /* octree of cell-centered nodes */
    template<typename voxel_t>
    struct Octree {
      struct Cell {
        float ccValue; //!< cell center value
        int32 childID; // -1 means 'no child', else this points into octCell[] array
      };
      struct OctCell {
        Cell child[2][2][2];
      };
      std::vector<Cell> rootCell;
      std::vector<OctCell> octCell;
      
      void writeTo(FILE *bin);
    };

  } // ::ospray::amr
} // ::ospray
