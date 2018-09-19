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

// common
#include "common/config.h"
#include "common/helper.h"
// ospray
#include "ospcommon/array3D/Array3D.h"
#include "ospcommon/box.h"
// ospcommon
#include "ospcommon/FileName.h"
#if __clang__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wundef"
#  pragma GCC diagnostic ignored "-Wduplicate-enum"
#  pragma GCC diagnostic ignored "-Winconsistent-missing-destructor-override"
#endif
#include "ospcommon/tasking/parallel_for.h"
#if __clang__
#  pragma GCC diagnostic pop
#endif
// MPI
#include "mpiCommon/MPICommon.h"
// std
#include <iostream>
#include <vector>
#include <map>
#include <thread>
#include <list>
#include <set>
#include <mutex>
#include <stack>
#include <queue>
#include <thread>
#include <algorithm>
#include <unordered_map>
#include <math.h>
#include <common/helper.h>

namespace ospray {
namespace bt
{

using namespace ospcommon;

template<typename T>
const char *typeToString();

static std::mutex brickLoadMtx;

static const size_t numThread = 5;

enum BRICKTYPE
{
  INDEXBRICK,
  VALUEBRICK,
  BRICKINFO
};

struct LoadBricks
{
  LoadBricks(BRICKTYPE btype, size_t offset)
    :
    bricktype(btype),
    brickID(offset)
  {
  }
  BRICKTYPE bricktype;
  size_t brickID;  // N th of a specific brick
};

struct BrickStatus
{
  BrickStatus()
    :
    isRequested(0),
    isLoaded(0),
    loadWeight(0.0)//,
    // valueRange(vec2f(std::numeric_limits<float>::infinity(),
    // 		     -std::numeric_limits<float>::infinity()))
  {
  }
  BrickStatus(int request, int load, float Weight/*, vec2f range*/)
    :
    isRequested(request),
    isLoaded(load),
    loadWeight(Weight)//,
    //valueRange(range)
  {
  }
  int8_t isRequested;
  int8_t isLoaded;
  float loadWeight;
  //vec2f valueRange;   /*8*/
};

template<int N, typename T>
struct BrickTree
{
  /*! 4x4x4 brick/brick of value. */
  struct ValueBrick
  {
    void clear();
    double computeWeightedAverage(
      // coordinates of lower-left-front
      // voxel, in resp level
      const vec3i &brickCoord,
      // size of bricks in current level
      const int brickSize,
      // maximum size in finest level
      const vec3i &maxSize) const;
    T value[N][N][N];
    T vRange[2];
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
    explicit BrickInfo(int32_t ID = invalidID())
      :
      indexBrickID(ID)
    {
    }

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
  int32_t depth;

  ValueBrick *valueBrick = nullptr; /*8*/
  IndexBrick *indexBrick = nullptr; /*8*/
  BrickInfo *brickInfo = nullptr; /*8*/
  BrickStatus *valueBricksStatus = nullptr; /*8*/

  size_t **vbIdxByLevelBuffers = nullptr;
  size_t *vbIdxByLevelStride = nullptr;

  BrickTree();
  ~BrickTree();

  static inline int invalidID()
  { return -1; }

  std::string voxelType() const
  {
    return typeToString<T>();
  }

  int brickSize() const
  { return N; }

  vec3i getRootGridDims() const
  { return rootGridDims; }

  /*! map this one from a binary dump that was created by the
   * bricktreebuilder/raw2bricks tool */
  void mapOSP(const FileName &brickFileBase, int treeID, vec3i treeCoord);
  void mapOspBin(const FileName &brickFileBase, size_t treeID);
  void loadBricks(FILE *file, vec2i vbListInfo);
  void loadBricks(FILE *file, LoadBricks aBrick);
  void loadTreeByBrick(const FileName &brickFileBase,
                       size_t treeID,
                       std::vector<int> vbList);
  void loadTreeByBrick(const FileName &brickFileBase,
                       size_t treeID,
                       std::vector<vec2i> vbReqList);
  void loadTreeByBrick(const FileName &brickFileBase, size_t treeID);

  const T findValue(const int blockID, const vec3i &coord, int blockWidth);
  const T findBrickValue(const int blockID,
                         const size_t brickID,
                         const vec3i cellPos,
                         const size_t parentBrickID,
                         const vec3i parentCellPos);

