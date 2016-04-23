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

#include "ChomboVolume.h"
// ospray
#include "ospray/common/Model.h"
#include "ospray/common/Data.h"
#include "ospray/transferFunction/TransferFunction.h"
// ispc exports
#include "ChomboVolume_ispc.h"
// stl
#include <set>
#include <map>

namespace ospray {
  namespace amr {
    using std::endl;
    using std::cout;
    using std::ostream;
    using std::flush;

    Chombo::KDTree::~KDTree() 
    {
      for (int i=0;i<leaf.size();i++)
        delete[] leaf[i].brickList;
    }

    void Chombo::KDTree::makeLeaf(index_t nodeID,
                                  const box3f &bounds,
                                  const std::vector<const Chombo::Brick *> &brick) 
    { 
      node[nodeID].dim = 3; 
      node[nodeID].ofs = this->leaf.size(); //item.size(); 
      node[nodeID].numItems = brick.size();

      Chombo::Leaf newLeaf;
      newLeaf.bounds = bounds;
      newLeaf.brickList = new const Chombo::Brick *[brick.size()];
      for (int i=0;i<brick.size();i++)
        // push bricks in reverse order, so the finest level comes first
        newLeaf.brickList[i] = brick[brick.size()-1-i];
      this->leaf.push_back(newLeaf);
    }
    
    void Chombo::KDTree::makeInner(index_t nodeID, int dim, float pos, int childID) 
    { 
      node[nodeID].dim = dim; 
      node[nodeID].pos = pos; 
      node[nodeID].ofs = childID; 
    }
    
    void Chombo::KDTree::buildRec(int nodeID,
                                  const box3f &bounds,
                                  std::vector<const Chombo::Brick *> &brick)
    {
      std::set<float> possibleSplits[3];
      for (int i=0;i<brick.size();i++) {
        const box3f clipped = intersectionOf(bounds,brick[i]->bounds);
        if (clipped.lower.x != bounds.lower.x)
          possibleSplits[0].insert(clipped.lower.x);
        if (clipped.upper.x != bounds.upper.x)
          possibleSplits[0].insert(clipped.upper.x);
        if (clipped.lower.y != bounds.lower.y)
          possibleSplits[1].insert(clipped.lower.y);
        if (clipped.upper.y != bounds.upper.y)
          possibleSplits[1].insert(clipped.upper.y);
        if (clipped.lower.z != bounds.lower.z)
          possibleSplits[2].insert(clipped.lower.z);
        if (clipped.upper.z != bounds.upper.z)
          possibleSplits[2].insert(clipped.upper.z);
      }

      int bestDim = -1;
      vec3f width = bounds.size();
      for (int dim=0;dim<3;dim++) {
        if (possibleSplits[dim].empty()) continue;
        if (bestDim == -1 || width[dim] > width[bestDim])
          bestDim = dim;
      }

      if (bestDim == -1) {
        // no split dim - make a leaf

        // note that by construction the last brick must be the onoe
        // we're looking for (all on a lower level must be earlier in
        // the list)
        assert(nodeID < this->node.size());
        makeLeaf(nodeID,bounds,brick);
      } else {
        float bestPos = std::numeric_limits<float>::infinity();
        float mid = bounds.center()[bestDim];
        for (auto it = possibleSplits[bestDim].begin(); 
             it != possibleSplits[bestDim].end(); it++) {
          if (fabsf(*it - mid) < fabsf(bestPos-mid))
            bestPos = *it;
        }
        box3f lBounds = bounds;
        box3f rBounds = bounds;

        switch(bestDim) {
        case 0:
          lBounds.upper.x = bestPos;
          rBounds.lower.x = bestPos;
          break;
        case 1:
          lBounds.upper.y = bestPos;
          rBounds.lower.y = bestPos;
          break;
        case 2:
          lBounds.upper.z = bestPos;
          rBounds.lower.z = bestPos;
          break;
        }


        std::vector<const Chombo::Brick *> l, r;
        float sum_lo = 0.f, sum_hi = 0.f;
        for (int i=0;i<brick.size();i++) {
          const box3f wb = intersectionOf(brick[i]->bounds,bounds);
          if (wb.empty())
            throw std::runtime_error("empty box!?");
          sum_lo += wb.lower[bestDim];
          sum_hi += wb.upper[bestDim];
          if (wb.lower[bestDim] >= bestPos) {
            r.push_back(brick[i]);
          } else if (wb.upper[bestDim] <= bestPos) {
            l.push_back(brick[i]);
          } else {
            r.push_back(brick[i]);
            l.push_back(brick[i]);
          }
        }
        assert(!(l.empty() || r.empty()));
        
        int newNodeID = node.size();
        makeInner(nodeID,bestDim,bestPos,newNodeID);

        node.push_back(KDTree::Node());
        node.push_back(KDTree::Node());
        brick.clear();

        buildRec(newNodeID+0,lBounds,l);
        buildRec(newNodeID+1,rBounds,r);
      }
    }
    
