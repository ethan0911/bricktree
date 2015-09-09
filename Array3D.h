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
#include "ospray/volume/Volume.h"

namespace ospray {
  namespace amr {

    template<typename value_t>
    struct Array3D {
      Array3D(const vec3i &dims);
      void set(const vec3i &where, value_t value);
      value_t get(const vec3i &where) const;
      value_t getSafe(const vec3i &where) const;

      const vec3i dims;
      value_t *value;
    };

    /*! load a RAW file with given dims (and templated voxel type) into a 3D Array */
    template<typename T>
    void loadRAW(Array3D<T> &volume, const std::string &fileName, const vec3i &dims);

  }
}
