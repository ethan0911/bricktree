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
// amr base
#include "../amr/AMR.h"
#include "../amr/Sumerian.h"
	 
namespace ospray {
  namespace sg {
    
    /*! multi-octree amr node - for now only working for float format */
    struct AMRVolume : public sg::Volume {
      
      AMRVolume();
      
      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const
      { return "ospray::sg::AMRVolume (ospray_amr module)"; }

      /*! 'render' the nodes - all geometries, materials, etc will
          create their ospray counterparts, and store them in the
          node  */
      virtual void render(RenderContext &ctx);

      //! \brief Initialize this node's value from given XML node 
      virtual void setFromXML(const xml::Node *const node, 
                              const unsigned char *binBasePtr);

      //! return bounding box of all primitives
      virtual box3f getBounds();
      
      //! handle to the ospray volume object
      OSPVolume   ospVolume;

      //! type of voxel (uint8,float) that we have to deal with 
      OSPDataType voxelType;

      /*! data array for octree cells */
      OSPData octCellData;

      /*! data array for root cells */
      OSPData rootCellData;

      /*! the actual MOA we're handling */
      amr::AMR *moa;
    };


    /*! multi-octree amr node - for now only working for float format */
    struct MultiSumAMR : public sg::Volume {
      
      MultiSumAMR() : multiSum(NULL), ospVolume(NULL) {};
      
      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const
      { return "ospray::sg::MultiSumAMR (ospray_amr module)"; }

      /*! 'render' the nodes - all geometries, materials, etc will
          create their ospray counterparts, and store them in the
          node  */
      virtual void render(RenderContext &ctx);

      //! \brief Initialize this node's value from given XML node 
      virtual void setFromXML(const xml::Node *const node, 
                              const unsigned char *binBasePtr);

      //! return bounding box of all primitives
      virtual box3f getBounds() { return box3f(vec3f(0.f),clipBoxSize*vec3f(rootGridSize)); }
      
      //! handle to the ospray volume object
      OSPVolume   ospVolume;

      //! type of voxel (uint8,float) that we have to deal with 
      OSPDataType voxelType;
      
      vec3f clipBoxSize;
      vec3i rootGridSize;

      OSPData dataBlockData;
      OSPData indexBlockData;
      OSPData blockInfoData;
      OSPData rootCellData;

      amr::Sumerian *multiSum;
    };

  } // ::ospray::sg
} // ::ospray

