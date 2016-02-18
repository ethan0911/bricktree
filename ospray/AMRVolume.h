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
// amr base
#include "../amr/AMR.h"

namespace ospray {
  namespace amr {

    struct AMRVolume : public ospray::Volume {
      AMRVolume();

      //! \brief common function to help printf-debugging 
      virtual std::string toString() const { return "ospray::amr::AMRVolume"; }
      
      /*! \brief integrates this volume's primitives into the respective
        model's comperation structure */
      // virtual void finalize(Model *model);

      //! Allocate storage and populate the volume.
      virtual void commit();

      //! Copy voxels into the volume at the given index (non-zero return value indicates success).
      virtual int setRegion(const void *source, const vec3i &index, const vec3i &count)
      {
        FATAL("'setRegion()' doesn't make sense for AMR volumes; they can only be set from existing data");
      }

      //! Create the equivalent ISPC volume container.
      virtual void createEquivalentISPC();

      Ref<Data>   amrData;
      OSPDataType voxelType;
    };
    
  }
}

