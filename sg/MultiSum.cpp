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

#include "MultiSum.h"
	 
namespace ospray {
  namespace sg {

    using std::endl;
    using std::cout;
    using namespace amr;

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

      // -------------------------------------------------------
      // format, root size, value range, ...
      // -------------------------------------------------------
      const std::string voxelType = node->getProp("voxelType");
      if (voxelType != "float")
        throw std::runtime_error("can only do float MultiSumAMR right now");
      this->voxelType = typeForString(voxelType.c_str());
      this->samplingRate = node->getPropf("samplingRate",1.f);
      this->valueRange = Range<float>::fromString(node->getProp("valueRange"),
                                                  Range<float>(0.f,1.f));

      rootGridSize = toVec3i(node->getProp("rootGrid").c_str());
      clipBoxSize = toVec3f(node->getProp("clipBoxSize").c_str());
      maxLevel = node->getPropl("maxLevel",0);

      // -------------------------------------------------------
      // data and index blocks
      // -------------------------------------------------------
      std::vector<int> dataBlockCount;
      std::vector<int> indexBlockCount;
      for (int childID=0;childID<node->child.size();childID++) {
        const xml::Node *child = node->child[childID];
        if (child->name == "dataBlocks")
          parseVecInt(dataBlockCount,child->content.c_str());
        else if (child->name == "dataBricks")
          parseVecInt(dataBlockCount,child->content.c_str());
        else if (child->name == "indexBlocks")
          parseVecInt(indexBlockCount,child->content.c_str());
        else if (child->name == "indexBricks")
          parseVecInt(indexBlockCount,child->content.c_str());
      }
      multiSum = new Sumerian;
      multiSum->mapFrom(binBasePtr,rootGridSize,clipBoxSize,dataBlockCount,indexBlockCount);

      // -------------------------------------------------------
      // transfer function
      // -------------------------------------------------------
      std::string xfName = node->getProp("transferFunction");
      if (xfName != "") {
        this->transferFunction = dynamic_cast<TransferFunction*>(sg::findNamedNode(xfName));
      }
      if (this->transferFunction.ptr == NULL)
        this->transferFunction = new TransferFunction;

      cout << "setting xf value range " << this->valueRange << endl;
      this->transferFunction->setValueRange(this->valueRange.toVec2f());
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

      ospSet1f(ospVolume,"samplingRate",samplingRate);
      ospSet1i(ospVolume,"maxLevel",maxLevel);

      firstIndexBlockOfTreeData
        = ospNewData(multiSum->rootGridDims.product(),OSP_INT,
                     multiSum->firstIndexBlockOfTree,OSP_DATA_SHARED_BUFFER);
      ospSetData(ospVolume,"firstIndexBlockOfTree",firstIndexBlockOfTreeData);

      firstDataBlockOfTreeData
        = ospNewData(multiSum->rootGridDims.product(),OSP_INT,
                     multiSum->firstDataBlockOfTree,OSP_DATA_SHARED_BUFFER);
      ospSetData(ospVolume,"firstDataBlockOfTree",firstDataBlockOfTreeData);

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

    OSP_REGISTER_SG_NODE(MultiSumAMR);

  }
}
