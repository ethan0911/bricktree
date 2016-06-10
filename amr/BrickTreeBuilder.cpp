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
#include "BrickTreeBuilder.h"

namespace ospray {
  namespace amr {

    using std::cout;
    using std::endl;
    using std::flush;

    template<int N>
    inline int brickSizeOf(int level)
    { 
      int bs = N;
      for (int i=0;i<level;i++) bs *= N;
      return bs;
    }

    // -------------------------------------------------------
    // SINGLE sum builder
    // -------------------------------------------------------

    template<int N, typename T>
    BrickTreeBuilder<N,T>::BrickTreeBuilder()
      : maxLevel(0)
    {
      newValueBrick();
    }

    template<int N, typename T>
    BrickTreeBuilder<N,T>::~BrickTreeBuilder()
    {

    }

    template<int N, typename T>
    int32 BrickTreeBuilder<N,T>::newValueBrick()
    {
      int32 ID = valueBrick.size();
      valueBrick.push_back(new typename BrickTree<N,T>::ValueBrick);
      valueBrick.back()->clear();
      indexBrickOf.push_back(BrickTree<N,T>::invalidID());
      return ID;
    }

    /*! get index brick for given value brick ID - create if required */
    template<int N, typename T>
    typename BrickTree<N,T>::IndexBrick *
    BrickTreeBuilder<N,T>::getIndexBrickFor(int32 valueBrickID)
    {
      assert(valueBrickID < indexBrickOf.size());
      int32 indexBrickID = indexBrickOf[valueBrickID];
      if (indexBrickID == BrickTree<N,T>::invalidID()) {
        indexBrickID = indexBrickOf[valueBrickID] = indexBrick.size();
        indexBrick.push_back(new typename BrickTree<N,T>::IndexBrick);
        indexBrick.back()->clear();
      }
      
      for (int iz=0;iz<N;iz++)
        for (int iy=0;iy<N;iy++)
          for (int ix=0;ix<N;ix++) {
            assert(indexBrick[indexBrickID] != NULL);
            assert(indexBrick[indexBrickID]->childID[iz][iy][ix] == -1 ||
                   indexBrick[indexBrickID]->childID[iz][iy][ix] < valueBrick.size());
          }
      return indexBrick[indexBrickID];
    }

    /*! be done with the build, and save all value to the xml/bin
      file of 'fileName' and 'filename+"bin"' */
    template<int N, typename T>
    void BrickTreeBuilder<N,T>::save(const std::string &ospFileName, const vec3f &clipBoxSize)
    {
      throw std::runtime_error("saving an individual sumerian not implemented yet (use multisum instead)");
    }

    /*! find or create value brick that contains given cell. if that
      brick (or any of its parents, indices referring to it, etc)
      does not exist, create and initialize whatever is required to
      have this node */
    template<int N, typename T>
    typename BrickTree<N,T>::ValueBrick *
    BrickTreeBuilder<N,T>::findValueBrick(const vec3i &coord, int level)
    {
      // start with the root brick
      int brickSize = brickSizeOf<N>(level);

      assert(reduce_max(coord) < brickSize);
      int32 brickID = 0; 
      while (brickSize > N) {
        int cellSize = brickSize / N;

        assert(brickID < valueBrick.size());
        typename BrickTree<N,T>::IndexBrick *ib = getIndexBrickFor(brickID);
        assert(ib);
        int cx = (coord.x % brickSize) / cellSize;
        int cy = (coord.y % brickSize) / cellSize;
        int cz = (coord.z % brickSize) / cellSize;
        assert(cx < N);
        assert(cy < N);
        assert(cz < N);
        brickID = ib->childID[cz][cy][cx];
        assert(brickID == -1 || brickID < valueBrick.size());
        if (brickID == BrickTree<N,T>::invalidID()) {
          brickID = ib->childID[cz][cy][cx] = newValueBrick();
        }
        assert(brickID < valueBrick.size());
        brickSize = cellSize;
      }
      
      assert(brickID < valueBrick.size());
      return valueBrick[brickID];
    }
    

