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

#include "BrickTree.h"

#ifdef OSPRAY_TASKING_TBB
#  include <tbb/parallel_for.h>
#  define PARALLEL_MULTI_TREE_BUILD
#endif

namespace ospray {
  namespace amr {

    template<int N, typename T>
    struct BrickTreeBuilder : public BrickTree<N,T>::Builder {
      BrickTreeBuilder();
      ~BrickTreeBuilder();
#ifdef PARALLEL_MULTI_TREE_BUILD
      std::mutex mutex;
#endif

      /*! public interface to writing values into the tree */
      virtual void set(const vec3i &coord, int level, float v) override;

      /*! find or create data brick that contains given cell. if that
        brick (or any of its parents, indices referring to it, etc)
        does not exist, create and initialize whatever is required to
        have this node */
      typename BrickTree<N,T>::DataBrick *findDataBrick(const vec3i &coord, int level);

      int32 newDataBrick();

      /*! get index brick for given data brick ID - create if required */
      typename BrickTree<N,T>::IndexBrick *getIndexBrickFor(int32 dataBrickID);

      std::vector<int32> indexBrickOf;
      std::vector<typename BrickTree<N,T>::DataBrick *>  dataBrick;
      std::vector<typename BrickTree<N,T>::IndexBrick *> indexBrick;

      /*! be done with the build, and save all data to the xml/bin
        file of 'fileName' and 'filename+"bin"' */
      virtual void save(const std::string &ospFileName, const vec3f &clipBoxSize) override;

      int maxLevel;
    };

    /*! multi-sumerian - a root grid of cell, with one sumerian per
      cell */
    template<int N, typename T>
    struct MultiBrickTreeBuilder : public BrickTree<N,T>::Builder {
      MultiBrickTreeBuilder();
      ~MultiBrickTreeBuilder();

      /*! public interface to writing values into the tree */
      virtual void set(const vec3i &coord, int level, float v) override;

      /*! be done with the build, and save all data to the xml/bin
        file of 'fileName' and 'filename+"bin"' */
      virtual void save(const std::string &ospFileName, const vec3f &clipBoxSize) override;

      // const ActualArray3D<BrickTreeBuilder<N,T> *> *getRootGrid() const
      // { return rootGrid; } 

      BrickTreeBuilder<N,T> *getRootCell(const vec3i &rootCellID);
      vec3i getRootGridSize() {
#ifdef PARALLEL_MULTI_TREE_BUILD
        std::lock_guard<std::mutex> lock(this->rootGridMutex);
#endif
        return rootGrid->size();
      }
    private:
      void allocateAtLeast(const vec3i &neededSize);

      /*! the 3d array of (single) sumerians */
      ActualArray3D<BrickTreeBuilder<N,T> *> *rootGrid;

#ifdef PARALLEL_MULTI_TREE_BUILD
      std::mutex rootGridMutex;
#endif

      /* value range of all the values written into this tree ... */
      Range<T> valueRange;
    };

  } // ::ospray::amr
} // ::ospray
