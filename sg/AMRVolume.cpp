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

/*! \file sg/volume/Tumes.cpp Scene Graph Importer Plugin for Neuromorpho SWC files */

#undef NDEBUG
// ospray
// ospray/sg
#include "AMRVolume.h"
#include "sg/common/Node.h"
#include "sg/common/Integrator.h"
#include "sg/module/Module.h"
	 
namespace ospray {
  namespace sg {

    using std::endl;
    using std::cout;
    using namespace amr;

    AMRVolume::AMRVolume() 
      : ospVolume(NULL)
    {}
      
    //! return bounding box of all primitives
    box3f AMRVolume::getBounds()
    {
      return box3f(vec3f(0.f), vec3f(dimensions));
    }

    template<typename T>
    void readMem(unsigned char *&ptr, T &t)
    {
      memcpy(&t,ptr,sizeof(t));
      ptr += sizeof(t);
    }

    template<typename T>
    void AMRVolume::readAMR(OSPVolume volume, const char *voxelType,
                            unsigned char *dataMem)
    {
      ospSetString(volume,"voxelType",voxelType);


      // iw: this code no longer works since swithcing to multi-octree data structure

      // size_t numGrids;
      // readMem(dataMem,numGrids);
      // PRINT(numGrids);
      // ospSet1i(volume,"numGrids",numGrids);

      // std::vector<OSPData> voxelDataVector;
      // std::vector<OSPData> cellDataVector;

      // typename AMR<T>::Grid *grid = new typename AMR<T>::Grid[numGrids];
      // for (int i=0;i<numGrids;i++) {
      //   readMem(dataMem,grid[i].cellDims);
      //   readMem(dataMem,grid[i].numCells);
      //   grid[i].cell = NULL; //(typename AMR<T>::Grid::Cell *)dataPtr;
      //   OSPData cellData = ospNewData(grid[i].numCells*sizeof(typename AMR<T>::Grid::Cell),
      //                                 OSP_CHAR,dataMem,OSP_DATA_SHARED_BUFFER);
      //   cellDataVector.push_back(cellData);
      //   dataMem += grid[i].numCells * sizeof(typename AMR<T>::Grid::Cell);
      //   readMem(dataMem,grid[i].voxelDims);
      //   readMem(dataMem,grid[i].numVoxels);

      //   grid[i].voxel = NULL; //(typename AMR<T>::Grid::Voxel *)dataPtr;
      //   OSPData voxelData = ospNewData(grid[i].numVoxels*sizeof(typename AMR<T>::Grid::Voxel),
      //                                 OSP_CHAR,dataMem,OSP_DATA_SHARED_BUFFER);
      //   voxelDataVector.push_back(voxelData);
      //   dataMem += grid[i].numVoxels * sizeof(typename AMR<T>::Grid::Voxel);
      // }

      // // create data buffer for grid info, and assign to volume
      // OSPData gridData = ospNewData(sizeof(typename AMR<T>::Grid)*numGrids,OSP_CHAR,grid,OSP_DATA_SHARED_BUFFER);
      // ospSetData(volume,"gridData",gridData);

      // // create data buffer for the individual voxel arrays, and assign
      // OSPData voxelData = ospNewData(voxelDataVector.size(),OSP_DATA,&voxelDataVector[0]);
      // ospSetData(volume,"voxelDataArraysData",voxelData);
      // // create data buffer for the individual cell arrays, and assign
      // OSPData cellData = ospNewData(cellDataVector.size(),OSP_DATA,&cellDataVector[0]);
      // ospSetData(volume,"cellDataArraysData",cellData);
    }

    void AMRVolume::render(RenderContext &ctx)
    {
      if (ospVolume) 
        return;

      ospLoadModule("amr");
      ospVolume = ospNewVolume("AMRVolume");
      
      readAMR<float>(ospVolume,"float",(unsigned char *)dataPtr);
      // unsigned char *basePtr = (unsigned char *)dataPtr;

      
      // int numGrids;
      // readMem(basePtr,numGrids);
      // PRINT(numGrids);
      // AMR<float>
      // PING;
      // PRINT(dataPtr);
      // PRINT(*(int*)dataPtr);
      // PRINT(dataSize);
      // ospSetData(ospVolume,"amrData",ospAMRData);
      ospCommit(ospVolume);
      
      // and finally, add this volume to the model
      ospAddVolume(ctx.world->ospModel,ospVolume);
      
    }    

    //! \brief Initialize this node's value from given XML node 
    void AMRVolume::setFromXML(const xml::Node *const node, const unsigned char *binBasePtr)
    {
      size_t ofs = node->getPropl("ofs");
      dataSize = node->getPropl("size");
      dataPtr = (void *)(binBasePtr+ofs);
      const std::string voxelType = node->getProp("voxelType");
      this->voxelType = typeForString(voxelType.c_str());
    }

    OSP_REGISTER_SG_NODE(AMRVolume);

    OSPRAY_SG_DECLARE_MODULE(amr)
    {
      cout << "#osp:sg: loading 'mrbvolume' module (defines sg::MRBVolume type)" << endl;
    }

  }
}

