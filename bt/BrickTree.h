// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
#include "ospcommon/array3D/Array3D.h"
#include "ospcommon/box.h"
// ospcommon
#include "ospcommon/FileName.h"
#include "ospcommon/tasking/parallel_for.h"
// MPI
#include "mpiCommon/MPICommon.h"
// common
#include "common/helper.h"
// std
#include <iostream>
#include <vector>
#include <map>
#include <thread>
#include <list>
#include <set>
#include <mutex>
#include <stack>
#include <thread>
#include <algorithm>

#define STREAM_DATA 1

namespace ospray {
  namespace bt {

    using namespace ospcommon;

    template <typename T>
    const char *typeToString();

    static std::mutex brickLoadMtx;
    static const size_t numThread = 2;

    enum BRICKTYPE
    {
      INDEXBRICK,
      VALUEBRICK,
      BRICKINFO
    };

    struct LoadBricks
    {
      LoadBricks(BRICKTYPE btype, size_t offset)
          : bricktype(btype), brickID(offset){};
      BRICKTYPE bricktype;
      size_t brickID;  // N th of a specific brick
    };

    struct BrickStatus
    {
      BrickStatus()
      {
        isRequested = false;
        isLoaded    = false;
      }

      BrickStatus(bool request, bool load)
          : isRequested(request), isLoaded(load)
      {
      }

      bool isRequested;
      bool isLoaded;
    };

    template <int N, typename T = float>
    struct BrickTree
    {
      /*! 4x4x4 brick/brick of value. */
      struct ValueBrick
      {
        void clear();
        // Range<float> getValueRange() const;
        double computeWeightedAverage(  // coordinates of lower-left-front
                                        // voxel, in resp level
            const vec3i &brickCoord,
            // size of bricks in current level
            const int brickSize,
            // maximum size in finest level
            const vec3i &maxSize) const;

        T value[N][N][N];
      };

      /*! gives the _value_ brick ID of NxNxN children. if a
        nodes does NOT have a child, it will use ID "invalidID". if NO
        childID is valid, the brick shouldn't exist in the first place
        ... (see BrickInfo description) */
      struct IndexBrick
      {
        void clear();
        int32_t childID[N][N][N];
      };

      struct BrickInfo
      {
        BrickInfo(int32_t ID = invalidID()) : indexBrickID(ID){};

        /*! gives the ID of the index brick that encodes the children
          for the current brick. if the current brick doesn't HAVE any
          children, this will be (int32)-1 */
        int32_t indexBrickID;
      };

      size_t numValueBricks; /*8*/
      size_t numIndexBricks; /*8*/
      size_t numBrickInfos;  /*8*/

      size_t indexBricksOfs;  /*8*/
      size_t valueBricksOfs;  /*8*/
      size_t indexBrickOfOfs; /*8*/

      float avgValue;     /*4*/
      int32_t nBrickSize; /*4*/

      vec2f valueRange;   /*8*/
      vec3i validSize;    /*12*/
      vec3i rootGridDims; /*12*/

      ValueBrick *valueBrick         = nullptr; /*8*/
      IndexBrick *indexBrick         = nullptr; /*8*/
      BrickInfo *brickInfo           = nullptr; /*8*/
      BrickStatus *valueBricksStatus = nullptr; /*8*/

      BrickTree();
      ~BrickTree();

      static inline int invalidID()
      {
        return -1;
      }

      std::string voxelType() const
      {
        return typeToString<T>();
      };
      int brickSize() const
      {
        return N;
      };

      vec3i getRootGridDims() const
      {
        return rootGridDims;
      }

      /*! map this one from a binary dump that was created by the
       * bricktreebuilder/raw2bricks tool */
      void mapOSP(const FileName &brickFileBase, int treeID, vec3i treeCoord);
      void mapOspBin(const FileName &brickFileBase, size_t treeID);
      void loadBricks(FILE *file, vec2i vbListInfo);
      void loadBricks(FILE *file, LoadBricks aBrick);
      void loadTreeByBrick(const FileName &brickFileBase,
                           size_t treeID,
                           std::vector<vec2i> vbReqList);
      void loadTreeByBrick(const FileName &brickFileBase, size_t treeID);

      // const typename BrickTree<N,T>::ValueBrick * findValueBrick(const vec3i
      // &coord,int blockWidth,int xIdx, int yIdx, int zIdx);
      const T findValue(const int blockID, const vec3i &coord, int blockWidth);

      const T findBrickValue(const int blockID,
                             const size_t brickID,
                             const vec3i cellPos,
                             const size_t parentBrickID,
                             const vec3i parentCellPos);

      bool isTreeNeedLoad()
      {
        for (size_t i = 0; i < numValueBricks; i++) {
          if (!valueBricksStatus[i].isLoaded &&
              valueBricksStatus[i].isRequested) {
            return true;
          }
        }
        return false;
      }

