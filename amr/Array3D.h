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

      /*! get value range over entire volume */
      Range<value_t> getValueRange() const
      { return getValueRange(vec3i(0),size()); }

      /*! returns number of elements (as 64-bit int) across all dimensions */
      virtual size_t numElements() const = 0;
    };

    /*! implementation for an actual array3d that stores a 3D array of values */
    template<typename value_t>
    struct ActualArray3D : public Array3D<value_t> {

      ActualArray3D(const vec3i &dims, void *externalMem=NULL);
      virtual ~ActualArray3D() { if (valuesAreMine) delete[] value; }

      /*! return size (ie, "dimensions") of volume */
      virtual vec3i size() const override;

      /*! get cell value at location

        \warning 'where' MUST be a valid cell location */
      virtual value_t get(const vec3i &where) const override;

      /*! returns number of elements (as 64-bit int) across all dimensions */
      virtual size_t numElements() const override;

      /* compute the (1D) linear array index for a (3D) grid coordinate */
      size_t indexOf(const vec3i &coord) const;

      const vec3i dims;
      value_t *value;
      // bool that specified whether it was us that alloc'ed this mem,
      // and thus, whether we should free it upon termination.
      bool valuesAreMine;
    };

    /*! implemnetaiton of a wrapper class that makes an actual array3d
        of one type look like that of another type */
    template<typename in_t, typename out_t>
    struct Array3DAccessor : public Array3D<out_t> {

      Array3DAccessor(const Array3D<in_t> *actual);

      /*! return size (ie, "dimensions") of volume */
      virtual vec3i size() const override;

      /*! get cell value at location

        \warning 'where' MUST be a valid cell location */
      virtual out_t get(const vec3i &where) const override;

      /*! returns number of elements (as 64-bit int) across all dimensions */
      virtual size_t numElements() const override;

    private:
      //! the actual 3D array we're wrapping around
      const Array3D<in_t> *actual;
    };

    /*! wrapper class that generates an artifically larger data set by
        simply repeating the given input */
    template<typename T>
    struct Array3DRepeater : public Array3D<T> {

      Array3DRepeater(const Array3D<T> *actual, const vec3i &repeatedSize);

      /*! return size (ie, "dimensions") of volume */
      virtual vec3i size() const override;

      /*! get cell value at location

        \warning 'where' MUST be a valid cell location */
      virtual T get(const vec3i &where) const override;

      /*! returns number of elements (as 64-bit int) across all dimensions */
      virtual size_t numElements() const override;

      const vec3i repeatedSize;
      const Array3D<T> *const actual;
    };

    /*! load raw file with given dimensions. the 'type' of the raw
        file (uint8,float,...) is given through the function's
        template parameter */
    template<typename T>
    Array3D<T> *loadRAW(const std::string &fileName, const vec3i &dims);

    /*! load raw file with given dimensions. the 'type' of the raw
        file (uint8,float,...) is given through the function's
        template parameter */
    template<typename T>
    Array3D<T> *mmapRAW(const std::string &fileName, const vec3i &dims);

  } // ::ospray::amr
} // ::ospray
