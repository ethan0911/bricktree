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
#include "AMR.h"

namespace ospray {
  namespace amr {

    template<typename T>
    struct FromArray3DBuilder {
      // constructor
      FromArray3DBuilder();

      // build one root block (the one with given ID, obviously)
      void makeBlock(const vec3i &blockID);

      typename Octree<T>::Cell recursiveBuild(const vec3i &begin, int blockSize);

      size_t inputCellID(const vec3i &cellID) const;
      size_t rootCellID(const vec3i &cellID) const;

      // build the entire thing
      AMR<T> *makeAMR(const Array3D<T> *input, int maxLevels, float threshold);
      
      // pre-loaded input volume
      const Array3D<T> *input;
      
      // max number of levels we are allowed to create
      int maxLevels;
      
      // threshold for refining
      float threshold;

      AMR<T> *oct;
    };
    
  } // ::ospray::amr
} // ::ospray

