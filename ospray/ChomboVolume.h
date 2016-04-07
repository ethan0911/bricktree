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
#include "ospray/volume/Volume.h"
// amr base
#include "../amr/Array3D.h"

namespace ospray {
  namespace amr {

    struct Chombo : public ospray::Volume {
      /*! this is how an app _specifies_ a brick (or better, the array
        of bricks); the brick data is specified through a separate
        array of data buffers (one data buffer per brick) */
      struct BrickInfo {
        /*! bounding box of integer coordinates of cells. note that
          this EXCLUDES the width of the rightmost cell: ie, a 4^3
          box at root level pos (0,0,0) would have a _box_ of
          [(0,0,0)-(3,3,3)] (because 3,3,3 is the highest valid
          coordinate in this box!), while its bounds would be
          [(0,0,0)-(4,4,4)]. Make sure to NOT use box.size() for the
          grid dimensions, since this will always be one lower than
          the dims of the grid.... */
        box3i box;
        //! level this brick is at
        int   level;
        // width of each cell in this level
        float cellWidth;

        // inline box3f worldBounds() const 
        // { 
        //   return box3f(vec3f(box.lower) * dt, vec3f(box.upper+vec3i(1)) * dt);
        // }
      };

      /*! this is how we store bricks internally */
      struct Brick : public BrickInfo {
        /* world bounds, including entire cells. ie, a 4^3 root brick
           at (0,0,0) would have bounds [(0,0,0)-(4,4,4)] (as opposed
           to the 'box' value, see above!) */
        box3f bounds;
        // pointer to the actual data values stored in this brick
        float *value;
        // dimensions of this box's data
        vec3i dims;
        // scale factor from grid space to world space (ie,1.f/cellWidth)
        float gridToWorldScale;
        
        // rcp(bounds.upper-bounds.lower);
        vec3f bounds_scale;
        // dimensions, in float
        vec3f f_dims;
      };

      Chombo();

      //! \brief common function to help printf-debugging 
      virtual std::string toString() const { return "ospray::amr::Chombo"; }
      
      /*! \brief integrates this volume's primitives into the respective
        model's comperation structure */
      // virtual void finalize(Model *model);

      //! Allocate storage and populate the volume.
      virtual void commit();

      //! Copy voxels into the volume at the given index (non-zero return value indicates success).
      virtual int setRegion(const void *source, const vec3i &index, const vec3i &count)
      {
        FATAL("'setRegion()' doesn't make sense for AMR volumes; they can only be set from existing data");
      }

      Ref<Data> brickInfoData;
      Ref<Data> brickDataData;

      struct KDTree {
        /*! each node in the tree refers to either a pair ofo child
          nodes (using split plane pos, split plane dim, and offset in
          node[] array), or to a list of 'numItems' bricks (using
          dim=3, and offset pointing to the start of the list of
          numItems brick pointers in 'brickArray[]'. In case of a
          leaf, the 'num' bricks stored in this leaf are (in theory),
          exactly one brick per level, in sorted order */
        struct Node {
          // first dword
          uint32 ofs:30; // offset in node[] array (if inner), or brick ID (if leaf)
          uint32 dim:2;  // upper two bits: split dimension. '3' means 'leaf
          // second dword
          union {
            float pos;
            uint32 numItems;
          };
        };
        void makeLeaf(index_t nodeID, const std::vector<const Chombo::Brick *> &brickIDs);
        void makeInner(index_t nodeID, int dim, float pos, int childID);
        void buildRec(int nodeID,const box3f &bounds,std::vector<const Chombo::Brick *> &brick);
        /*! build entire tree over given number of bricks */
        void build(const Chombo::Brick *brickArray, size_t numBricks);

        std::vector<Node>                  node;
        std::vector<const Chombo::Brick *> item;
      };

      void makeBricks();

      size_t numBricks;
      Brick *brickArray;
      KDTree kdTree;
      Range<float> *rangeOfKDTreeNode;
    };
    
  }
}

