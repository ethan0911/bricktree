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
#include "MultiSumBuilder.h"

namespace ospray {
  namespace amr {

    using std::cout;
    using std::endl;
    using std::flush;

    inline int blockSizeOf(int level)
    { 
      int bs = 4;
      for (int i=0;i<level;i++) bs *= 4;
      return bs;
    }

    // -------------------------------------------------------
    // SINGLE sum builder
    // -------------------------------------------------------

    SingleSumBuilder::SingleSumBuilder()
    {
      newDataBlock();
    }

    int32 SingleSumBuilder::newDataBlock()
    {
      int32 ID = dataBlock.size();
      dataBlock.push_back(new Sumerian::DataBlock);
      dataBlock.back()->clear();
      indexBlockOf.push_back(Sumerian::invalidID);
      return ID;
    }

    /*! get index block for given data block ID - create if required */
    Sumerian::IndexBlock *SingleSumBuilder::getIndexBlockFor(int32 dataBlockID)
    {
      assert(dataBlockID < indexBlockOf.size());
      int32 indexBlockID = indexBlockOf[dataBlockID];
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
    void SingleSumBuilder::save(const std::string &ospFileName, const vec3f &clipBoxSize)
    {
      throw std::runtime_error("saving an individual sumerian not implemented yet (use multisum instead)");
    }

    /*! find or create data block that contains given cell. if that
        block (or any of its parents, indices referring to it, etc)
        does not exist, create and initialize whatever is required to
        have this node */
    Sumerian::DataBlock *SingleSumBuilder::findDataBlock(const vec3i &coord, int level)
    {
      // start with the root block
      int blockSize = blockSizeOf(level);

      assert(reduce_max(coord) < blockSize);
      int32 blockID = 0; 
      while (blockSize > 4) {
        int cellSize = blockSize / 4;

        assert(blockID < dataBlock.size());
        Sumerian::IndexBlock *ib = getIndexBlockFor(blockID);
        assert(ib);
        int cx = (coord.x % blockSize) / cellSize;
        int cy = (coord.y % blockSize) / cellSize;
        int cz = (coord.z % blockSize) / cellSize;
        assert(cx < 4);
        assert(cy < 4);
        assert(cz < 4);
        blockID = ib->childID[cz][cy][cx];
        assert(blockID == Sumerian::invalidID || blockID < dataBlock.size());
        if (blockID == Sumerian::invalidID) {
          blockID = ib->childID[cz][cy][cx] = newDataBlock();
        }
        assert(blockID < dataBlock.size());
        blockSize = cellSize;
      }
      
      assert(blockID < dataBlock.size());
      return dataBlock[blockID];
    }
    

    void SingleSumBuilder::set(const vec3i &coord, int level, float v)
    {
      assert(reduce_max(coord) < blockSizeOf(level));
      Sumerian::DataBlock *db = findDataBlock(coord, level);
      assert(db);
      db->value[coord.z % 4][coord.y % 4][coord.x % 4] = v;
    }


    // -------------------------------------------------------
    // MULTI sum builder
    // -------------------------------------------------------

    MultiSumBuilder::MultiSumBuilder() 
      : rootGrid(NULL), valueRange(empty), maxLevel(0)
    {}


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
      fprintf(osp,"   rootGrid=\"%i %i %i\"\n",
              rootGrid->size().x,rootGrid->size().y,rootGrid->size().z);
      fprintf(osp,"   voxelType=\"float\"\n");
      fprintf(osp,"   valueRange=\"%f %f\"\n",valueRange.lo,valueRange.hi);
      // fprintf(osp,"   samplingRate=\"%f\"\n",samplingRate);
      fprintf(osp,"   clipBoxSize=\"%f %f %f\"\n",
              clipBoxSize.x,clipBoxSize.y,clipBoxSize.z);
      fprintf(osp,"   maxLevel=\"%i\"\n",maxLevel);
      fprintf(osp,"   >\n");

      fprintf(osp,"\t<dataBlocks>\n");
      for (int iz=0;iz<rootGrid->size().z;iz++)
        for (int iy=0;iy<rootGrid->size().y;iy++)
          for (int ix=0;ix<rootGrid->size().x;ix++) {
            SingleSumBuilder *msb = getRootCell(vec3i(ix,iy,iz));
            if (!msb) {
              fprintf(osp," %i\n",0);
            } else {
              fprintf(osp,"\t\t%li\n",msb->dataBlock.size());
              for (int i=0;i<msb->dataBlock.size();i++) {
                fwrite(msb->dataBlock[i],sizeof(*msb->dataBlock[i]),1,bin);
              }
            }
          }
      fprintf(osp,"\t</dataBlocks>\n");
      
      fprintf(osp,"\t<indexBlocks>\n");
      size_t treeRootOffset = 0;
      for (int iz=0;iz<rootGrid->size().z;iz++)
        for (int iy=0;iy<rootGrid->size().y;iy++)
          for (int ix=0;ix<rootGrid->size().x;ix++) {
            SingleSumBuilder *msb = getRootCell(vec3i(ix,iy,iz));
            if (!msb) {
              fprintf(osp,"\t\t%i\n",0);
            } else {
              fprintf(osp,"\t\t%li\n",msb->indexBlock.size());
              for (int i=0;i<msb->indexBlock.size();i++) {
                Sumerian::IndexBlock ib = *msb->indexBlock[i];
                fwrite(&ib,sizeof(ib),1,bin);
              }
              treeRootOffset += msb->dataBlock.size();
            }
          }
      fprintf(osp,"\t</indexBlocks>\n");

      fprintf(osp,"\t<indexBlockOf>\n");
      for (int iz=0;iz<rootGrid->size().z;iz++)
        for (int iy=0;iy<rootGrid->size().y;iy++)
          for (int ix=0;ix<rootGrid->size().x;ix++) {
            SingleSumBuilder *msb = getRootCell(vec3i(ix,iy,iz));
            if (!msb) {
              fprintf(osp,"\t\t%i\n",0);
            } else {
              fprintf(osp,"\t\t%li\n",msb->indexBlockOf.size());
              fwrite(&msb->indexBlockOf[0],msb->indexBlockOf.size(),
                     sizeof(msb->indexBlockOf[0]),bin);
            }
          }
      fprintf(osp,"\t</indexBlockOf>\n");
      
      fprintf(osp,"</MultiSumAMR>\n");
      fprintf(osp,"</ospray>\n");
      fclose(osp);
      fclose(bin);
    }

