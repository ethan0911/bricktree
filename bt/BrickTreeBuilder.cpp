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
  namespace bt {

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

    // /*! be done with the build, and save all value to the xml/bin
    //   file of 'fileName' and 'filename+"bin"' */
    // template<int N, typename T>
    // void BrickTreeBuilder<N,T>::save(const std::string &ospFileName, const vec3f &clipBoxSize)
    // {
    //   throw std::runtime_error("saving an individual sumerian not implemented yet (use multisum instead)");
    // }

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

    template struct BrickTreeBuilder<2,uint8>;
    template struct BrickTreeBuilder<4,uint8>;
    template struct BrickTreeBuilder<8,uint8>;
    template struct BrickTreeBuilder<16,uint8>;
    template struct BrickTreeBuilder<32,uint8>;
    template struct BrickTreeBuilder<64,uint8>;

    template struct BrickTreeBuilder<2,float>;
    template struct BrickTreeBuilder<4,float>;
    template struct BrickTreeBuilder<8,float>;
    template struct BrickTreeBuilder<16,float>;
    template struct BrickTreeBuilder<32,float>;
    template struct BrickTreeBuilder<64,float>;

    template struct BrickTreeBuilder<2,double>;
    template struct BrickTreeBuilder<4,double>;
    template struct BrickTreeBuilder<8,double>;
    template struct BrickTreeBuilder<16,double>;
    template struct BrickTreeBuilder<32,double>;
    template struct BrickTreeBuilder<64,double>;

  } // ::ospray
}
