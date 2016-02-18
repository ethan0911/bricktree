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
      : Volume()
    {
      ispcEquivalent = ispc::AMRVolume_create(this);
    }

    // void AMRVolume::finalize(Model *model)
    // {
    //   amrData = getParamData("amrData",NULL);
    //   if (!amrData) {
    //     cout << "#osp:amr: AMRVolume without AMR data" << endl;
    //     return;
    //   }
    //   // ispc::AMRVolume_set(getIE(),model->getIE(),
    //   //                         (const ispc::box3f&)bounds,
    //   //                         (ispc::TubesNode*)nodeArray,numNodes);
    // }

    //! Create the equivalent ISPC volume container.
    void AMRVolume::createEquivalentISPC()
    {
      PING;
    }

    //! Allocate storage and populate the volume.
    void AMRVolume::commit()
    {
      cout << "#osp:amr: AMRVolume::commit()" << endl;
      Ref<Data> rootCellData = getParamData("rootCellData");
      assert(rootCellData);
      Ref<Data> octCellData = getParamData("octCellData");
      assert(octCellData);
      dimensions = getParam3i("dimensions",vec3i(-1));
      PRINT(dimensions);

      // Ref<Data> gridData = getParamData("gridData",NULL);
      // AMR<float>::Grid *grid = (AMR<float>::Grid*)gridData->data;
      // size_t numGrids = getParam1i("numGrids",-1);
      
      // Data *voxelDataArraysData = getParamData("voxelDataArraysData",NULL);
      // Data *cellDataArraysData = getParamData("cellDataArraysData",NULL);

      // for (int i=0;i<numGrids;i++) {
      //   grid[i].voxel = (float *)(((Data **)voxelDataArraysData->data)[i]->data);
      //   grid[i].cell = (AMR<float>::Grid::Cell *)(((Data **)cellDataArraysData->data)[i]->data);
      //   PRINT(grid[i].cell[i].lo);
      //   PRINT(grid[i].cell[i].hi);
      // }
    }
    
    OSP_REGISTER_VOLUME(AMRVolume,MultiOctreeAMRVolume);

    extern "C" void ospray_init_module_amr()
    {
      std::cout << "#amr: initializing ospray amr plugin" << std::endl;
    }
  }
} // ::ospray

