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

namespace ospray {
  namespace bt {

    using namespace ospcommon;

    template <typename T>
    const char *typeToString();

    static std::mutex brickLoadMtx;
    static const size_t numThread = 1;

    enum BRICKTYPE {INDEXBRICK, VALUEBRICK, BRICKINFO};

    struct LoadBricks
    {
      LoadBricks(BRICKTYPE btype, size_t offset):bricktype(btype),brickID(offset){};
      BRICKTYPE bricktype;
      size_t brickID; //N th of a specific brick
    };

    struct BrickStatus
    {
      BrickStatus(){
        isRequested = false;
        isLoaded = false;
      }

      BrickStatus(bool request, bool load): isRequested(request), isLoaded(load){}
      
      bool isRequested;
      bool isLoaded;
    };

    /*! the superblock we'll store at the end of the binary
    memory/file region for a binary bricktree */
    struct BinaryFileSuperBlock
    {
      size_t indexBricksOfs;
      size_t valueBricksOfs;
      size_t indexBrickOfOfs;
    };

    struct BrickTreeBase {
      virtual std::string voxelType() const = 0;
      virtual int         brickSize() const = 0;
      // virtual const void *brickInfoPtr()  const = 0;
      // virtual const void *valueBrickPtr()  const = 0;
      // virtual const void *indexBrickPtr() const = 0;
      virtual vec3i getRootGridDims() const = 0;
      // virtual const int *firstIndexBrickOfTreePtr() const = 0;
      // virtual const int *firstValueBrickOfTreePtr() const = 0;

      static BrickTreeBase *mapFrom(const void *ptr, 
                                    const std::string &format, 
                                    int BS, 
                                    size_t superBlockOfs);
      // virtual void saveTo(FILE *bin, size_t &superBlockOfs) = 0;

      size_t numValueBricks;
      size_t numIndexBricks;
      size_t numBrickInfos;
      BinaryFileSuperBlock binBlockInfo;

      float avgValue;
      vec2f valueRange;
      int nBrickSize;
      vec3i validSize;
      vec3i rootGridDims;
      vec3f validFractionOfRootGrid;

      FileName brickFileBase;
    };

    /* hierarchical tree of bricks. if N is the template parameter,
       this tree will encode bricks of NxNxN cells (a so-called value
       brick). child pointers are encoded in 'index bricks', with each
       value brick possibly having one associated index brick. For each
       value brick, we store as 'indexBrickOf' value (in a separate
       array); if this value is 'invalidID' the given value brick is a
       complete leaf brick that doesn't ahve an index brick (and thus,
       no children at all); otherwise this ID refers to a index brick
       in the index brick array, with the index brick having exactly
       one child index value (referring to value brick IDs) for each
       cell (though it is possible that some (or even, many) cells do
       not have a child at all, in which the corresponding child index
       in the index brick is 'invalidID'  */
    template<int N, typename T=float>
    struct BrickTree : public BrickTreeBase {
      
      BrickTree();
      ~BrickTree();

      static inline int invalidID() { return -1; }

      virtual std::string voxelType() const override { return typeToString<T>(); };
      virtual int         brickSize() const override { return N; };
      