    template<int N, typename T>
    void BrickTreeBuilder<N,T>::set(const vec3i &coord, int level, float v)
    {
#ifdef PARALLEL_MULTI_TREE_BUILD
      std::lock_guard<std::mutex> lock(mutex);
#endif
      maxLevel = max(maxLevel,level);
      assert(reduce_max(coord) < brickSizeOf<N>(level));
      typename BrickTree<N,T>::ValueBrick *db = this->findValueBrick(coord, level);
      assert(db);
      db->value[coord.z % N][coord.y % N][coord.x % N] = v;
    }


    // -------------------------------------------------------
    // MULTI sum builder
    // -------------------------------------------------------

    template<int N, typename T>
    MultiBrickTreeBuilder<N,T>::MultiBrickTreeBuilder() 
      : rootGrid(NULL), valueRange(empty)
    {}

    /*! be done with the build, and save all value to the xml/bin
      file of 'fileName' and 'filename+"bin"' */
    template<int N, typename T>
    void MultiBrickTreeBuilder<N,T>::save(const std::string &ospFileName, const vec3f &clipBoxSize)
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
      fprintf(osp,"   voxelType=\"%s\"\n",typeToString<T>());
      fprintf(osp,"   blockSize=\"%i\"\n",N);
      fprintf(osp,"   valueRange=\"%f %f\"\n",(float)valueRange.lo,(float)valueRange.hi);
      // fprintf(osp,"   samplingRate=\"%f\"\n",samplingRate);
      fprintf(osp,"   clipBoxSize=\"%f %f %f\"\n",
              clipBoxSize.x,clipBoxSize.y,clipBoxSize.z);


      int maxLevel = 0;
      for (int iz=0;iz<rootGrid->size().z;iz++)
        for (int iy=0;iy<rootGrid->size().y;iy++)
          for (int ix=0;ix<rootGrid->size().x;ix++) {
            BrickTreeBuilder<N,T> *msb = getRootCell(vec3i(ix,iy,iz));
            if (msb)
              maxLevel = max(maxLevel,msb->maxLevel);
          }
      fprintf(osp,"   maxLevel=\"%i\"\n",maxLevel);
      fprintf(osp,"   >\n");

      fprintf(osp,"\t<valueBricks>\n");
      for (int iz=0;iz<rootGrid->size().z;iz++)
        for (int iy=0;iy<rootGrid->size().y;iy++)
          for (int ix=0;ix<rootGrid->size().x;ix++) {
            BrickTreeBuilder<N,T> *msb = getRootCell(vec3i(ix,iy,iz));
            if (!msb) {
              fprintf(osp," %i\n",0);
            } else {
              fprintf(osp,"\t\t%li\n",msb->valueBrick.size());
              for (int i=0;i<msb->valueBrick.size();i++) {
                fwrite(msb->valueBrick[i],sizeof(*msb->valueBrick[i]),1,bin);
              }
            }
          }
      fprintf(osp,"\t</valueBricks>\n");
      
      fprintf(osp,"\t<indexBricks>\n");
      size_t treeRootOffset = 0;
      for (int iz=0;iz<rootGrid->size().z;iz++)
        for (int iy=0;iy<rootGrid->size().y;iy++)
          for (int ix=0;ix<rootGrid->size().x;ix++) {
            BrickTreeBuilder<N,T> *msb = getRootCell(vec3i(ix,iy,iz));
            if (!msb) {
              fprintf(osp,"\t\t%i\n",0);
            } else {
              fprintf(osp,"\t\t%li\n",msb->indexBrick.size());
              for (int i=0;i<msb->indexBrick.size();i++) {
                typename BrickTree<N,T>::IndexBrick ib = *msb->indexBrick[i];
                fwrite(&ib,sizeof(ib),1,bin);
              }
              treeRootOffset += msb->valueBrick.size();
            }
          }
      fprintf(osp,"\t</indexBricks>\n");

