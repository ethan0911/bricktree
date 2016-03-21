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

#include "amr/Sumerian.h"
	 
namespace ospray {
  namespace sg {

    using std::endl;
    using std::cout;
    using namespace amr;

    AMRVolume::AMRVolume() 
      : ospVolume(NULL), moa(NULL)
    {}
      
    //! return bounding box of all primitives
    box3f AMRVolume::getBounds()
    {
      assert(moa);
      return box3f(vec3f(0.f), vec3f(moa->dimensions));
    }

    void AMRVolume::render(RenderContext &ctx)
    {
      if (ospVolume) 
        return;

      ospLoadModule("amr");
      ospVolume = ospNewVolume("MultiOctreeAMRVolume");
      if (!ospVolume) 
        throw std::runtime_error("could not create ospray 'MultiOctreeAMRVolume'");
      
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
    void AMRVolume::setFromXML(const xml::Node *const node, 
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

      cout << "AMRVolume has xf: " << this->transferFunction << endl;
    }

    void parseVecInt(std::vector<int> &vec, const std::string &s)
    {
      char *ss = strdup(s.c_str());
      char *tok = strtok(ss," \t\n");
      while (tok) {
        vec.push_back(atoi(tok));
        tok = strtok(NULL," \n\t");
      }
      free(ss);
    }

    //! \brief Initialize this node's value from given XML node 
    void MultiSumAMR::setFromXML(const xml::Node *const node, 
                                 const unsigned char *binBasePtr)
    {
      size_t ofs = node->getPropl("ofs");
      size_t dataSize = node->getPropl("size");
      const std::string voxelType = node->getProp("voxelType");
      if (voxelType != "float")
        throw std::runtime_error("can only do float MultiSumAMR right now");
      this->voxelType = typeForString(voxelType.c_str());

      std::string xfName = node->getProp("transferFunction");
      if (xfName != "") {
        this->transferFunction = dynamic_cast<TransferFunction*>(sg::findNamedNode(xfName));
        cout << "USING NAMED XF " << this->transferFunction << endl;
      }
      if (this->transferFunction.ptr == NULL)
        this->transferFunction = new TransferFunction;
      
      rootGridSize = toVec3i(node->getProp("rootGrid").c_str());
      clipBoxSize = toVec3f(node->getProp("clipBoxSize").c_str());

      std::vector<int> dataBlockCount;
      std::vector<int> indexBlockCount;
      for (int childID=0;childID<node->child.size();childID++) {
        const xml::Node *child = node->child[childID];
        if (child->name == "dataBlocks")
          parseVecInt(dataBlockCount,child->content.c_str());
        if (child->name == "indexBlocks")
          parseVecInt(indexBlockCount,child->content.c_str());
      }

      multiSum = new Sumerian;
      multiSum->mapFrom(binBasePtr,rootGridSize,clipBoxSize,dataBlockCount,indexBlockCount);
      cout << "-------------------------------------------------------" << endl;
    }

    void MultiSumAMR::render(RenderContext &ctx)
    {
      if (ospVolume) 
        return;

      ospLoadModule("amr");
      ospVolume = ospNewVolume("MultiSumAMRVolume");
      if (!ospVolume) 
        throw std::runtime_error("could not create ospray 'MultiSumAMRVolume'");
      
      // -------------------------------------------------------
      dataBlockData = ospNewData(multiSum->numDataBlocks*sizeof(multiSum->dataBlock[0]),OSP_UCHAR,
                                 multiSum->dataBlock,OSP_DATA_SHARED_BUFFER);
      ospSetData(ospVolume,"dataBlockData",dataBlockData);
      indexBlockData = ospNewData(multiSum->numIndexBlocks*sizeof(multiSum->indexBlock[0]),OSP_UCHAR,
                                   multiSum->indexBlock,OSP_DATA_SHARED_BUFFER);
      ospSetData(ospVolume,"indexBlockData",indexBlockData);
      blockInfoData = ospNewData(multiSum->numIndexBlocks*sizeof(multiSum->blockInfo[0]),OSP_UCHAR,
                                  multiSum->blockInfo,OSP_DATA_SHARED_BUFFER);
      ospSetData(ospVolume,"blockInfoData",blockInfoData);
      rootCellData = ospNewData(multiSum->rootGridDims.product(),OSP_INT,
                                multiSum->firstBlockOfRootCell,OSP_DATA_SHARED_BUFFER);
      ospSetData(ospVolume,"rootCellData",rootCellData);
      ospSetVec3i(ospVolume,"rootGridDims",(osp::vec3i&)multiSum->rootGridDims);
      ospSetVec3f(ospVolume,"validFractionOfRootGrid",multiSum->validFractionOfRootGrid);

      // -------------------------------------------------------
      std::cout << "#sg:amr: adding transfer function" << std::endl;
      if (transferFunction) {
        transferFunction->render(ctx);
        ospSetObject(ospVolume,"transferFunction",transferFunction->getOSPHandle());
      }
      
      // -------------------------------------------------------
      std::cout << "#sg:amr: committing Multi-Octree AMR volume" << std::endl;
      ospCommit(ospVolume);
      
      // and finally, add this volume to the model
      cout << "adding volume " << ospVolume << endl;
      ospAddVolume(ctx.world->ospModel,ospVolume);

    }    



    typedef AMRVolume MultiOctreeAMR;

    OSP_REGISTER_SG_NODE(MultiOctreeAMR);
    OSP_REGISTER_SG_NODE(MultiSumAMR);
    OSP_REGISTER_SG_NODE(AMRVolume);

    OSPRAY_SG_DECLARE_MODULE(amr)
    {
      cout << "#osp:sg: loading 'mrbvolume' module (defines sg::MRBVolume type)" << endl;
    }

  }
}

