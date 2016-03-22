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

#undef NDEBUG

#include "MultiOctree.h"

namespace ospray {
  namespace sg {

    using std::endl;
    using std::cout;
    using namespace amr;

    MultiOctreeAMR::MultiOctreeAMR() 
      : ospVolume(NULL), moa(NULL)
    {}
      
    //! return bounding box of all primitives
    box3f MultiOctreeAMR::getBounds()
    {
      assert(moa);
      return box3f(vec3f(0.f), vec3f(moa->dimensions));
    }

    void MultiOctreeAMR::render(RenderContext &ctx)
    {
      if (ospVolume) 
        return;

      ospLoadModule("amr");
      ospVolume = ospNewVolume("MultiOctreeMultiOctreeAMR");
      if (!ospVolume) 
        throw std::runtime_error("could not create ospray 'MultiOctreeMultiOctreeAMR'");
      
      assert(moa != NULL);

      std::cout << "#sg:amr: creating root cell data buffer" << std::endl;
      rootCellData = ospNewData(moa->rootCell.size()*sizeof(moa->rootCell[0]),OSP_UCHAR,
                                &moa->rootCell[0],OSP_DATA_SHARED_BUFFER);

      assert(rootCellData);
      ospSetData(ospVolume, "rootCellData", rootCellData);

      std::cout << "#sg:amr: creating oct cell data buffer" << std::endl;
      octCellData = ospNewData(moa->octCell.size()*sizeof(moa->octCell[0]),OSP_UCHAR,
                               &moa->octCell[0],OSP_DATA_SHARED_BUFFER);
      assert(octCellData);
      ospSetData(ospVolume, "octCellData", octCellData);

      ospSet3iv(ospVolume, "dimensions", &moa->dimensions.x);

      std::cout << "#sg:amr: adding transfer function" << std::endl;
      if (transferFunction) {
        transferFunction->render(ctx);
        ospSetObject(ospVolume,"transferFunction",transferFunction->getOSPHandle());
      }
      
      std::cout << "#sg:amr: committing Multi-Octree AMR volume" << std::endl;
      ospCommit(ospVolume);
      
      // and finally, add this volume to the model
      ospAddVolume(ctx.world->ospModel,ospVolume);

    }    

    //! \brief Initialize this node's value from given XML node 
    void MultiOctreeAMR::setFromXML(const xml::Node *const node, 
                               const unsigned char *binBasePtr)
    {
      size_t ofs = node->getPropl("ofs");
      size_t dataSize = node->getPropl("size");
      const std::string voxelType = node->getProp("voxelType");
      if (voxelType != "float")
        throw std::runtime_error("can only do float AMR right now");
      this->voxelType = typeForString(voxelType.c_str());

      this->moa = new AMR;
      this->moa->mmapFrom(binBasePtr);

      std::string xfName = node->getProp("transferFunction");
      if (xfName != "") {
        this->transferFunction = dynamic_cast<TransferFunction*>(sg::findNamedNode(xfName));
        cout << "USING NAMED XF " << this->transferFunction << endl;
      }
      
      if (this->transferFunction.ptr == NULL)
        this->transferFunction = new TransferFunction;

      cout << "MultiOctreeAMR has xf: " << this->transferFunction << endl;
    }

    OSP_REGISTER_SG_NODE(MultiOctreeAMR);

  }
}

