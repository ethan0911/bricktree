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
#include "ChomboInput.h"

namespace ospray {
  namespace amr {

    struct ChomboAccel {
      struct Leaf {
        Leaf() {};
        Leaf(const Leaf &o) : brickList(o.brickList), bounds(o.bounds) {}
        /*! list of bricks that overlap this leaf; sorted from finest
            to coarsest level */
        const Brick **brickList;
        /*! bounding box of this leaf - note that the bricks will
            likely "stick out" of this bounding box, and the same
            brick may be listed in MULTIPLE leaves */
        box3f bounds;
      };

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
      void makeLeaf(index_t nodeID, 
                    const box3f &bounds,
                    const std::vector<const Chombo::Brick *> &brickIDs);
      void makeInner(index_t nodeID, int dim, float pos, int childID);
      void buildRec(int nodeID,const box3f &bounds,std::vector<const Chombo::Brick *> &brick);
      /*! build entire tree over given number of bricks */
      void build(const Chombo::Brick *brickArray, size_t numBricks);
      
      std::vector<Node>         node;
      std::vector<Chombo::Leaf> leaf;

      KDTree kdTree;
    };

    /*! the actual ospray volume object */
    struct ChomboVolume : public ospray::Volume {

      //! \brief common function to help printf-debugging 
      virtual std::string toString() const { return "ospray::amr::Chombo"; }
      
      //! Allocate storage and populate the volume.
      virtual void commit();

      /*! Copy voxels into the volume at the given index (non-zero
        return value indicates success). */
      virtual int setRegion(const void *source, const vec3i &index, const vec3i &count);

      ChomboAccel chombo;

      Ref<Data> brickInfoData;
      Ref<Data> brickDataData;

    };
  }
}

