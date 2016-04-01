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

/*! \file sg/ChomboVolume.cpp node for reading and rendering chombo files */

#undef NDEBUG

#include "ChomboVolume.h"
// sg
#include "sg/importer/Importer.h"
// amr
#include "amr/Array3D.h"

namespace ospray {
  namespace sg {

    using std::endl;
    using std::cout;

    void ChomboHDFImporter(const FileName &fileName,
                           sg::ImportState &importerState)
    {
      ChomboVolume *cv = new ChomboVolume;
      cv->parseChomboFile(fileName);

      cv->setTransferFunction(new TransferFunction);

      importerState.world->add(cv);
    }

    void ChomboVolume::parseChomboFile(const FileName &fileName)
    {
      assert(chombo == NULL);
      chombo = ospray::chombo::Chombo::parse(fileName);
      assert(!chombo->level.empty());

      cout << "parsed chombo hdf5 file, found " << chombo->level.size() << " levels" << endl;
      box3i rootLevelBounds = empty;
      for (int i=0;i<chombo->level[0]->boxes.size();i++)
        rootLevelBounds.extend(chombo->level[0]->boxes[i]);
      cout << "root level has bounds of " << rootLevelBounds << endl;
      assert(rootLevelBounds.lower == vec3i(0));
      rootGridSize = rootLevelBounds.upper;
    }

    /*! 'render' the nodes - all geometries, materials, etc will
      create their ospray counterparts, and store them in the
      node  */
    void ChomboVolume::render(RenderContext &ctx)
    {
      if (ospVolume != NULL)
        // already rendered...
        return;
      
      ospLoadModule("amr");
      cout << "-------------------------------------------------------" << endl;
      cout << "#sg:chom: rendering chombo data ..." << endl;
      Range<float> valueRange = empty;
      for (int levelID=0;levelID<chombo->level.size();levelID++) {
        const chombo::Level *level = this->chombo->level[levelID];
        cout << " - level: " << levelID << " : " << level->boxes.size() << " boxes" << endl;
        for (int brickID=0;brickID<level->boxes.size();brickID++) {
          BrickInfo bi;
          bi.box = level->boxes[brickID];
          bi.dt = level->dt;
          bi.level = levelID;

          brickInfo.push_back(bi);

          const double *inPtr
            = &level->data[0]
            + level->offsets[brickID]
            + componentID * bi.size().product();

          amr::ActualArray3D<float> brickData(bi.size());

          for (int iz=0;iz<brickData.size().z;iz++)
            for (int iy=0;iy<brickData.size().y;iy++)
              for (int ix=0;ix<brickData.size().x;ix++) {
                brickData.set(vec3i(ix,iy,iz),*inPtr);
                valueRange.extend(*inPtr);
                ++inPtr;
              }
          OSPData data = ospNewData(bi.size().product(),OSP_FLOAT,
                                    brickData.value,0);
          this->brickData.push_back(data);
        }
      }
      cout << "#sg:chom: found " << brickInfo.size() << " bricks" << endl;

      ospVolume = ospNewVolume("chombo_volume");
      assert(ospVolume);
      
      brickDataData = ospNewData(brickData.size(),OSP_OBJECT,&brickData[0],0);
      ospSetData(ospVolume,"brickData",brickDataData);
      brickInfoData = ospNewData(brickInfo.size()*sizeof(brickInfo[0]),OSP_RAW,&brickInfo[0],0);
      ospSetData(ospVolume,"brickInfo",brickInfoData);

      if (transferFunction) {
        cout << "-------------------------------------------------------" << endl;
        std::cout << "#sg:chom: adding transfer function" << std::endl;
        cout << "setting xf value range " << valueRange << endl;
        transferFunction->setValueRange(valueRange.toVec2f());
        transferFunction->render(ctx);
        cout << "#sg:chom: setting transfer function " << transferFunction->toString() << endl;
        ospSetObject(ospVolume,"transferFunction",transferFunction->getOSPHandle());
      }

      cout << "-------------------------------------------------------" << endl;
      ospCommit(ospVolume);
      cout << "#sg.chom: adding volume " << ospVolume << endl;
      ospAddVolume(ctx.world->ospModel,ospVolume);
    }
    
    //! \brief Initialize this node's value from given XML node 
    void ChomboVolume::setFromXML(const xml::Node *const node, 
                                  const unsigned char *binBasePtr)
    {
      std::string fileName = node->getProp("fileName");
      parseChomboFile(fileName);

      // -------------------------------------------------------
      // transfer function
      // -------------------------------------------------------
      std::string xfName = node->getProp("transferFunction");
      if (xfName != "") {
        this->transferFunction = dynamic_cast<TransferFunction*>(sg::findNamedNode(xfName));
      }
      if (this->transferFunction.ptr == NULL)
        this->transferFunction = new TransferFunction;
      cout << "-------------------------------------------------------" << endl;
    }
    
    OSP_REGISTER_SG_NODE(ChomboVolume);

  }
}

