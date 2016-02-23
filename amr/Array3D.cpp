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

// ospray
#include "AMR.h"

namespace ospray {
  namespace amr {

    template<typename T>
    ActualArray3D<T>::ActualArray3D(const vec3i &dims)
      : dims(dims)
    {
      const size_t numVoxels = size_t(dims.x)*size_t(dims.y)*size_t(dims.z);
      try {
      value = new T[numVoxels];
      } catch (std::bad_alloc e) {
        std::stringstream ss;
        ss << "could not allocate memory for Array3D of dimensions "
           << dims << " (in Array3D::Array3D())";
        throw std::runtime_error(ss.str());
      }
    }
    
    template<typename T>
    T ActualArray3D<T>::get(const vec3i &_where) const
    {
      assert(value != NULL);
      const vec3i where = max(vec3i(0),min(_where,dims - vec3i(1)));
      size_t index = where.x + size_t(dims.x) * (where.y + size_t(dims.y) * (where.z));
      return value[index];
    }

    /*! get the range/interval of all cell values in the given
        begin/end region of the volume */
    template<typename T>
    inline Range<T> Array3D<T>::getValueRange(const vec3i &begin, const vec3i &end) const
    {
      Range<T> v = get(begin);
      for (int iz=begin.z;iz<end.z;iz++)
        for (int iy=begin.y;iy<end.y;iy++)
          for (int ix=begin.x;ix<end.x;ix++) {
            v.extend(get(vec3i(ix,iy,iz)));
          }
      return v;
    }

    template<typename T>
    Array3D<T> *loadRAW(const std::string &fileName, const vec3i &dims)
    {
      ActualArray3D<T> *volume = new ActualArray3D<T>(dims);
      FILE *file = fopen(fileName.c_str(),"rb");
      if (!file)
        throw std::runtime_error("ospray::amr::loadRaw(): could not open '"+fileName+"'");
      const size_t num = size_t(dims.x) * size_t(dims.y) * size_t(dims.z);
      size_t numRead = fread(volume->value,sizeof(T),num,file);
      if (num != numRead)
        throw std::runtime_error("ospray::amr::loadRaw(): read incomplete data ...");
      fclose(file);

      return volume;
    }
    
    // -------------------------------------------------------
    // explicit instantiations section
    // -------------------------------------------------------

    template Array3D<uint8> *loadRAW(const std::string &fileName, const vec3i &dims);
    template Array3D<float> *loadRAW(const std::string &fileName, const vec3i &dims);

    template struct Array3D<float>;
    template struct Array3D<uint8>;
  }
}