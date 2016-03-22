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

// own
#include "Sumerian.h"

namespace ospray {
  namespace amr {

    using std::cout;
    using std::endl;
    using std::flush;

    const uint32 Sumerian::invalidID;

    void Sumerian::DataBlock::clear()
    {
      for (int iz=0;iz<4;iz++)
        for (int iy=0;iy<4;iy++)
          for (int ix=0;ix<4;ix++)
            value[iz][iy][ix] = 0.f;
    }

    void Sumerian::IndexBlock::clear()
    {
      for (int iz=0;iz<4;iz++)
        for (int iy=0;iy<4;iy++)
          for (int ix=0;ix<4;ix++)
            childID[iz][iy][ix] = invalidID;
    }

    Range<float> Sumerian::DataBlock::getValueRange() const
    {
      Range<float> r = empty;
      for (int iz=0;iz<4;iz++)
        for (int iy=0;iy<4;iy++)
          for (int ix=0;ix<4;ix++)
            r.extend(value[iz][iy][ix]);
      return r;
    }

    void Sumerian::mapFrom(const unsigned char *ptr, 
                           const vec3i &rootGrid,
                           const vec3f &fracOfRootGrid,
                           std::vector<int32_t> numDataBlocksPerTree,
                           std::vector<int32_t> numIndexBlocksPerTree)
    {
      this->rootGridDims = rootGrid;
      PRINT(rootGridDims);
      this->validFractionOfRootGrid = fracOfRootGrid;

      const size_t numRootCells = rootGridDims.product();
      uint32_t *firstIndexBlockOfTree = new uint32_t[numRootCells];
      uint32_t *firstDataBlockOfTree = new uint32_t[numRootCells];
      size_t sumIndex = 0;
      size_t sumData = 0;
      for (int i=0;i<numRootCells;i++) {
        firstIndexBlockOfTree[i] = sumIndex;
        firstDataBlockOfTree[i] = sumData;
        sumData += numDataBlocksPerTree[i];
        sumIndex += numIndexBlocksPerTree[i];
      }
      this->firstIndexBlockOfTree = firstIndexBlockOfTree;
      this->firstDataBlockOfTree = firstDataBlockOfTree;
      
      numDataBlocks = sumData;
      numIndexBlocks = sumIndex;
      
      dataBlock = (const DataBlock *)ptr;
      ptr += numDataBlocks * sizeof(DataBlock);
      
      indexBlock = (const IndexBlock *)ptr;
      ptr += numIndexBlocks * sizeof(IndexBlock);
      
      blockInfo = (const BlockInfo *)ptr;
    }      

    float Sumerian::DataBlock::computeWeightedAverage(// coordinates of lower-left-front
                                                      // voxel, in resp level
                                                      const vec3i &brickCoord,
                                                      // size of blocks in current level
                                                      const int blockSize,
                                                      // maximum size in finest level
                                                      const vec3i &maxSize) const
    {
      int cellSize = blockSize / 4;

      float num = 0.f, den = 0.f;

      for (int iz=0;iz<4;iz++)
        for (int iy=0;iy<4;iy++)
          for (int ix=0;ix<4;ix++) {
            vec3i cellBegin = brickCoord*vec3i(blockSize)+vec3i(ix,iy,iz)*vec3i(cellSize);
            vec3i cellEnd = min(cellBegin + vec3i(cellSize),maxSize);

            float weight = reduce_mul(cellEnd-cellBegin);
            if (weight > 0.f) {
              den += weight;
              num += weight * value[iz][iy][ix];
            }
          }

      assert(den != 0.f);
      return num / (den+1e-8f);
    }

  } // ::ospray::amr
} // ::ospray
