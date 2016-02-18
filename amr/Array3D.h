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

    /*! ABSTRACTION for a 3D array of data */
    template<typename value_t>
    struct Array3D {

      /*! return size (ie, "dimensions") of volume */
      virtual vec3i size() const = 0;

      /*! get cell value at given location, but ensure that location
        is actually a valid cell ID inside the volume (clamps to
        nearest cell in volume if 'where' is outside) */
      virtual value_t get(const vec3i &where) const = 0;

      /*! get the range/interval of all cell values in the given
        begin/end region of the volume */
      Range<value_t> getValueRange(const vec3i &begin, const vec3i &end) const;
    };

    template<typename value_t>
    struct ActualArray3D : public Array3D<value_t> {

      ActualArray3D(const vec3i &dims);

      /*! return size (ie, "dimensions") of volume */
      virtual vec3i size() const { return dims; }

      // /*! set cell location to given value */
      // virtual void set(const vec3i &where, value_t value);
      
      /*! get cell value at location

        \warning 'where' MUST be a valid cell location */
      virtual value_t get(const vec3i &where) const;

      const vec3i dims;
      value_t *value;
    };

    template<typename in_t, typename out_t>
    struct Array3DAccessor {
      Array3D<in_t> *actual;
    };

    template<typename T>
    Array3D<T> *loadRAW(const std::string &fileName, const vec3i &dims);

  } // ::ospray::amr
} // ::ospray