      /*! 4x4x4 brick/brick of value. */
      struct ValueBrick {
        void clear();
        // Range<float> getValueRange() const;
        double computeWeightedAverage(// coordinates of lower-left-front
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
      struct IndexBrick {
        void clear(); 
        int32_t childID[N][N][N];
      };
      struct BrickInfo {
        BrickInfo(int32_t ID=invalidID()) : indexBrickID(ID) {};
        
        /*! gives the ID of the index brick that encodes the children
          for the current brick. if the current brick doesn't HAVE any
          children, this will be (int32)-1 */
        int32_t indexBrickID; 
      }; 

      // void mapFrom(const unsigned char *ptr, 
      //              const vec3i &rootGrid,
      //              const vec3f &fracOfRootGrid,
      //              std::vector<int32_t> numValueBricksPerTree,
      //              std::vector<int32_t> numIndexBricksPerTree);

      // virtual const void *brickInfoPtr()  const override { return brickInfo; }
      // virtual const void *valueBrickPtr()  const override { return valueBrick; }
      // virtual const void *indexBrickPtr() const override { return indexBrick; }

      ValueBrick *valueBrick;
      IndexBrick *indexBrick;
      BrickInfo  *brickInfo;
      
      virtual vec3i getRootGridDims() const override { return rootGridDims; }

      /*! map this one from a binary dump that was created by the bricktreebuilder/raw2bricks tool */
      void mapOSP(const FileName &brickFileBase, size_t treeID);
      void mapOspBin(const FileName &brickFileBase, size_t treeID);
      void loadBricks(FILE* file, vec2i vbListInfo);
      void loadBricks(FILE* file, LoadBricks aBrick);
      void loadTreeByBrick(const FileName &brickFileBase, size_t treeID,std::vector<vec2i> vbReqList);
      void loadTreeByBrick(const FileName &brickFileBase, size_t treeID);

      // const typename BrickTree<N,T>::ValueBrick * findValueBrick(const vec3i &coord,int blockWidth,int xIdx, int yIdx, int zIdx);
      const T findValue(const vec3i &coord,int blockWidth);

      const T findBrickValue(size_t brickID,vec3i cellPos, size_t parentBrickID,vec3i parentCellPos);

      bool isTreeNeedLoad()
      {   
        for(size_t i= 0;i < numValueBricks;i++){
          if(!valueBricksStatus[i].isLoaded && valueBricksStatus[i].isRequested)
          {
            return true;
          }
        }
        return false;
      }

      //get the request value brick list (L = need to load, F = no need to load)
      // L,L,F,F,F,L,L,L,F,L,F,F,L
      //return (0,2),(5,3)...
      std::vector<vec2i> getRequestVBList()
      { 
        std::vector<vec2i> scheduledVB;
        // size_t startIdx(0);
        // size_t num(0);   
        // bool loadFlag = false;
        // for(size_t i= 0;i < numValueBricks;i++){
        //   if (!valueBricksStatus[i].isLoaded &&
        //       valueBricksStatus[i].isRequested) {
        //     if (!loadFlag) {
        //       startIdx = i;
        //       loadFlag = true;
        //     }
        //     num++;
        //     if(i == numValueBricks){ 
        //       scheduledVB.emplace_back(vec2i(startIdx, num));
        //     }
        //   } else {
        //     if (loadFlag)
        //       scheduledVB.emplace_back(vec2i(startIdx, num));
        //     loadFlag = false;
        //     num      = 0;
        //   }
        // }

        std::stack<int> sIdxStack;
        for(int i=0;i<(int)numValueBricks;i++)
        {
          if (!valueBricksStatus[i].isLoaded && valueBricksStatus[i].isRequested) {
            if(sIdxStack.empty()){
              sIdxStack.push(i);
            }
            if(i == (int)numValueBricks -1){
              scheduledVB.emplace_back(vec2i(sIdxStack.top(), i - sIdxStack.top() + 1));
              sIdxStack.pop();
            }
          }else{
            if(!sIdxStack.empty()){
              scheduledVB.emplace_back(vec2i(sIdxStack.top(), i - sIdxStack.top()));
              sIdxStack.pop();
            }
          }
        }
        return scheduledVB;
      }


      /* gives, for each root cell / tree in the root grid, the ID of
         the first index brick in the (shared) value brick array */
      const int32_t *firstIndexBrickOfTree;

      // virtual const int *firstIndexBrickOfTreePtr() const override { return firstIndexBrickOfTree; } ;
      // virtual const int *firstValueBrickOfTreePtr() const override { return firstValueBrickOfTree; } ;

      /* gives, for each root cell / tree in the rwhileoot grid, the ID of
         the first value brick in the (shared) value brick array */
      const int32_t *firstValueBrickOfTree;


      std::vector<BrickStatus> valueBricksStatus;
      std::vector<size_t> vbNeed2Load;
      size_t loadBrickNum;

    };

    
    /* a entire *FOREST* of bricktrees */
    template <int N, typename T = float>
    struct BrickTreeForest
    {
      void loadTreeBrick(const FileName &brickFileBase)
      {
        // while (!tree.empty()) {
        //   typename std::map<size_t, BrickTree<N, T>>::iterator it;
        //   for (it = tree.begin(); it != tree.end(); it++) {
        //     bool needLoad = false;
        //     std::vector<size_t> vbRequested; 
        //     time_point t1 = Time();
        //     needLoad= it->second.isTreeNeedLoad();
        //     double ts1 = Time(t1);

        //     time_point t2 = Time();
        //     if(needLoad)
        //       it->second.loadTreeByBrick(brickFileBase, it->first);
        //     double ts2 = Time(t2);
        //     //printf("test need load: %f, load tree: %f \n",ts1,ts2);
        //   }
        // }

        while (!tree.empty()) {
          typename std::map<size_t, BrickTree<N, T>>::iterator it;
          for (it = tree.begin(); it != tree.end(); it++) {
            std::vector<vec2i> vbReqList = it->second.getRequestVBList();
            if (!vbReqList.empty())
              it->second.loadTreeByBrick(brickFileBase, it->first, vbReqList);
          }
        }
      }

      void Initialize()
      {
        std::cout<<"#osp: start to initialize bricktree forest!"<<std::endl;
        int numTrees = forestSize.product();
        assert(numTrees > 0);

        std::mutex amutex;
        tasking::parallel_for(numTrees,[&](int treeID){
          BrickTree<N, T> aTree;
          aTree.mapOSP(brickFileBase, treeID);
          std::lock_guard<std::mutex> lock(amutex);
          tree.insert(std::make_pair(treeID, aTree));
        });

        //printf("#osp: %d trees have initialized!",numTrees);
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
        loadBrickTreeForest();
      }

      ~BrickTreeForest()
      {
        tree.clear();
      }

      const vec3i forestSize;
      const vec3i originalVolumeSize;
      const FileName &brickFileBase;

      std::thread loadBrickTreeThread[numThread];

      box3f forestBounds; 

      std::map<size_t, BrickTree<N,T>> tree;

      //std::vector<BrickTree<N, T>> tree;
    };

    // =======================================================
    // INLINE IMPLEMENTATION SECTION 
    // =======================================================
    
    /*! print a value brick */
    template<int N, typename T>
    inline std::ostream &operator<<(std::ostream &o, 
                                    const typename BrickTree<N,T>::ValueBrick &db)
    {      
      for (int iz=0;iz<N;iz++) {
        for (int iy=0;iy<N;iy++) {
          if (N > 2) std::cout << std::endl;
          for (int ix=0;ix<N;ix++)
            o << db.value[iz][iy][ix] << " ";
          o << "| ";
        }
        if (iz < 3) o << std::endl;
      }
        
      return o;
    }
    
    /*! print an index brick */
    template<int N, typename T>
    inline std::ostream &operator<<(std::ostream &o, 
                                    const typename BrickTree<N,T>::IndexBrick &db)
    {      
      for (int iz=0;iz<N;iz++) {
        for (int iy=0;iy<N;iy++) {
          if (N > 2) std::cout << std::endl;
          for (int ix=0;ix<N;ix++)
            o << db.childID[iz][iy][ix] << " ";
          o << "| ";
        }
        if (iz < 3) o << std::endl;
      }
        
      return o;
    }

  } // ::ospray::bt
} // ::ospray