    /*! build entire tree over given number of bricks */
    void Chombo::KDTree::build(const Chombo::Brick *brickArray,
                               size_t numBricks)
    {
      box3f bounds = empty;
      std::vector<const Chombo::Brick *> brick;
      for (int i=0;i<numBricks;i++) {
        brick.push_back(brickArray+i);
        bounds.extend(brick.back()->bounds);
      }
      node.resize(1);
      buildRec(0,bounds,brick);
      cout << "#osp:chom: done building kdtree, created " 
           << node.size() << " nodes" << endl;
    }

    Chombo::Chombo()
      : Volume()
    {
      ispcEquivalent = ispc::ChomboVolume_create(this);
    }

    void Chombo::makeBricks()
    {
      assert(brickInfoData);
      numBricks = brickInfoData->numBytes / sizeof(BrickInfo);
      cout << "#osp:chom: making bricks - found " << numBricks << " brick infos" << endl;
      brickArray = new Brick[numBricks];

      Data **allBricksData = (Data **)brickDataData->data;

      for (int i=0;i<numBricks;i++) {
        Brick *brick = brickArray+i;
        BrickInfo *info = (BrickInfo*)brickInfoData->data + i;
        Data      *thisBrickData = allBricksData[i];
        // copy values
        brick->box = info->box;
        brick->level = info->level;
        brick->cellWidth = info->cellWidth;

        brick->value = (float*)thisBrickData->data;
        brick->dims  = brick->box.size() + vec3i(1);
        brick->gridToWorldScale = 1.f/brick->cellWidth;
        brick->bounds = box3f(vec3f(brick->box.lower) * brick->cellWidth, 
                              vec3f(brick->box.upper+vec3i(1)) * brick->cellWidth);
        brick->bounds_scale = rcp(brick->bounds.size());
        brick->f_dims = vec3f(brick->dims);
      }
    }

    //! Allocate storage and populate the volume.
    void Chombo::commit()
    {
      cout << "#osp:amr: creating chombo volume" << endl;

      brickInfoData = getParamData("brickInfo");
      assert(brickInfoData->data);

      brickDataData = getParamData("brickData");
      assert(brickDataData->data);

      Ref<TransferFunction> xf = (TransferFunction*)getParamObject("transferFunction");
      assert(xf);

      makeBricks();
      kdTree.build(brickArray,numBricks);

      float finestLevelCellWidth = brickArray[0].cellWidth;
      box3i rootLevelBox = empty;
      for (int i=0;i<numBricks;i++) {
        if (brickArray[i].level == 0)
          rootLevelBox.extend(brickArray[i].box);
        finestLevelCellWidth = min(finestLevelCellWidth,brickArray[i].cellWidth);
      }
      vec3i rootGridDims = rootLevelBox.size()+vec3i(1);
      cout << "#osp:chom: found root level dimensions of " << rootGridDims << endl;
      ispc::ChomboVolume_set(getIE(),
                             xf->getIE(),
                             (ispc::vec3i&)rootGridDims,
                             brickArray,
                             &kdTree.node[0],
                             kdTree.node.size(),
                             &kdTree.leaf[0],
                             kdTree.leaf.size(),
                             finestLevelCellWidth);
      cout << "#osp:chom: done building chombo volume" << endl;
    }

    OSP_REGISTER_VOLUME(Chombo,ChomboVolume);
    OSP_REGISTER_VOLUME(Chombo,chombo_volume);
  }
} // ::ospray

