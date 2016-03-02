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

        /*! return float position of given cell center, in GRID space */
        inline vec3f centerPos() const { return (vec3f(x,y,z)+vec3f(.5f))*(1.f/(1<<m)); }
        /*! return the cell index of the coarsest (root) level ancester of the given cell index */
        inline CellIdx rootAncestorIndex() const { return CellIdx(vec3i(x >> m, y >> m, z >> m), m); }
        /*! return cell index of given neighbor _direction_ (ie, dx,dy,dz are RELATEIVE to *this */
        inline CellIdx neighborIndex(const vec3i &delta) const { return CellIdx(vec3i(x,y,z)+delta,m); }
        
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


      /*! try to locate the given cell at 'desiredIdx', and return
          either the cell itself (if it exists), or the leaf cell
          that's the finest existing ancestor of the cell (if it
          doesn't exist on the desired level) */
      const Cell *findCell(const CellIdx &desiredIdx, 
                           CellIdx &actualIdx) const;
      
      /*! find a given leaf cell, returning a pointer to the leaf cell
          itself (as return value) and the logical cell index (2nd
          reference parameters) that this cell would have had in
          infinite nested grid space. the location is given in GRID coordinates, ie, in [(0,0,0):dims] */
      const Cell *findLeafCell(const vec3f &gridPos, CellIdx &idx) const;



      /*! return interpolated value at given unit position (ie pos must be in [(0),(1)] */
      /*! note(iw): i'll TEMPORARILY hcnage this to return a vec3f,
          which is easier for debugging; eventually we DO need
          floats */
      vec3f sample(const vec3f &unitPos) const;
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
