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

#include "Sumerian.h"

namespace ospray {
  namespace amr {

    struct SingleSumBuilder : public Sumerian::Builder {
      SingleSumBuilder();

      /*! public interface to writing values into the tree */
      virtual void set(const vec3i &coord, int level, float v) override;

      /*! find or create data block that contains given cell. if that
        block (or any of its parents, indices referring to it, etc)
        does not exist, create and initialize whatever is required to
        have this node */
      Sumerian::DataBlock *findDataBlock(const vec3i &coord, int level);

      uint32 newDataBlock();

      /*! get index block for given data block ID - create if required */
      Sumerian::IndexBlock *getIndexBlockFor(uint32 dataBlockID);

      std::vector<uint32> indexBlockOf;
      std::vector<Sumerian::DataBlock *>  dataBlock;
      std::vector<Sumerian::IndexBlock *> indexBlock;

      /*! be done with the build, and save all data to the xml/bin
        file of 'fileName' and 'filename+"bin"' */
      virtual void save(const std::string &ospFileName, const vec3f &clipBoxSize);
    };

    /*! multi-sumerian - a root grid of cell, with one sumerian per
        cell */
    struct MultiSumBuilder : public Sumerian::Builder {
      MultiSumBuilder();

      /*! public interface to writing values into the tree */
      virtual void set(const vec3i &coord, int level, float v) override;

      SingleSumBuilder *getRootCell(const vec3i &rootCellID);
      void allocateAtLeast(const vec3i &neededSize);

      /*! the 3d array of (single) sumerians */
      ActualArray3D<SingleSumBuilder *> *rootGrid;

      /* value range of all the values written into this tree ... */
      Range<float> valueRange;

      /*! be done with the build, and save all data to the xml/bin
        file of 'fileName' and 'filename+"bin"' */
      virtual void save(const std::string &ospFileName, const vec3f &clipBoxSize);
    };

  } // ::ospray::amr
} // ::ospray
