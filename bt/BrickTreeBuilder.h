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
// std
#include <mutex>

#ifdef OSPRAY_TASKING_TBB
#  include <tbb/parallel_for.h>
#  define PARALLEL_MULTI_TREE_BUILD
#endif

namespace ospray {
  namespace bt {

    /*! builds ONE bricktree - NOT a forest of them; just a single one */
    template<int N, typename T>
    struct BrickTreeBuilder
    {
      BrickTreeBuilder();
      ~BrickTreeBuilder();
#ifdef PARALLEL_MULTI_TREE_BUILD
      std::mutex mutex;
#endif

      /*! public interface to writing values into the tree */
      virtual void set(const vec3i &coord, int level, float v) ;

      /*! find or create value brick that contains given cell. if that
        brick (or any of its parents, indices referring to it, etc)
        does not exist, create and initialize whatever is required to
        have this node */
      typename BrickTree<N,T>::ValueBrick *findValueBrick(const vec3i &coord, int level);

      int32_t newValueBrick();

      /*! get index brick for given value brick ID - create if required */
      typename BrickTree<N,T>::IndexBrick *getIndexBrickFor(int32_t valueBrickID);

      std::vector<int32_t> indexBrickOf;
      std::vector<typename BrickTree<N,T>::ValueBrick *> valueBrick;
      std::vector<typename BrickTree<N,T>::IndexBrick *> indexBrick;

      /*! be done with the build, and save all value to the xml/bin
        file of 'fileName' and 'filename+"bin"' */
      // virtual void save(const std::string &ospFileName, const vec3i
      // &validSize) const override;

      int maxLevel;
    };

    template <int N>
    inline int brickSizeOf(int level)
    {
      int bs = N;
      for (int i = 0; i < level; i++)
        bs *= N;
      return bs;
    }

  }  // namespace bt
}  // namespace ospray