      // get the request value brick list (L = need to load, F = no need to load)
      // L,L,F,F,F,L,L,L,F,L,F,F,L
      // return (0,2),(5,3)...
      std::vector<vec2i> getRequestVBList()
      {
        std::vector<vec2i> scheduledVB;
        std::stack<int> sIdxStack;
        for (int i = 0; i < (int)numValueBricks; i++) {
          if (!valueBricksStatus[i].isLoaded &&
              valueBricksStatus[i].isRequested) {
            if (sIdxStack.empty()) {
              sIdxStack.push(i);
            }
            if (i == (int)numValueBricks - 1) {
              scheduledVB.emplace_back(
                  vec2i(sIdxStack.top(), i - sIdxStack.top() + 1));
              sIdxStack.pop();
            }
          } else {
            if (!sIdxStack.empty()) {
              scheduledVB.emplace_back(
                  vec2i(sIdxStack.top(), i - sIdxStack.top()));
              sIdxStack.pop();
            }
          }
        }
        return scheduledVB;
      }
    };

    /* a entire *FOREST* of bricktrees */
    template <int N, typename T = float>
    struct BrickTreeForest
    {
      const vec3i forestSize;
      const vec3i originalVolumeSize;
      const FileName &brickFileBase;

      std::thread loadBrickTreeThread[numThread];

      box3f forestBounds;

      std::vector<BrickTree<N, T>> tree;

      void loadTreeBrick(const FileName &brickFileBase)
      {
        while (!tree.empty()) {
          for (size_t i = 0; i < tree.size(); i++) {
            //tree[i].loadTreeByBrick(brickFileBase, i);
            // if(i == 3)
            // {
            //   bool  isR= (bool)tree[i].valueBricksStatus[192412].isRequested;
            //   std::cout << "cpp  " << tree[i].valueBricksStatus[192412].isRequested << std::endl;
            // }

            bool needLoad = false;
            needLoad      = tree[i].isTreeNeedLoad();
            //PRINT(needLoad);
            if (needLoad)
            {
              tree[i].loadTreeByBrick(brickFileBase, i);
            }
          }
        }

        // while (!tree.empty()) {
        //   for (size_t i = 0; i < tree.size(); i++) {
        //     std::vector<vec2i> vbReqList = tree[i].getRequestVBList();
        //     if (!vbReqList.empty())
        //       tree[i].loadTreeByBrick(brickFileBase, i, vbReqList);
        //   }
        // }
      }

      void Initialize()
      {
        std::cout << "#osp: start to initialize bricktree forest!" << std::endl;
        int numTrees = forestSize.product();
        assert(numTrees > 0);

        tree.resize(numTrees);

        std::mutex amutex;
        tasking::parallel_for(numTrees, [&](int treeID) {
          vec3i treeCoord = vec3i(treeID % forestSize.x,
                                  (treeID / forestSize.x) % forestSize.y,
                                  treeID / (forestSize.x * forestSize.y));
          BrickTree<N, T> aTree;
          aTree.mapOSP(brickFileBase, treeID, treeCoord);
#if !(STREAM_DATA)
          aTree.mapOspBin(brickFileBase, treeID);
#endif
          std::lock_guard<std::mutex> lock(amutex);
          // tree.insert(std::make_pair(treeID, aTree));
          tree[treeID] = aTree;
        });

        printf("#osp: %d trees have initialized!\n", numTrees);
      }

      void loadBrickTreeForest()
      {
        // set another thread to load the binary data.(*.ospbin)
        for (size_t i = 0; i < numThread; i++) {
          loadBrickTreeThread[i] = std::thread(
              [&](const FileName &filebase) { loadTreeBrick(filebase); },
              this->brickFileBase);
          loadBrickTreeThread[i].detach();
        }
      }

      BrickTreeForest(const vec3i &forestSize,
                      const vec3i &originalVolumeSize,
                      const FileName &brickFileBase)
          : forestSize(forestSize),
            originalVolumeSize(originalVolumeSize),
            brickFileBase(brickFileBase)
      {
        Initialize();
#if STREAM_DATA
        loadBrickTreeForest();
#endif
      }

      ~BrickTreeForest()
      {
        tree.clear();
      }
    };

    // =======================================================
    // INLINE IMPLEMENTATION SECTION
    // =======================================================

    /*! print a value brick */
    template <int N, typename T>
    inline std::ostream &operator<<(
        std::ostream &o, const typename BrickTree<N, T>::ValueBrick &db)
    {
      for (int iz = 0; iz < N; iz++) {
        for (int iy = 0; iy < N; iy++) {
          if (N > 2)
            std::cout << std::endl;
          for (int ix = 0; ix < N; ix++)
            o << db.value[iz][iy][ix] << " ";
          o << "| ";
        }
        if (iz < 3)
          o << std::endl;
      }

      return o;
    }

    /*! print an index brick */
    template <int N, typename T>
    inline std::ostream &operator<<(
        std::ostream &o, const typename BrickTree<N, T>::IndexBrick &db)
    {
      for (int iz = 0; iz < N; iz++) {
        for (int iy = 0; iy < N; iy++) {
          if (N > 2)
            std::cout << std::endl;
          for (int ix = 0; ix < N; ix++)
            o << db.childID[iz][iy][ix] << " ";
          o << "| ";
        }
        if (iz < 3)
          o << std::endl;
      }

      return o;
    }

  }  // namespace bt
}  // namespace ospray
