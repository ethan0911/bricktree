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
    Array3D<T>::Array3D(const vec3i &dims)
      : dims(dims), value(new T[size_t(dims.x)*size_t(dims.y)*size_t(dims.z)])
    {}
    
    template<typename T>
    void Array3D<T>::set(const vec3i &where, T value)
    {
      this->value[where.x + size_t(dims.x)*(where.y + size_t(dims.y)*where.z)] = value;
    }

    template<typename T>
    T Array3D<T>::get(const vec3i &where) const
    {
      return value[where.x + size_t(dims.x)*(where.y + size_t(dims.y)*where.z)];
    }

    template<typename T>
    T Array3D<T>::getSafe(const vec3i &_where) const
    {
      return get(max(vec3i(0),min(_where,dims-vec3i(1))));
    }

    template<typename T>
    void loadRAW(Array3D<T> &volume, const std::string &fileName, const vec3i &dims)
    {
      if (dims != volume.dims)
        throw std::runtime_error("ospray::amr::loadRaw(): dims of volume and input aren't matching'");

      FILE *file = fopen(fileName.c_str(),"rb");
      if (!file)
        throw std::runtime_error("ospray::amr::loadRaw(): could not open '"+fileName+"'");
      const size_t num = size_t(dims.x) * size_t(dims.y) * size_t(dims.z);
      size_t numRead = fread(volume.value,sizeof(T),num,file);
      if (num != numRead)
        throw std::runtime_error("ospray::amr::loadRaw(): read incomplete data ...");
      fclose(file);
    }
    
    template void loadRAW(Array3D<uint8> &volume, const std::string &fileName, const vec3i &dims);
    template void loadRAW(Array3D<float> &volume, const std::string &fileName, const vec3i &dims);

    template struct Array3D<float>;
    template struct Array3D<uint8>;
  }
}
