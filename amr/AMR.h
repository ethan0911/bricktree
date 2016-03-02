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

// ospray
#include "Octree.h"

namespace ospray {
  namespace amr {

    /*! out multi-octree data structure - one root grid with one octree per cell */
    template<typename voxel_t>
    struct AMR {
      
      struct CellIdx : public vec3i {
        CellIdx() {};
        CellIdx(const vec3i &idx, int m) : vec3i(idx), m(m) {}

        int m; /*!< level ID */
      };

      typedef typename Octree<voxel_t>::Cell Cell;
      typedef typename Octree<voxel_t>::OctCell OctCell;
      
      AMR() : dimensions(0) {}
      
      void allocate(const vec3i &newDims) { dimensions = newDims; rootCell.resize(numCells()); }
      size_t numCells() const { return dimensions.x*dimensions.y*dimensions.z; }
      
      // 3D array of root nodes
      std::vector<Cell> rootCell;
      // array of (all) child cells, across all octrees
      std::vector<OctCell> octCell;
      // dimensions of root grid
      vec3i dimensions;

      Range<voxel_t> getValueRange(const int32_t subCellID) const;
      Range<voxel_t> getValueRange(const vec3i &rootCellID) const;
      Range<voxel_t> getValueRange() const;

      
      /*! find a given leaf cell, returning a pointer to the leaf cell
          itself (as return value) and the logical cell index (2nd
          reference parameters) that this cell would have had in
          infinite nested grid space */
      const Cell *findLeafCell(const vec3f &unitPos, CellIdx &idx) const;



      /*! return interpolated value at given unit position (ie pos must be in [(0),(1)] */
      float sample(const vec3f &unitPos) const;
      float sampleRootCellOnly(const vec3f &unitPos) const;
      /*! this version will sample down to whatever octree leaf gets
        hit, but without interpolation, just nearest neighbor */
      float sampleFinestLeafNearestNeighbor(const vec3f &unitPos) const; 

      void writeTo(const std::string &outFileName);
      void mmapFrom(const unsigned char *binBasePtr);
      static AMR<voxel_t> *loadFrom(const char *fileName);
    };
    
  } // ::ospray::amr
} // ::ospray
