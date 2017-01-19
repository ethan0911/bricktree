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
#include "ospray/common/OSPCommon.h"

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

      /*! constructor */
    BrickTree::BrickTree()
      : ospVolume(NULL),
        brickSize(-1),
        blockWidth(-1),
        validSize(-1),
        gridSize(-1),
        format("<none>"),
        fileName("<none>"),
        valueRange(one)
    {};
    
    //! return bounding box of all primitives
    box3f BrickTree::getBounds() 
    {
      const box3f bounds = box3f(vec3f(0.f),vec3f(validSize));
      return bounds;
    }
    
    //! \brief Initialize this node's value from given XML node 
    void BrickTree::setFromXML(const xml::Node &node, 
                               const unsigned char *binBasePtr)
    {
      // -------------------------------------------------------
      // parameter parsing
      // -------------------------------------------------------
      format       = node.getProp("format");
      samplingRate = toFloat(node.getProp("samplingRate","1.f").c_str());
      brickSize    = toInt(node.getProp("brickSize").c_str());
      blockWidth   = toInt(node.getProp("blockWidth").c_str());
      gridSize     = toVec3i(node.getProp("gridSize").c_str());
      validSize    = toVec3i(node.getProp("validSize").c_str());
      fileName     = node.doc->fileName;

      assert(gridSize.x > 0);
      assert(gridSize.y > 0);
      assert(gridSize.z > 0);
      PRINT(gridSize);

      // -------------------------------------------------------
      // parameter sanity checking 
      // -------------------------------------------------------
      if (format != "float")
        throw std::runtime_error("can only do float BrickTree right now");
      this->voxelType = typeForString(format.c_str());

      // -------------------------------------------------------
      // transfer function
      // -------------------------------------------------------
      std::string xfName = node.getProp("transferFunction");
      if (xfName != "") {
        // this->transferFunction = dynamic_cast<TransferFunction*>(sg::findNamedNode(xfName));
      }
      if (!this->transferFunction)
        this->transferFunction = std::make_shared<TransferFunction>();
     
      cout << "setting xf value range " << this->valueRange << endl;
      this->transferFunction->setValueRange(this->valueRange.toVec2f());
      cout << "-------------------------------------------------------" << endl;
    }

    void BrickTree::render(RenderContext &ctx)
    {
      if (ospVolume) 
        return;

      ospLoadModule("bricktree");
      ospVolume = ospNewVolume("BrickTreeVolume");
      if (!ospVolume) 
        throw std::runtime_error("could not create ospray 'BrickTreeVolume'");

      assert(transferFunction);
      transferFunction->render(ctx);
      assert(transferFunction->getOSPHandle());
      
      // -------------------------------------------------------
      ospSet1i(ospVolume,"blockWidth",blockWidth);
      ospSet1i(ospVolume,"brickSize",brickSize);
      ospSet3i(ospVolume,"gridSize",gridSize.x,gridSize.y,gridSize.z);
      ospSetString(ospVolume,"fileName",fileName.c_str());
      ospSetString(ospVolume,"format",format.c_str());
      ospSetObject(ospVolume,"transferFunction",transferFunction->getOSPHandle());
      std::cout << "#sg:bt: committing Multi-BrickTree volume" << std::endl;
      ospCommit(ospVolume);
      
      // and finally, add this volume to the model
      cout << "adding volume " << ospVolume << endl;
      ospAddVolume(ctx.world->ospModel,ospVolume);
    }    

    /*! in the most exact sense of the word our data structure is a
        MULTI-bricktree, which is the name of the scene graph node
        that the converter creates; so make sure we 'define' this type
        here */
    typedef BrickTree MultiBrickTree;
    
    OSP_REGISTER_SG_NODE(MultiBrickTree);
    OSP_SG_REGISTER_MODULE(bricktree) {
      printf("loading bricktree scene graph plugin\n");
    }
  }
}