  std::vector<size_t> getValueBrickIDsByLevel(int level)
  {
    std::vector<size_t> vbIDs;
    if (level == 0) {
      vbIDs.emplace_back(0);
    }
    else {
      std::vector<size_t> pVBIDs = getValueBrickIDsByLevel(level - 1);
      for (size_t i = 0; i < pVBIDs.size(); i++) {
        const int ibID = brickInfo[pVBIDs[i]].indexBrickID;
        if (ibID != invalidID()) {
          auto ib = indexBrick[ibID];
          array3D::for_each(vec3i(N), [&](const vec3i &idx)
          {
            const int cvbID = ib.childID[idx.x][idx.y][idx.z];
            if (cvbID != invalidID()) {
              vbIDs.emplace_back(cvbID);
            }
          });
        }
      }
    }
    return vbIDs;
  }

  void reorganizeValueBrickBufferByLevel()
  {
    for (int i = 0; i < depth; i++) {
      std::vector<size_t> vbsByLevel = getValueBrickIDsByLevel(i);
      vbIdxByLevelStride[i] = vbsByLevel.size();
      *(vbIdxByLevelBuffers + i) = (size_t *) malloc(sizeof(size_t) * vbIdxByLevelStride[i]);
      std::copy(vbsByLevel.begin(), vbsByLevel.end(), *(vbIdxByLevelBuffers + i));
    }
  }

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

};

/* a entire *FOREST* of bricktrees */
template<int N, typename T = float>
struct BrickTreeForest
{

  const vec3i forestSize;
  const vec3i originalVolumeSize;
  const int depth;
  const FileName &brickFileBase;

  vec2f valueRange;

  std::thread loadBrickTreeThread[numThread];

  box3f forestBounds;

  std::vector<BrickTree<N, T>> tree;

  bool vbNeed2Load(size_t treeID, int vbID)
  {
    return (!tree[treeID].valueBricksStatus[vbID].isLoaded) &&
      tree[treeID].valueBricksStatus[vbID].isRequested;
  }

  // get the request value brick list
  //      (L = need to load, F = no need to load)
  // L,L,F,F,F,L,L,L,F,L,F,F,L
  // return (0,2),(5,3)...
  std::vector<vec2i> getReqVBs(BrickTree<N, T> &bt)
  {
    std::vector<vec2i> scheduledVB;
    std::stack<int> sIdxStack;
    for (int i = 0; i < (int) bt.numValueBricks; i++) {
      if (!bt.valueBricksStatus[i].isLoaded &&
        bt.valueBricksStatus[i].isRequested) {
        if (sIdxStack.empty()) {
          sIdxStack.push(i);
        }
        if (i == (int) bt.numValueBricks - 1) {
          scheduledVB.emplace_back(
            vec2i(sIdxStack.top(), i - sIdxStack.top() + 1));
          sIdxStack.pop();
        }
      }
      else {
        if (!sIdxStack.empty()) {
          scheduledVB.emplace_back(
            vec2i(sIdxStack.top(), i - sIdxStack.top()));
          sIdxStack.pop();
        }
      }
    }
    return scheduledVB;
  }