    void MultiSumBuilder::set(const vec3i &coord, int level, float v)
    {
      // PING;
      int blockSize = blockSizeOf(level);
      assert(level >= 0);
      vec3i rootBlockID = coord / vec3i(blockSize);

      SingleSumBuilder *msb = getRootCell(rootBlockID);
      assert(msb);
      msb->set(coord % vec3i(blockSize), level, v);
      valueRange.extend(v);
      maxLevel = std::max(maxLevel,level);
    }

    void MultiSumBuilder::allocateAtLeast(const vec3i &_neededSize)
    {
      if (rootGrid == NULL) {
        rootGrid = new ActualArray3D<SingleSumBuilder *>(vec3i(1));
        rootGrid->set(vec3i(0,0,0),NULL);
      }
      const vec3i newSize = max(_neededSize,rootGrid->size());

      if (rootGrid->size().x >= newSize.x && 
          rootGrid->size().y >= newSize.y && 
          rootGrid->size().z >= newSize.z)
        return;

      ActualArray3D<SingleSumBuilder *> *newGrid
        = new ActualArray3D<SingleSumBuilder *>(newSize);
      
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
    
    SingleSumBuilder *MultiSumBuilder::getRootCell(const vec3i &rootCellID)
    {
      // if (rootCellID.x >= 17) PRINT(rootCellID);
      allocateAtLeast(rootCellID+vec3i(1));
      if (rootGrid->get(rootCellID) == NULL) {
        SingleSumBuilder *newBuilder = new SingleSumBuilder;
        rootGrid->set(rootCellID,newBuilder);
      }
      return rootGrid->get(rootCellID);
    }
    
  } // ::ospray::amr
} // ::ospray
