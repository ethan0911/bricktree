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

// ospray
// ospray/sg
#include "sg/volume/Volume.h"
#include "sg/module/Module.h"
// chombo
#include "ChomboReader.h"
// amr
#include "amr/Array3D.h"
	 
namespace ospray {
  namespace sg {

    struct ChomboVolume : public Volume {
      ChomboVolume() : ospVolume(NULL), chombo(NULL), componentID(0),valueRange(empty) {}

      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const
      { return "ospray::sg::ChomboVolume (ospray_amr module)"; }

      /*! 'render' the nodes - all geometries, materials, etc will
          create their ospray counterparts, and store them in the
          node  */
      virtual void render(RenderContext &ctx);

      //! \brief Initialize this node's value from given XML node 
      virtual void setFromXML(const xml::Node *const node, 
                              const unsigned char *binBasePtr);

      //! return bounding box of all primitives
      virtual box3f getBounds() { return box3f(vec3f(0.f),vec3f(rootGridSize)); }

      void parseChomboFile(const FileName &fileName, const std::string &desiredComponent);

      // ------------------------------------------------------------------
      // this is the data we're parsing from the chombo file
      // ------------------------------------------------------------------
      vec3i rootGridSize;
      // std::vector<chombo::Level *> level;
      chombo::Chombo *chombo;

      // ------------------------------------------------------------------
      // this is the way we're passing over the data. for each input
      // box we create one data array (for the data values), and one
      // 'BrickInfo' descriptor that describes it. we then pass two
      // arrays - one array of all AMRBox'es, and one for all data
      // object handles. order of the brickinfos MUST correspond to
      // the order of the brickDataArrays; and the brickInfos MUST be
      // ordered from lowest level to highest level (inside each level
      // it doesn't matter)
      // ------------------------------------------------------------------
      struct BrickInfo {
        box3i box;
        int   level;
        float dt;

        vec3i size() const { return box.size() + vec3i(1); }
      };


      // ID of the data component we want to render (each brick can
      // contain multiple components)
      int componentID;
      Range<float> valueRange;
      std::vector<OSPData>   brickData;
      std::vector<BrickInfo> brickInfo;
      std::vector<float *>   brickPtrs;
      OSPData brickInfoData;
      OSPData brickDataData;
      OSPVolume ospVolume;
    };

  } // ::ospray::sg
} // ::ospray

