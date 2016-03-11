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
    struct AMR {
      
      struct CellIdx : public vec3i {
        CellIdx() {};
        CellIdx(const vec3i &idx, int m) : vec3i(idx), m(m) {}

        /*! return float position of given cell center, in GRID space */
        inline vec3f centerPos() const { return (vec3f(x,y,z)+vec3f(.5f))*(1.f/(1<<m)); }
        /*! return the cell index of the coarsest (root) level ancester of the given cell index */
        inline CellIdx rootAncestorIndex() const { return CellIdx(vec3i(x >> m, y >> m, z >> m), 0); }
        /*! return cell index of given neighbor _direction_ (ie, dx,dy,dz are RELATEIVE to *this */
        inline CellIdx neighborIndex(const vec3i &delta) const { return CellIdx(vec3i(x,y,z)+delta,m); }
        /*! return index of given child */
        inline CellIdx childIndex(const vec3i &d) const { return CellIdx(vec3i(2*x+d.x,2*y+d.y,2*z+d.z),m+1); }
        /*! return a (obviously) invalid index */
        static inline CellIdx invalidIndex() { return CellIdx(vec3i(-1),-1); }
        int m; /*!< level ID */

        inline friend std::ostream &operator<<(std::ostream &o,
                                 const typename AMR::CellIdx &i)
        { o << "(" << i.x << "," << i.y << "," << i.z << ":" << i.m << ")"; return o;}
      };

      typedef typename Octree::Cell Cell;
      typedef typename Octree::OctCell OctCell;
      
      AMR() : dimensions(0) {}
      
      void allocate(const vec3i &newDims) { dimensions = newDims; rootCell.resize(numCells()); }
      size_t numCells() const { return dimensions.x*dimensions.y*dimensions.z; }
      
      virtual float sample(const vec3f &unitPos);

      // 3D array of root nodes
      std::vector<Cell> rootCell;
      // array of (all) child cells, across all octrees
      std::vector<OctCell> octCell;
      // dimensions of root grid
      vec3i dimensions;

      Range<float> getValueRange(const int32_t subCellID) const;
      Range<float> getValueRange(const vec3i &rootCellID) const;
      Range<float> getValueRange() const;

      void writeTo(const std::string &outFileName);
      void mmapFrom(const unsigned char *binBasePtr);
      static AMR *loadFrom(const char *fileName);
    };

  } // ::ospray::amr
} // ::ospray