      fprintf(osp,"\t<indexBrickOf>\n");
      for (int iz=0;iz<rootGrid->size().z;iz++)
        for (int iy=0;iy<rootGrid->size().y;iy++)
          for (int ix=0;ix<rootGrid->size().x;ix++) {
            BrickTreeBuilder<N,T> *msb = getRootCell(vec3i(ix,iy,iz));
            if (!msb) {
              fprintf(osp,"\t\t%i\n",0);
            } else {
              fprintf(osp,"\t\t%li\n",msb->indexBrickOf.size());
              fwrite(&msb->indexBrickOf[0],msb->indexBrickOf.size(),
                     sizeof(msb->indexBrickOf[0]),bin);
            }
          }
      fprintf(osp,"\t</indexBrickOf>\n");
      
      fprintf(osp,"</MultiSumAMR>\n");
      fprintf(osp,"</ospray>\n");
      fclose(osp);
      fclose(bin);
    }

    template<int N, typename T>
    void MultiBrickTreeBuilder<N,T>::set(const vec3i &coord, int level, float v)
    {
      // PING;
      int brickSize = brickSizeOf<N>(level);
      assert(level >= 0);
      vec3i rootBrickID = coord / vec3i(brickSize);

      BrickTreeBuilder<N,T> *msb = getRootCell(rootBrickID);
      assert(msb);
      msb->set(coord % vec3i(brickSize), level, v);
      valueRange.extend(v);
    }

    template<int N, typename T>
    void MultiBrickTreeBuilder<N,T>::allocateAtLeast(const vec3i &_neededSize)
    {
      if (rootGrid == NULL) {
        rootGrid = new ActualArray3D<BrickTreeBuilder<N,T> *>(vec3i(1));
        rootGrid->set(vec3i(0,0,0),NULL);
      }
      const vec3i newSize = max(_neededSize,rootGrid->size());

      if (rootGrid->size().x >= newSize.x && 
          rootGrid->size().y >= newSize.y && 
          rootGrid->size().z >= newSize.z)
        return;

      ActualArray3D<BrickTreeBuilder<N,T> *> *newGrid
        = new ActualArray3D<BrickTreeBuilder<N,T> *>(newSize);
      
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
    
    template<int N, typename T>
    BrickTreeBuilder<N,T> *
    MultiBrickTreeBuilder<N,T>::getRootCell(const vec3i &rootCellID)
    {
#ifdef PARALLEL_MULTI_TREE_BUILD
      std::lock_guard<std::mutex> lock(rootGridMutex);
#endif
      allocateAtLeast(rootCellID+vec3i(1));
      if (rootGrid->get(rootCellID) == NULL) {
        BrickTreeBuilder<N,T> *newBuilder = new BrickTreeBuilder<N,T>;
        rootGrid->set(rootCellID,newBuilder);
      }
      return rootGrid->get(rootCellID);
    }

    template struct MultiBrickTreeBuilder<2,uint8>;
    template struct MultiBrickTreeBuilder<4,uint8>;
    template struct MultiBrickTreeBuilder<8,uint8>;
    template struct MultiBrickTreeBuilder<16,uint8>;
    template struct MultiBrickTreeBuilder<32,uint8>;
    template struct MultiBrickTreeBuilder<64,uint8>;

    template struct MultiBrickTreeBuilder<2,float>;
    template struct MultiBrickTreeBuilder<4,float>;
    template struct MultiBrickTreeBuilder<8,float>;
    template struct MultiBrickTreeBuilder<16,float>;
    template struct MultiBrickTreeBuilder<32,float>;
    template struct MultiBrickTreeBuilder<64,float>;

    template struct MultiBrickTreeBuilder<2,double>;
    template struct MultiBrickTreeBuilder<4,double>;
    template struct MultiBrickTreeBuilder<8,double>;
    template struct MultiBrickTreeBuilder<16,double>;
    template struct MultiBrickTreeBuilder<32,double>;
    template struct MultiBrickTreeBuilder<64,double>;
    
  } // ::ospray::amr
} // ::ospray