  void loadTreeBrick(const FileName &brickFileBase)
  {
    // while (!tree.empty()) {
    //   for (size_t i = 0; i < tree.size(); i++) {
    //     bool needLoad = false;
    //     needLoad      = tree[i].isTreeNeedLoad();
    //     if (needLoad)
    //     {
    //       tree[i].loadTreeByBrick(brickFileBase, i);
    //     }
    //   }
    // }

    //performance to be optimized
    while (!tree.empty()) {
      for (int i = 0; i < depth; i++) {
        tasking::parallel_for(tree.size(), [&](size_t treeID)
        {
          size_t *curLevelVBs = tree[treeID].vbIdxByLevelBuffers[i];
          size_t numVBs = tree[treeID].vbIdxByLevelStride[i];
          std::vector<int> reqVBs;
          for (size_t j = 0; j < numVBs; j++) {
            size_t vbIdx = curLevelVBs[j];
            if (!tree[treeID].valueBricksStatus[vbIdx].isLoaded &&
              tree[treeID].valueBricksStatus[vbIdx].isRequested)
              reqVBs.emplace_back(vbIdx);
          }
          tree[treeID].loadTreeByBrick(brickFileBase, treeID, reqVBs);
        });
      }
    }

    //while (!tree.empty()) {
    //   std::vector<std::vector<int>> loadVBList;
    //   loadVBList.resize(tree.size());

    //   for (int d = 0; d < depth; d++) {
    //     tasking::parallel_for(tree.size(), [&](size_t treeID) {
    //       int vbID = 0;
    //       if (d == 0) {
    //         if (!tree[treeID].valueBricksStatus[vbID].isLoaded &&
    //             tree[treeID].valueBricksStatus[vbID].isRequested)
    //           loadVBList[treeID].emplace_back(vbID);
    //       } else {
    //         std::vector<int> childVBList;
    //         for (size_t i = 0; i < loadVBList[treeID].size(); i++) {
    //           vbID           = loadVBList[treeID][i];
    //           const int ibID = tree[treeID].brickInfo[vbID].indexBrickID;
    //           if (ibID != -1) {
    //             auto &ib = tree[treeID].indexBrick[ibID];
    //             array3D::for_each(vec3i(N), [&](const vec3i &idx) {
    //               const unsigned int cvbID =
    //                   ib.childID[idx.x][idx.y][idx.z];
    //               if (cvbID != -1 &&
    //                   !tree[treeID].valueBricksStatus[cvbID].isLoaded) {
    //                 childVBList.emplace_back(cvbID);
    //               }
    //             });
    //           }
    //         }
    //         loadVBList[treeID].clear();
    //         loadVBList[treeID] = childVBList;
    //       }
    //       tree[treeID].loadTreeByBrick(
    //           brickFileBase, treeID, loadVBList[treeID]);
    //     });
    //   }
    // }

    // while (!tree.empty()) {
    //   for (size_t i = 0; i < tree.size(); i++) {
    //     std::vector<vec2i> vbReqList =getReqVBs(tree[i]);
    //     if (!vbReqList.empty())
    //       tree[i].loadTreeByBrick(brickFileBase, i, vbReqList);
    //   }
    // }
  }

  void Initialize()
  {
    ospray::time_point t1 = ospray::Time();
    int numTrees = forestSize.product();
    assert(numTrees > 0);

    tree.resize(numTrees);

    std::mutex amutex;
    tasking::parallel_for(numTrees, [&](int treeID)
    {
      vec3i treeCoord = vec3i(treeID % forestSize.x,
                              (treeID / forestSize.x) % forestSize.y,
                              treeID / (forestSize.x * forestSize.y));
      BrickTree<N, T> aTree;
      aTree.mapOSP(brickFileBase, treeID, treeCoord);

#if !(STREAM_DATA)
      aTree.mapOspBin(brickFileBase, treeID);
#endif
      tree[treeID] = aTree;
      valueRange.x = min(valueRange.x, aTree.valueRange.x);
      valueRange.y = max(valueRange.y, aTree.valueRange.y);
    });

    tasking::parallel_for(numTrees, [&](int treeID)
    {
      tree[treeID].reorganizeValueBrickBufferByLevel();
    });

    printf("#osp: %d trees have initialized! Timespan:%lf\n", numTrees, ospray::Time(t1));
  }

  void loadBrickTreeForest()
  {
    // set another thread to load the binary data.(*.ospbin)
    for (size_t i = 0; i < numThread; i++) {
      loadBrickTreeThread[i] = std::thread(
        [&](const FileName &filebase)
        { loadTreeBrick(filebase); },
        this->brickFileBase);
      loadBrickTreeThread[i].detach();
    }
  }

  BrickTreeForest(const vec3i &forestSize,
                  const vec3i &originalVolumeSize,
                  const int &depth,
                  const FileName &brickFileBase)
    : forestSize(forestSize),
      originalVolumeSize(originalVolumeSize),
      depth(depth),
      brickFileBase(brickFileBase),
      valueRange(vec2f(std::numeric_limits<float>::infinity(),
                       -std::numeric_limits<float>::infinity()))
  {
    Initialize();
    PRINT(valueRange);
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
template<int N, typename T>
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
template<int N, typename T>
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
