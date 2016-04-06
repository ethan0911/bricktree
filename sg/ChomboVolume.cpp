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

      cout << "-------------------------------------------------------" << endl;
      cout << "done parsing chombo HDF file." << endl;
      cout << "now extracing brickinfo and float arrays from that ..." << endl;
      cout << "-------------------------------------------------------" << endl;
      
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

          size_t numValues = bi.size().product();
          float *f = new float[numValues];
          brickPtrs.push_back(f);
          // amr::ActualArray3D<float> brickData(bi.size());
          for (int iz=0;iz<bi.size().z;iz++)
            for (int iy=0;iy<bi.size().y;iy++)
              for (int ix=0;ix<bi.size().x;ix++) {
                // brickData.set(vec3i(ix,iy,iz),*inPtr);
                valueRange.extend(*inPtr);
                *f++ = *inPtr++;
              }
        }
      }
      cout << "#sg:chom: found " << brickInfo.size() << " bricks" << endl;
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
      cout << "-------------------------------------------------------" << endl;
      
#if 1
      assert(!brickInfo.empty());
      for (int bID=0;bID<brickInfo.size();bID++) {
        BrickInfo bi = brickInfo[bID];
        OSPData data = ospNewData(bi.size().product(),OSP_FLOAT,
                                  this->brickPtrs[bID],OSP_DATA_SHARED_BUFFER); //brickData.value,0);
        this->brickData.push_back(data);
      }
      cout << "#sg:chom: created " << this->brickData.size() << " data arrays for bricks..." << endl;
#else
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
#endif

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
      cout << "#sg.chom: done adding volume..." << endl;
    }
    
    //! \brief Initialize this node's value from given XML node 
    void ChomboVolume::setFromXML(const xml::Node *const node, 
                                  const unsigned char *binBasePtr)
    {
      std::string fileName = node->getProp("fileName");
      if (fileName != "") 
        parseChomboFile(fileName);
      else {
        cout << "#osp:sg: setting chombo volume from XML node" << endl;
        // parse directly from XML file
        int numLevels = node->getPropl("numLevels");
        assert(numLevels > 0);
        int bs = node->getPropl("brickSize");
        assert(bs > 0);
        const std::string valueRange = node->getProp("valueRange");
        int rc = sscanf(valueRange.c_str(),"%f %f",&this->valueRange.lo,&this->valueRange.hi);
        assert(rc == 2);

        rootGridSize = vec3i(0);

        float dt = 1.f;
        const std::string xmlFileName = node->doc->fileName;
        PRINT(xmlFileName);
        const std::string  baseName = node->doc->fileName.base();
        PRINT(baseName);
        for (int level=0;level<numLevels;level++) {
          char levelFileName[10000];
          strcpy(levelFileName,xmlFileName.c_str());
          char *ext = strstr(levelFileName,".osp");
          if (!ext)
            throw std::runtime_error("could not detect '.osp' extension in input file name");
          sprintf(ext,"_level%02d.bin",level);
          FILE *bin = fopen(levelFileName,"rb");
          if (!bin) throw std::runtime_error("no nodes on level! file="+std::string(levelFileName));
          
          while (1) {
            vec3i coord;
            int rc = fread(&coord,sizeof(coord),1,bin);
            if (!rc) 
              break;
            BrickInfo bi;
            bi.box.lower = coord;
            bi.box.upper = coord + vec3i(bs-1);
            bi.level = level;
            bi.dt = dt;
            if (level == 0)
              rootGridSize = max(rootGridSize,bi.box.upper+vec3i(1));
            
            float *f = new float[bs*bs*bs];

            rc = fread(f,sizeof(float),bs*bs*bs,bin);
            assert(rc == bs*bs*bs);

            // cout << "found " << coord << ":" << level << " val[0]=" << f[0] << endl;

            brickInfo.push_back(bi);
            brickPtrs.push_back(f);
          }
          fclose(bin);
          dt /= float(bs);
        }
      }
      PRINT(rootGridSize);
      
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

