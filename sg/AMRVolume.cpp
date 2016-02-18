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

    void AMRVolume::render(RenderContext &ctx)
    {
      if (ospVolume) 
        return;

      ospLoadModule("amr");
      ospVolume = ospNewVolume("AMRVolume");

      FATAL("not implemented");
      
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
    void AMRVolume::setFromXML(const xml::Node *const node, 
                               const unsigned char *binBasePtr)
    {
      size_t ofs = node->getPropl("ofs");
      dataSize = node->getPropl("size");
      dataPtr = (void *)(binBasePtr+ofs);
      const std::string voxelType = node->getProp("voxelType");
      if (voxelType != "float")
        throw std::runtime_error("can only do float AMR right now");
      this->voxelType = typeForString(voxelType.c_str());

      AMR<float> *amr = new AMR<float>;
      amr->mmapFrom(binBasePtr);

      FATAL("not implemented");
    }

    typedef AMRVolume MultiOctreeAMR;
    OSP_REGISTER_SG_NODE(MultiOctreeAMR);
    OSP_REGISTER_SG_NODE(AMRVolume);

    OSPRAY_SG_DECLARE_MODULE(amr)
    {
      cout << "#osp:sg: loading 'mrbvolume' module (defines sg::MRBVolume type)" << endl;
    }

  }
}

