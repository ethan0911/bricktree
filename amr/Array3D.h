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

      /*! returns number of elements (as 64-bit int) across all dimensions */
      virtual size_t numElements() const = 0;
    };

    /*! implementation for an actual array3d that stores a 3D array of values */
    template<typename value_t>
    struct ActualArray3D : public Array3D<value_t> {

      ActualArray3D(const vec3i &dims);

      /*! return size (ie, "dimensions") of volume */
      virtual vec3i size() const ;

      /*! get cell value at location

        \warning 'where' MUST be a valid cell location */
      virtual value_t get(const vec3i &where) const ;

      virtual size_t numElements() const ;

      size_t indexOf(const vec3i &pos) const;

      const vec3i dims;
      value_t *value;
    };

    /*! implemnetaiton of a wrapper class that makes an actual array3d
        of one type look like that of another type */
    template<typename in_t, typename out_t>
    struct Array3DAccessor : public Array3D<out_t> {

      Array3DAccessor(const Array3D<in_t> *actual);

      /*! return size (ie, "dimensions") of volume */
      virtual vec3i size() const ;

      /*! get cell value at location

        \warning 'where' MUST be a valid cell location */
      virtual out_t get(const vec3i &where) const ;

      /*! returns number of elements (as 64-bit int) across all dimensions */
      virtual size_t numElements() const 
      { assert(actual); return actual->numElements(); }


    private:
      //! the actual 3D array we're wrapping around
      const Array3D<in_t> *actual;
    };

    template<typename T>
    Array3D<T> *loadRAW(const std::string &fileName, const vec3i &dims);

    // Inlined template definitions ///////////////////////////////////////////

    // Array3D //

    /*! get the range/interval of all cell values in the given
        begin/end region of the volume */
    // template<typename T>
    // inline Range<T> Array3D<T>::getValueRange(const vec3i &begin,
    //                                           const vec3i &end) const
    // {
    //   Range<T> v = get(begin);
    //   for (int iz=begin.z;iz<end.z;iz++)
    //     for (int iy=begin.y;iy<end.y;iy++)
    //       for (int ix=begin.x;ix<end.x;ix++) {
    //         v.extend(get(vec3i(ix,iy,iz)));
    //       }
    //   return v;
    // }

    // ActualArray3D //

    // template<typename T>
    // inline ActualArray3D<T>::ActualArray3D(const vec3i &dims)
    //   : dims(dims)
    // {
    //   const size_t numVoxels = size_t(dims.x)*size_t(dims.y)*size_t(dims.z);
    //   try {
    //     value = new T[numVoxels];
    //   } catch (std::bad_alloc e) {
    //     std::stringstream ss;
    //     ss << "could not allocate memory for Array3D of dimensions "
    //        << dims << " (in Array3D::Array3D())";
    //     throw std::runtime_error(ss.str());
    //   }
    // }

    template<typename T>
    inline vec3i ActualArray3D<T>::size() const
    {
      return dims;
    }

    template<typename T>
    inline size_t ActualArray3D<T>::numElements() const
    {
      return size_t(dims.x)*size_t(dims.y)*size_t(dims.z);
    }

    template<typename T>
    inline size_t ActualArray3D<T>::indexOf(const vec3i &pos) const
    {
      return pos.x+size_t(dims.x)*(pos.y+size_t(dims.y)*pos.z);
    }

    template<typename T>
    inline T ActualArray3D<T>::get(const vec3i &_where) const
    {
      assert(value != NULL);
      const vec3i where = max(vec3i(0),min(_where,dims - vec3i(1)));
      size_t index = where.x+size_t(dims.x)*(where.y+size_t(dims.y)*(where.z));
      return value[index];
    }

    // Array3DAccessor //

    template<typename in_t, typename out_t>
    inline Array3DAccessor<in_t, out_t>
    ::Array3DAccessor(const Array3D<in_t> *actual) :
      actual(actual)
    {}

    template<typename in_t, typename out_t>
    inline vec3i Array3DAccessor<in_t, out_t>::size() const
    {
      return actual->size();
    }

    template<typename in_t, typename out_t>
    inline out_t Array3DAccessor<in_t, out_t>::get(const vec3i &where) const
    {
      return (out_t)actual->get(where);
    }

  } // ::ospray::amr
} // ::ospray
