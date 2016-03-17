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

#pragma once

// ospray
#include "Array3D.h"
// std
#include <iostream>

namespace ospray {
  namespace amr {

    /* octree of cell-centered nodes */
    struct Sumerian {
      static const uint32 invalidID = (uint32)-1;

      /*! 4x4x4 brick/block of data. */
      struct DataBlock {
        void clear();
        Range<float> getValueRange() const;
        float computeWeightedAverage(// coordinates of lower-left-front
                                     // voxel, in resp level
                                     const vec3i &brickCoord,
                                     // size of blocks in current level
                                     const int blockSize,
                                     // maximum size in finest level
                                     const vec3i &maxSize) const;

        float value[4][4][4];
      };
      
      /*! gives the _data_ block ID of the up to 64 children. if a
        nodes does NOT have a child, it will use ID "invalidID". if NO
        childID is valid, the block shouldn't exist in the first place
        ... (see BlockInfo description) */
      struct IndexBlock {
        void clear();

        uint32 childID[4][4][4];
      };
      struct BlockInfo {
        BlockInfo(uint32 ID=invalidID) : indexBlockID(ID) {};
        
        /*! gives the ID of the index block that encodes the children
          for the current block. if the current block doesn't HAVE any
          children, this will be (uint32)-1 */
        uint32 indexBlockID; 
      }; 

      struct Builder {
        virtual void set(const vec3i &coord, int level, float v) = 0;
        /*! be done with the build, and save all data to the xml/bin
            file of 'fileName' and 'filename+"bin"'. clipboxsize
            specifies which fraction of the encoded domain si actually
            useful. (1,1,1) means that everything is value (as in a
            native AMR data set), but encoding a 302^3 RAW volume into
            something that can, by definiion, only encode powers of 4,
            woudl result i na clip box size of ~vec3f(0.3) (=302/1024)
            : the tree itself encodes voxels up to 1024^3, but only
            that fraction of them iscatually valid. */
        virtual void save(const std::string &ospFileName, const vec3f &clipBoxSize) = 0;
      };

    };
    



    struct MemorySumBuilder : public Sumerian::Builder {
      MemorySumBuilder();
      virtual void set(const vec3i &coord, int level, float v);

      Sumerian::DataBlock *findDataBlock(const vec3i &coord, int level);

      uint32 newDataBlock();
      
      Sumerian::IndexBlock *getIndexBlockFor(uint32 dataBlockID);

      std::vector<uint32> indexBlockOf;
      std::vector<Sumerian::DataBlock *>  dataBlock;
      std::vector<Sumerian::IndexBlock *> indexBlock;

      /*! be done with the build, and save all data to the xml/bin
        file of 'fileName' and 'filename+"bin"' */
      virtual void save(const std::string &ospFileName, const vec3f &clipBoxSize);
    };

    /*! multi-sumerian - a root grid of cell, with one sumerian per
        cell */
    struct MultiSumBuilder : public Sumerian::Builder {
      MultiSumBuilder();
      MemorySumBuilder *getRootCell(const vec3i &rootCellID);
      void set(const vec3i &coord, int level, float v);
      void allocateAtLeast(const vec3i &neededSize);

      ActualArray3D<MemorySumBuilder *> *rootGrid;

      /*! be done with the build, and save all data to the xml/bin
        file of 'fileName' and 'filename+"bin"' */
      virtual void save(const std::string &ospFileName, const vec3f &clipBoxSize);
    };

  } // ::ospray::amr
} // ::ospray



