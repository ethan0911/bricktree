// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
// bt base
#include "../bt/BrickTree.h"

namespace ospray {
  namespace bt {

    /*! abstract base class for any type of scalar volume sampler 

      we will eventually specialize this for bricktree further below
    */
    struct ScalarVolumeSampler
    {
      /*! compute sample at given position */
      virtual float computeSample(const vec3f &pos) const = 0;
      
      /*! compute gradient at given position */
      virtual vec3f computeGradient(const vec3f &pos) const = 0;
    };

    /*! the actual C++ implementation of an ospray bricktree volume type 
      
      note this only maintains all kind of ospray state fo rthis
      object; the actual sampling is done in the sampler defined in
      the implementation file */
    struct BrickTreeVolume : public ospray::Volume
    {
      BrickTreeVolume();

      //! \brief common function to help printf-debugging 
      virtual std::string toString() const { return "ospray::bt::BrickTreeVolume"; }

      //! Allocate storage and populate the volume.
      virtual void commit();

      //! Copy voxels into the volume at the given index (non-zero return value indicates success).
      virtual int setRegion(const void *source, const vec3i &index, const vec3i &count) override;

      /*! create specialization of sampler for given type and brick size */
      template<typename T, int N>
      ScalarVolumeSampler *createSamplerTN();
      /*! create specialization of sampler for given type */
      template<typename T>
      ScalarVolumeSampler *createSamplerT();
      /*! create specialization of sampler given values for brick size an dtype */
      ScalarVolumeSampler *createSampler();


      ScalarVolumeSampler *sampler;
      
      vec3i       gridSize;
      vec3i       validSize;
      vec3f       validFractionOfRootGrid;
      int         depth;
      int         brickSize;
      int         blockWidth;
      std::string fileName;
      std::string format;
      OSPDataType voxelType;
    };
    
  }
}

