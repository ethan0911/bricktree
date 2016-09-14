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

#include "SGBrickTree.h"
#include "sg/module/Module.h"

namespace ospray {
  namespace sg {

    using std::endl;
    using std::cout;
    using namespace bt;

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
    void BrickTree::setFromXML(const xml::Node *const node, 
                               const unsigned char *binBasePtr)
    {
      // -------------------------------------------------------
      // format, root size, value range, ...
      // -------------------------------------------------------
      const std::string format = node->getProp("format");
      if (format != "float")
        throw std::runtime_error("can only do float BrickTree right now");
      this->voxelType = typeForString(format.c_str());
      this->samplingRate = toFloat(node->getProp("samplingRate","1.f").c_str());
      int brickSize      = toInt(node->getProp("brickSize").c_str());
      assert(brickSize > 0);
      int depth          = toInt(node->getProp("depth","0").c_str());
      assert(depth > 0);

      rootGridSize = toVec3i(node->getProp("gridSize").c_str());
      clipBoxSize  = toVec3f(node->getProp("validSize").c_str());

      std::string fileName = node->doc->fileName;
      PRINT(fileName);
      
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

    void BrickTree::render(RenderContext &ctx)
    {
#if 0
      if (ospVolume) 
        return;

      ospLoadModule("bt");
      ospVolume = ospNewVolume("BrickTreeVolume");
      if (!ospVolume) 
        throw std::runtime_error("could not create ospray 'BrickTreeVolume'");
      
      size_t sizeOfBrick
        = multiSum->brickSize()*multiSum->brickSize()*multiSum->brickSize() * sizeof(int);
      // -------------------------------------------------------
      valueBrickValue = ospNewData(multiSum->numValueBricks*sizeOfBrick,
                                 OSP_UCHAR,
                                 multiSum->valueBrickPtr(),OSP_DATA_SHARED_BUFFER);
      ospSetValue(ospVolume,"valueBrickValue",valueBrickValue);
      indexBrickValue = ospNewData(multiSum->numIndexBricks*sizeOfBrick,
                                  OSP_UCHAR,
                                  multiSum->indexBrickPtr(),OSP_DATA_SHARED_BUFFER);
      ospSetValue(ospVolume,"indexBrickValue",indexBrickValue);
      brickInfoValue = ospNewData(multiSum->numIndexBricks*sizeOfBrick,
                                 OSP_UCHAR,
                                 multiSum->brickInfoPtr(),OSP_DATA_SHARED_BUFFER);
      ospSetValue(ospVolume,"brickInfoValue",brickInfoValue);

      ospSet1f(ospVolume,"samplingRate",samplingRate);
      ospSet1i(ospVolume,"maxLevel",maxLevel);

      firstIndexBrickOfTreeValue
        = ospNewData(multiSum->getRootGridDims().product(),OSP_INT,
                     multiSum->firstIndexBrickOfTreePtr(),OSP_DATA_SHARED_BUFFER);
      ospSetValue(ospVolume,"firstIndexBrickOfTree",firstIndexBrickOfTreeValue);

      firstValueBrickOfTreeValue
        = ospNewData(multiSum->getRootGridDims().product(),OSP_INT,
                     multiSum->firstValueBrickOfTreePtr(),OSP_DATA_SHARED_BUFFER);
      ospSetValue(ospVolume,"firstValueBrickOfTree",firstValueBrickOfTreeValue);

      vec3i rootDims = multiSum->getRootGridDims();
      ospSetVec3i(ospVolume,"rootGridDims",(osp::vec3i&)rootDims);
      ospSetVec3f(ospVolume,"validFractionOfRootGrid",multiSum->validFractionOfRootGrid);

      // -------------------------------------------------------
      std::cout << "#sg:bt: adding transfer function" << std::endl;
      if (transferFunction) {
        transferFunction->render(ctx);
        ospSetObject(ospVolume,"transferFunction",transferFunction->getOSPHandle());
      }
      
      // -------------------------------------------------------
      std::cout << "#sg:bt: committing Multi-BrickTree volume" << std::endl;
      ospCommit(ospVolume);
      
      // and finally, add this volume to the model
      cout << "adding volume " << ospVolume << endl;
      ospAddVolume(ctx.world->ospModel,ospVolume);
#endif
    }    

    OSP_REGISTER_SG_NODE(BrickTree);
    OSP_SG_REGISTER_MODULE(bricktree) {
      printf("loading bricktree scene graph plugin\n");
    }
  }
}
