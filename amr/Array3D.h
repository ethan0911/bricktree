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
#include "Range.h"

namespace ospray {
  namespace amr {

    template<typename value_t>
    struct Array3D {
      Array3D(const vec3i &dims);

      /*! set cell location to given value */
      void set(const vec3i &where, value_t value);
      
      /*! get cell value at location

        \warning 'where' MUST be a valid cell location */
      value_t get(const vec3i &where) const;

      /*! get cell value at given location, but ensure that location
          is actually a valid cell ID inside the volume (clamps to
          nearest cell in volume if 'where' is outside) */
      value_t getSafe(const vec3i &where) const;

      /*! get the range/interval of all cell values in the given
        begin/end region of the volume */
      Range<value_t> getValueRange(const vec3i &begin, const vec3i &end) const;

      const vec3i dims;
      value_t *value;
    };

    /*! load a RAW file with given dims (and templated voxel type) into a 3D Array */
    template<typename T>
    void loadRAW(Array3D<T> &volume, const std::string &fileName, const vec3i &dims);

    template<typename T>
    inline Array3D<T> *loadRAW(const std::string &fileName, const vec3i &dims)
    { 
      Array3D<T> *a = new Array3D<T>(dims);
      loadRAW(*a,fileName,dims);
      return a;
    }

  } // ::ospray::amr
} // ::ospray
