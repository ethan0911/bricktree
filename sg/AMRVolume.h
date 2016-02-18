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
	 
namespace ospray {
  namespace sg {
    
    struct AMRVolume : public sg::Volume {
      union {
        amr::AMR<uint8> *uint8AMR;
        amr::AMR<float> *floatAMR;
      };
      
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

      //! dimensions of ORIGINAL volume (e.g., if everything was on finest level)
      vec3i dimensions;

      //! size of the data, in bytes, as reported by the bin file - needed to declare proper data object
      size_t dataSize;

      //! pointer to the data, as specifid in data file
      void *dataPtr;

      template<typename T>
      void readAMR(OSPVolume volume, const char *voxelType,
                   unsigned char *dataMem);
      // /*! data array that contains all grid descriptions */
      // OSPData     ospGridData;
      // /*! data array that contains pointers to voxel data arrays */
      // OSPData     ospVoxelDataData;
      // /*! data array that contains pointers to cell data arrays */
      // OSPData     ospCellDataData;
    };

  } // ::ospray::sg
} // ::ospray

