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

#include "AMRVolume.h"
// ospray
#include "ospray/common/Model.h"
#include "ospray/common/Data.h"
#include "ospray/transferFunction/TransferFunction.h"
// #include "ospray/common/Core.h"
// ispc exports
#include "AMRVolume_ispc.h"
// stl
#include <set>
#include <map>

namespace ospray {
  namespace amr {
    using std::endl;
    using std::cout;
    using std::ostream;
    using std::flush;

    using embree::area;

    AMRVolume::AMRVolume()
      : Volume(), dimensions(-1), voxelType(OSP_FLOAT)
    {
      ispcEquivalent = ispc::AMRVolume_create(this);
    }

    //! Allocate storage and populate the volume.
    void AMRVolume::commit()
    {
      // right now we only support floats ...
      this->voxelType = OSP_FLOAT;

      cout << "#osp:amr: AMRVolume::commit()" << endl;
      Ref<Data> rootCellData = getParamData("rootCellData");
      assert(rootCellData);
      Ref<Data> octCellData = getParamData("octCellData");
      assert(octCellData);
      dimensions = getParam3i("dimensions",vec3i(-1));
      PRINT(dimensions);

      PING;
      Ref<TransferFunction> xf = (TransferFunction*)getParamObject("transferFunction");
      PING;

      ispc::AMRVolume_set(getIE(),
                          xf->getIE(),
                          (ispc::vec3i &)dimensions,
                          rootCellData->data,
                          octCellData->data);
    }

    extern "C" float AMR_sample_scalar(void *self, vec3f *where)
    {
      // ManagedObject *obj = (ManagedObject *)self;
      // PRINT(obj->toString());
      return 0.5f;
    }
    
    OSP_REGISTER_VOLUME(AMRVolume,MultiOctreeAMRVolume);

    extern "C" void ospray_init_module_amr()
    {
      std::cout << "#amr: initializing ospray amr plugin" << std::endl;
    }
  }
} // ::ospray

