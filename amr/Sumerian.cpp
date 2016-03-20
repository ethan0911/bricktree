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
      uint32_t *firstBlockOfRootCell = new uint32_t[numRootCells];
      size_t sum = 0;
      for (int i=0;i<numRootCells;i++) {
        firstBlockOfRootCell[i] = sum;
        sum += numDataBlocksPerTree[i];
      }
      this->firstBlockOfRootCell = firstBlockOfRootCell;
      
      numDataBlocks = 0;
      for (int i=0;i<numDataBlocksPerTree.size();i++)
        numDataBlocks += numDataBlocksPerTree[i];
      numIndexBlocks = 0;
      for (int i=0;i<numIndexBlocksPerTree.size();i++)
        numIndexBlocks += numIndexBlocksPerTree[i];
      
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
            den += weight;
            num += weight * value[iz][iy][ix];
          }

      assert(den != 0.f);
      return num / den;
    }

    inline int blockSizeOf(int level)
    { 
      int bs = 4;
      for (int i=0;i<level;i++) bs *= 4;
      return bs;
    }

    MemorySumBuilder::MemorySumBuilder()
    {
      newDataBlock();
    }

    uint32 MemorySumBuilder::newDataBlock()
    {
      uint32 ID = dataBlock.size();
      dataBlock.push_back(new Sumerian::DataBlock);
      dataBlock.back()->clear();
      indexBlockOf.push_back(Sumerian::invalidID);
      return ID;
    }

    Sumerian::IndexBlock *MemorySumBuilder::getIndexBlockFor(uint32 dataBlockID)
    {
      assert(dataBlockID < indexBlockOf.size());
      uint32 indexBlockID = indexBlockOf[dataBlockID];
      if (indexBlockID == Sumerian::invalidID) {
        indexBlockID = indexBlockOf[dataBlockID] = indexBlock.size();
        indexBlock.push_back(new Sumerian::IndexBlock);
        indexBlock.back()->clear();
      }
      
      for (int iz=0;iz<4;iz++)
        for (int iy=0;iy<4;iy++)
          for (int ix=0;ix<4;ix++)
            assert(indexBlock[indexBlockID]->childID[iz][iy][ix] == Sumerian::invalidID ||
                   indexBlock[indexBlockID]->childID[iz][iy][ix] < dataBlock.size());
      return indexBlock[indexBlockID];
    }




    /*! be done with the build, and save all data to the xml/bin
      file of 'fileName' and 'filename+"bin"' */
    void MemorySumBuilder::save(const std::string &ospFileName, const vec3f &clipBoxSize)
    {
      throw std::runtime_error("saving an individual sumerian not implemented yet (use multisum instead)");
    }

    /*! be done with the build, and save all data to the xml/bin
      file of 'fileName' and 'filename+"bin"' */
    void MultiSumBuilder::save(const std::string &ospFileName, const vec3f &clipBoxSize)
    {
      FILE *osp = fopen(ospFileName.c_str(),"w");
      assert(osp);
      std::string binFileName = ospFileName+"bin";
      FILE *bin = fopen(binFileName.c_str(),"wb");
      assert(bin);
      
      fprintf(osp,"<?xml?>\n");
      fprintf(osp,"<ospray>\n");
      fprintf(osp,"<MultiSumAMR\n");
      fprintf(osp,"   rootGrid=\"%i %i %i\"\n",rootGrid->size().x,rootGrid->size().y,rootGrid->size().z);
      fprintf(osp,"   voxelType=\"float\"\n");
      fprintf(osp,"   clipBoxSize=\"%f %f %f\"\n",clipBoxSize.x,clipBoxSize.y,clipBoxSize.z);
      fprintf(osp,"   >\n");

      fprintf(osp,"\t<dataBlocks>\n");
      for (int iz=0;iz<rootGrid->size().z;iz++)
        for (int iy=0;iy<rootGrid->size().y;iy++)
          for (int ix=0;ix<rootGrid->size().x;ix++) {
            MemorySumBuilder *msb = getRootCell(vec3i(ix,iy,iz));
            if (!msb) {
              fprintf(osp," %i\n",0);
            } else {
              if (msb->dataBlock.size())
                std::cout << "data block " << vec3i(ix,iy,iz) << " = " << *msb->dataBlock[0] << std::endl;
              fprintf(osp,"\t\t%li\n",msb->dataBlock.size());
              for (int i=0;i<msb->dataBlock.size();i++) {
                fwrite(msb->dataBlock[i],sizeof(*msb->dataBlock[i]),1,bin);
              }
            }
          }
      fprintf(osp,"\t</dataBlocks>\n");
      
      fprintf(osp,"\t<indexBlocks>\n");
      for (int iz=0;iz<rootGrid->size().z;iz++)
        for (int iy=0;iy<rootGrid->size().y;iy++)
          for (int ix=0;ix<rootGrid->size().x;ix++) {
            MemorySumBuilder *msb = getRootCell(vec3i(ix,iy,iz));
            if (!msb) {
              fprintf(osp,"\t\t%i\n",0);
            } else {
              fprintf(osp,"\t\t%li\n",msb->indexBlock.size());
              for (int i=0;i<msb->indexBlock.size();i++) {
                fwrite(msb->indexBlock[i],sizeof(*msb->indexBlock[i]),1,bin);
              }
            }
          }
      fprintf(osp,"\t</indexBlocks>\n");

      fprintf(osp,"\t<indexBlockOf>\n");
      for (int iz=0;iz<rootGrid->size().z;iz++)
        for (int iy=0;iy<rootGrid->size().y;iy++)
          for (int ix=0;ix<rootGrid->size().x;ix++) {
            MemorySumBuilder *msb = getRootCell(vec3i(ix,iy,iz));
            if (!msb) {
              fprintf(osp,"\t\t%i\n",0);
            } else {
              fprintf(osp,"\t\t%li\n",msb->indexBlockOf.size());
              fwrite(&msb->indexBlockOf[0],msb->indexBlockOf.size(),sizeof(msb->indexBlockOf[0]),bin);
            }
          }
      fprintf(osp,"\t</indexBlockOf>\n");
      
      fprintf(osp,"</MultiSumAMR>\n");
      fprintf(osp,"</ospray>\n");
      fclose(osp);
      fclose(bin);
    }



    Sumerian::DataBlock *MemorySumBuilder::findDataBlock(const vec3i &coord, int level)
    {
      // cout << "." << flush;
      // PING;
      // PRINT(coord);
      // PRINT(level);

      // start with the root block
      int blockSize = blockSizeOf(level);

      // PRINT(blockSize);

      assert(reduce_max(coord) < blockSize);
      uint32 blockID = 0; 
      while (blockSize > 4) {
        int cellSize = blockSize / 4;
        // PRINT(blockSize);
        // PRINT(cellSize);
        // PRINT(blockID);

        assert(blockID < dataBlock.size());
        Sumerian::IndexBlock *ib = getIndexBlockFor(blockID);
        assert(ib);
        int cx = (coord.x % blockSize) / cellSize;
        int cy = (coord.y % blockSize) / cellSize;
        int cz = (coord.z % blockSize) / cellSize;
        // PRINT(vec3i(cx,cy,cz));
        assert(cx < 4);
        assert(cy < 4);
        assert(cz < 4);
        blockID = ib->childID[cz][cy][cx];
        assert(blockID == Sumerian::invalidID || blockID < dataBlock.size());
        if (blockID == Sumerian::invalidID) {
          blockID = ib->childID[cz][cy][cx] = newDataBlock();
          // std::cout << "allocating block " << coord
          //           << " bs " << blockSize << " local " << vec3i(cx,cy,cz)
          //           << " level " << level
          //           << " " << this << std::endl;
        }
        assert(blockID < dataBlock.size());
        blockSize = cellSize;
      }
      
      assert(blockID < dataBlock.size());
      // cout << "/" << flush;
      return dataBlock[blockID];
    }
    
    void MemorySumBuilder::set(const vec3i &coord, int level, float v)
    {
      // PING;
      assert(reduce_max(coord) < blockSizeOf(level));
      Sumerian::DataBlock *db = findDataBlock(coord, level);
      assert(db);
      db->value[coord.z % 4][coord.y % 4][coord.x % 4] = v;
    }

    void MultiSumBuilder::set(const vec3i &coord, int level, float v)
    {
      // PING;
      int blockSize = blockSizeOf(level);
      assert(level >= 0);
      vec3i rootBlockID = coord / vec3i(blockSize);

      MemorySumBuilder *msb = getRootCell(rootBlockID);
      assert(msb);
      msb->set(coord % vec3i(blockSize), level, v);
    }

    MultiSumBuilder::MultiSumBuilder() 
      : rootGrid(NULL)
    {}
    
    void MultiSumBuilder::allocateAtLeast(const vec3i &_neededSize)
    {
      if (rootGrid == NULL) {
        rootGrid = new ActualArray3D<MemorySumBuilder *>(vec3i(1));
        rootGrid->set(vec3i(0,0,0),NULL);
      }
      const vec3i newSize = max(_neededSize,rootGrid->size());

      if (rootGrid->size().x >= newSize.x && 
          rootGrid->size().y >= newSize.y && 
          rootGrid->size().z >= newSize.z)
        return;

      ActualArray3D<MemorySumBuilder *> *newGrid
        = new ActualArray3D<MemorySumBuilder *>(newSize);
      
      for (int iz=0;iz<newGrid->size().z;iz++)
        for (int iy=0;iy<newGrid->size().y;iy++)
          for (int ix=0;ix<newGrid->size().x;ix++) {
            if (ix < rootGrid->size().x &&
                iy < rootGrid->size().y &&
                iz < rootGrid->size().z)
              newGrid->set(vec3i(ix,iy,iz),rootGrid->get(vec3i(ix,iy,iz)));
            else
              newGrid->set(vec3i(ix,iy,iz),NULL);
          }

      delete rootGrid;
      rootGrid = newGrid;
    }
    
    MemorySumBuilder *MultiSumBuilder::getRootCell(const vec3i &rootCellID)
    {
      allocateAtLeast(rootCellID+vec3i(1));
      if (rootGrid->get(rootCellID) == NULL) {
        MemorySumBuilder *newBuilder = new MemorySumBuilder;
        rootGrid->set(rootCellID,newBuilder);
      }
      return rootGrid->get(rootCellID);
    }
    
  } // ::ospray::amr
} // ::ospray
