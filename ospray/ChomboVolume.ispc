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

// ours
#include "CellRef.ih"
// ospray
#include "ospray/math/AffineSpace.ih"
#include "ospray/common/Ray.ih"
#include "ospray/geometry/Geometry.ih"
#include "ospray/common/Model.ih"
#include "ospray/transferFunction/LinearTransferFunction.ih"
// embree
#include "embree2/rtcore.isph"

/*! plainest, dumbest way of reconstructing: find the given leaf cell,
  and return the value */
extern varying float Chombo_nearestNeighbor(void *uniform _self, 
                                            const varying vec3f &worldSpacePos)
{
  Chombo *uniform self = (Chombo *)_self;
  // for now, let's assume the world-space position already IS in grid space
  const vec3f gridPos = worldSpacePos;
  float samplePos[3];
  samplePos[0] = gridPos.x;
  samplePos[1] = gridPos.y;
  samplePos[2] = gridPos.z;

#if 1
  uniform uint32 nodeStack[64];
  varying bool activeStack[64];
  nodeStack[0] = 0;
  unmasked { activeStack[0] = false; }
  activeStack[0] = true;
  uniform uint32 stackPtr = 1;
  while (stackPtr > 0) {
    --stackPtr;
    if (!activeStack[stackPtr]) continue;
    const uniform nodeID = nodeStack[stackPtr];
    const uniform KDTreeNode &node = self->node[nodeID];
    if (isLeaf(node)) {
      const ChomboBrick *uniform brick = self->item[getOfs(node)];
      const vec3f relBrickPos
        = (worldSpacePos - brick->bounds.lower)
        * rcp(brick->bounds.upper - brick->bounds.lower);
      // brick coords: integer cell coordinates inside brick
      const vec3i bc = make_vec3i(relBrickPos * make_vec3f(brick->dims));
      // clamped brick coordinates
      const vec3i cbc = max(make_vec3i(0),min(bc,brick->dims-make_vec3i(1)));
      return brick->value[cbc.x + brick->dims.x*(cbc.y+brick->dims.y*(cbc.z))];
    } else {
      const uniform uint32 childID = getOfs(node);
      if (samplePos[getDim(node)] >= getPos(node)) {
        unmasked { activeStack[stackPtr] = 0; }
        nodeStack[stackPtr] = childID+1;
        activeStack[stackPtr] = 1;
        stackPtr++;
      } else {
        unmasked { activeStack[stackPtr] = 0; }
        nodeStack[stackPtr] = childID;
        activeStack[stackPtr] = 1;
        stackPtr++;
      }
    }
  }
#else
  uint32 nodeID=0;
  while (1) {
    const KDTreeNode node = self->node[nodeID];
    if (isLeaf(node)) {
      return ((getOfs(node) * 13 * 17 * 23) % 128) / 127.f;
      break;
    } else {
      nodeID = getOfs(node);
      if (samplePos[getDim(node)] >= getPos(node))
        nodeID++;
    }
  }
#endif
}

void buildNodeRanges(Chombo *uniform self, 
                     uniform int nodeID,
                     const uniform box3f &bounds)
{
  const uniform KDTreeNode node = self->node[nodeID];
  if (isLeaf(node)) {
    self->nodeRange[nodeID] = make_box1f(0.f);
  } else {
    uniform box3f lBounds = bounds;
    uniform box3f rBounds = bounds;
    set(lBounds.upper,getDim(node),getPos(node));
    set(rBounds.lower,getDim(node),getPos(node));
    uniform uint32 childID = getOfs(node);
    buildNodeRanges(self,childID+0,lBounds);
    buildNodeRanges(self,childID+1,rBounds);
    self->nodeRange[nodeID] = box_union(self->nodeRange[childID+0],
                                        self->nodeRange[childID+1]);
  }
}

void buildNodeRanges(Chombo *uniform self)
{
  self->nodeRange = uniform new uniform box1f[self->numNodes];
  buildNodeRanges(self,0,self->bounds);
}


//! The gradient at the given sample location in world coordinates.
static varying vec3f Chombo_gradient(void *uniform _self, 
                                     const varying vec3f &worldCoordinates)
{
  return make_vec3f(1,0,0);
}

//! Find the next hit point in the volume for ray casting based renderers.
static void Chombo_intersect(void *uniform _self, 
                             varying Ray &ray)
{
  ChomboVolume *uniform self = (ChomboVolume *uniform) _self;

  // TODO: clean this up, properly implemnt an integrator with
  // variable step size...
  const uniform float step
    = self->super.samplingStep / self->super.samplingRate;

  ray.t0 += step;
}


export void *uniform ChomboVolume_create(void *uniform cppE)
{
  ChomboVolume *uniform self = uniform new uniform ChomboVolume;
  // self->super.cppE = cppE;
  return self;
} 

export void ChomboVolume_set(void *uniform _self,
                             void *uniform _xf,
                             uniform vec3i &rootGridDims,
                             void *uniform _brick_array,
                             void *uniform _tree_node,
                             uniform int32 numTreeNodes,
                             void *uniform _tree_item)
{
  ChomboVolume *uniform self = (ChomboVolume *uniform)_self;
  assert(self);
  
  self->super.boundingBox.lower = make_vec3f(0.f);
  self->super.boundingBox.upper = make_vec3f(rootGridDims); 
  
  self->super.volumeClippingBox.lower = make_vec3f(0.f);
  self->super.volumeClippingBox.upper = make_vec3f(rootGridDims); 
  
  self->super.samplingRate = 1.f;//samplingRate;
  self->super.samplingStep = .1f;

  self->super.gradientShadingEnabled = 0;

  self->super.transferFunction = (TransferFunction *uniform)_xf;

  self->super.intersect       = &Chombo_intersect;
  self->super.computeGradient = &Chombo_gradient;
  self->super.computeSample   = &Chombo_nearestNeighbor;


  self->chombo.brick  = (ChomboBrick  *)_brick_array;
  self->chombo.node   = (KDTreeNode   *)_tree_node;
  self->chombo.numNodes = numTreeNodes;
  self->chombo.item   = (ChomboBrick **)_tree_item;
  self->chombo.rootGridDims = rootGridDims;

  self->chombo.bounds.lower = make_vec3f(0);
  self->chombo.bounds.upper = make_vec3f(rootGridDims);
  //  buildNodeRanges(self);
}




