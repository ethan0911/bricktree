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
// std
#include <iostream>

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
        // the 8 children of this node. note those are to be accessed
        // as child[iz][iy][ix], not the other way around!
        Cell child[2][2][2];
      };
    };

    template<typename T>
    inline std::ostream &operator<<(std::ostream &o, 
                                    const typename ospray::amr::Octree<T>::Cell &c)
    { o << "("<<c.ccValue<<":"<<c.childID<<")"; return o; }

    template<typename T>
    inline std::ostream &operator<<(std::ostream &o, 
                                    const typename ospray::amr::Octree<T>::OctCell &c)
    { 
      for (int iz=0;iz<2;iz++)
        for (int iy=0;iy<2;iy++)
          for (int ix=0;ix<2;ix++) {
            o << ((ix||iy||iz)?" ":"(") 
              << c.child[iz][iy][ix] 
              << ((!ix||!iy||!iz)?",":")") 
              << std::endl;
          }
      return o;
    }

  } // ::ospray::amr
} // ::ospray



