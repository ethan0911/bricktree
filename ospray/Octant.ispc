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
#include "Octant.ih"

struct FindEightStack {
  varying bool act_lo[3];
  varying bool act_hi[3];
  uniform int32 nodeID;
};

void findEightCellsNotOrdered(const Chombo *uniform self,
                              Octant &oct,
                              const float tgtWidth)
{
#if 0
  set(oct.c[C000],findCell(self,make_vec3f(oct.P0.x,oct.P0.y,oct.P0.z),tgtWidth));
  set(oct.c[C001],findCell(self,make_vec3f(oct.P1.x,oct.P0.y,oct.P0.z),tgtWidth));

  set(oct.c[C010],findCell(self,make_vec3f(oct.P0.x,oct.P1.y,oct.P0.z),tgtWidth));
  set(oct.c[C011],findCell(self,make_vec3f(oct.P1.x,oct.P1.y,oct.P0.z),tgtWidth));

  set(oct.c[C100],findCell(self,make_vec3f(oct.P0.x,oct.P0.y,oct.P1.z),tgtWidth));
  set(oct.c[C101],findCell(self,make_vec3f(oct.P1.x,oct.P0.y,oct.P1.z),tgtWidth));

  set(oct.c[C110],findCell(self,make_vec3f(oct.P0.x,oct.P1.y,oct.P1.z),tgtWidth));
  set(oct.c[C111],findCell(self,make_vec3f(oct.P1.x,oct.P1.y,oct.P1.z),tgtWidth));
  return;
#endif
  // clear(oct);
  const uniform float hcw = 0.5f*self->finestLevelCellWidth;
  const vec3f _P0 = clamp(oct.P0,
                          make_vec3f(0.f),
                          self->bounds.upper-make_vec3f(hcw));
  const vec3f _P1 = clamp(oct.P1,
                          make_vec3f(0.f),
                          self->bounds.upper-make_vec3f(hcw));

  const varying float *const uniform p0 = &_P0.x;
  const varying float *const uniform p1 = &_P1.x;

  const float lo[3] = { min(_P0.x,_P1.x),min(_P0.y,_P1.y),min(_P0.z,_P1.z) };
  const float hi[3] = { max(_P0.x,_P1.x),max(_P0.y,_P1.y),max(_P0.z,_P1.z) };

#define STACK_SIZE 64
  uniform FindEightStack stack[STACK_SIZE];
  uniform FindEightStack *uniform stackPtr = &stack[0];
  // uniform FindStack *uniform stackPtr = pushStack(&stack[0],0,true);
  
  uniform int32 leafList[programCount*8];
  uniform int32 numLeaves = 0;

// #if 1
  bool act_lo[3] = { true, true, true };
  bool act_hi[3] = { true, true, true };
  uniform int nodeID = 0;
  // print("-------------------------------------------------------\n");
  while (any(true)) {
    // print("node %\n",nodeID);
    const uniform KDTreeNode &node = self->node[nodeID];
    const uniform uint32 childID = getOfs(node);
    if (isLeaf(node)) {
      // print("leaf (%) : %\n",numLeaves,childID);
      assert(numLeaves < (programCount * 8));
      leafList[numLeaves++] = childID;
      // go on to popping ...
    } else {
      const uniform int dim = getDim(node);
      const uniform float pos = getPos(node);
      const bool in_active
        = (act_lo[0] | act_hi[0])
        & (act_lo[1] | act_hi[1])
        & (act_lo[2] | act_hi[2]);
      const bool go_left 
        = ((act_lo[dim] & (lo[dim] < pos)) | 
           (act_hi[dim] & (hi[dim] < pos)))
        & in_active
        // & (act_lo[(dim+1)%3] | act_hi[(dim+1)%3])
        // & (act_lo[(dim+2)%3] | act_hi[(dim+2)%3]);
        ;
      const bool go_right
        = ((act_lo[dim] & (lo[dim] >= pos)) | 
           (act_hi[dim] & (hi[dim] >= pos)))
        & in_active
        // & (act_lo[(dim+1)%3] | act_hi[(dim+1)%3])
        // & (act_lo[(dim+2)%3] | act_hi[(dim+2)%3]);
        ;
        // = (act_lo[dim] & (lo[dim] >= pos)) 
        // | (act_hi[dim] & (hi[dim] >= pos))
        // ;
        // & (act_lo[(dim+1)%3] | act_hi[(dim+1)%3])
        // & (act_lo[(dim+2)%3] | act_hi[(dim+2)%3]);
        //       const bool go_right
        // = act_lo[dim] & (lo[dim] >= pos)
        // | act_hi[dim] & (hi[dim] >= pos);

      if (none(go_right)) {
        // all to the left: go left and iterate
        nodeID = childID+0;
        continue;
      }

      if (none(go_left)) {
        // all to the left: go left and iterate
        nodeID = childID+1;
        continue;
      }

      // push right
      stackPtr->nodeID = childID+1;
      for (uniform int i=0;i<3;i++) {
#if 1
        unmasked {
          stackPtr->act_lo[i] = act_lo[i];
          stackPtr->act_hi[i] = act_hi[i];
        }
#else
        unmasked { stackPtr->act_lo[i] = false; }
        unmasked { stackPtr->act_hi[i] = false; }
        stackPtr->act_lo[i] = act_lo[i];
        stackPtr->act_hi[i] = act_hi[i];
#endif
        // stackPtr->act_lo[i]
        //   = act_lo[i] & (lo[dim] >= pos)
        //   // & (act_lo[(dim+1)%3] | act_hi[(dim+1)%3])
        //   // & (act_lo[(dim+2)%3] | act_hi[(dim+2)%3])
        //   ;
        // stackPtr->act_hi[i] 
        //   = act_hi[i] & (hi[dim] >= pos)
        //   // & (act_lo[(dim+1)%3] | act_hi[(dim+1)%3])
        //   // & (act_lo[(dim+2)%3] | act_hi[(dim+2)%3])
        //   ;
      }
      stackPtr->act_lo[dim] &= (lo[dim] >= pos);
      stackPtr->act_hi[dim] &= (hi[dim] >= pos);
      ++stackPtr;
      // assert(stackPtr-stack < STACK_SIZE);
      
      // go left
      nodeID = childID+0;
      act_lo[dim] &= (lo[dim] < pos);
      act_hi[dim] &= (hi[dim] < pos);
      // for (uniform int i=0;i<3;i++) {
      //   act_lo[i] = act_lo[i] & (lo[dim] < pos);
      //   act_hi[i] = act_hi[i] & (hi[dim] < pos);
      //   assert(any(act_lo[i] | act_hi[i]));
      // }
      continue;
    }
    // pop:
    if (stackPtr == stack) break;
    // print("pop %\n",stackPtr-stack);
    --stackPtr;
    for (uniform int i=0;i<3;i++) {
      unmasked {
        act_lo[i] = stackPtr->act_lo[i];
        act_hi[i] = stackPtr->act_hi[i];
      }
    }
    nodeID = stackPtr->nodeID;
  }
//   while (stackPtr > stack) {
//     --stackPtr;
    
//     const uniform uint32 nodeID = stackPtr->nodeID;
//     const uniform KDTreeNode &node = self->node[nodeID];
//     const uniform uint32 childID = getOfs(node);
//     if (isLeaf(node)) {
//       if (numLeaves >= (programCount * 8)) {
//         print("num %\n",numLeaves);
//         for (uniform int i=0;i<numLeaves;i++) {
//           Leaf *leaf = self->leaf[leaftList[i]];
//           print("leaf[%] = % % % - % % %\n",
//                 i,
//                 leaf->lower.x,
//                 leaf->lower.y,
//                 leaf->lower.z,
//                 leaf->upper.x,
//                 leaf->upper.y,
//                 leaf->upper.z);
//         }
//       }
//       assert(numLeaves < (programCount * 8));
//       leafList[numLeaves++] = childID;
//     } else {
//       const uniform int dim = getDim(node);
//       const uniform float pos = getPos(node);
//       const bool goLeft  = stackPtr->active & (min(p0[dim],p1[dim]) < pos);
//       const bool goRight = stackPtr->active & (max(p0[dim],p1[dim]) >= pos);
      
//       if (goRight)
//         stackPtr = pushStack(stackPtr,childID+1,goRight);
//       if (goLeft)
//         stackPtr = pushStack(stackPtr,childID,goLeft);
//     }
//   }
// #else
//   while (stackPtr > stack) {
//     --stackPtr;
//     if (stackPtr->active) {
//       uniform uint32 nodeID = stackPtr->nodeID;
//       while (1) {
//         const uniform KDTreeNode &node = self->node[nodeID];
//         const uniform uint32 childID = getOfs(node);
//         if (isLeaf(node)) {
//           assert(numLeaves < (programCount * 8));
//           leafList[numLeaves++] = childID;
//           break;
//         } else {
//           const uniform int dim = getDim(node);
//           const uniform float pos = getPos(node);
//           const bool goLeft  = min(p0[dim],p1[dim]) < pos;
//           const bool goRight = max(p0[dim],p1[dim]) >= pos;

//           if (none(goLeft)) {
//             nodeID = childID+1;
//           } else if (none(goRight)) {
//             nodeID = childID;
//           } else {
//             stackPtr = pushStack(stackPtr,childID+1,goRight);
//             if (!goLeft) break;
//             nodeID = childID;
//           }
//         }
//       }
//     }
//   }
// #endif

  // print("numleaves %\n",numLeaves);
  // -------------------------------------------------------
  // now, process leaves we found
  // -------------------------------------------------------
  foreach_unique (desired_width in tgtWidth) {
    // print("width %\n",desired_width);
    for (uniform int leafID=0;leafID<numLeaves;leafID++) {
      // print("leafID %\n",leafID);
      const ChomboLeaf *uniform leaf = &self->leaf[leafList[leafID]];
      // print("leaf %\n",leaf);
      // assert(leaf);
      const bool valid_x0 = _P0.x >= leaf->bounds.lower.x & _P0.x < leaf->bounds.upper.x;
      const bool valid_y0 = _P0.y >= leaf->bounds.lower.y & _P0.y < leaf->bounds.upper.y;
      const bool valid_z0 = _P0.z >= leaf->bounds.lower.z & _P0.z < leaf->bounds.upper.z;
      
      const bool valid_x1 = _P1.x >= leaf->bounds.lower.x & _P1.x < leaf->bounds.upper.x;
      const bool valid_y1 = _P1.y >= leaf->bounds.lower.y & _P1.y < leaf->bounds.upper.y;
      const bool valid_z1 = _P1.z >= leaf->bounds.lower.z & _P1.z < leaf->bounds.upper.z;

      const bool anyValid = 
        (valid_x0 | valid_x1) &
        (valid_y0 | valid_y1) & 
        (valid_z0 | valid_z1);
      if (!(anyValid))
        continue;

      uniform int brickID = 0;
      uniform bool isLeaf = true;
      const ChomboBrick *uniform brick = leaf->brickList[brickID];
      while (brick->cellWidth < desired_width) {
        brick = leaf->brickList[++brickID];
        if (brick == NULL) 
          print("invalid brick!\n");
        isLeaf = false;
      }
      
      const float *uniform v = brick->value;
      const vec3f rp0 = (_P0 - brick->bounds.lower) * brick->bounds_scale;
      const vec3f rp1 = (_P1 - brick->bounds.lower) * brick->bounds_scale;
      
      const vec3f f_bc0 = floor(rp0 * brick->f_dims);
      const vec3f f_bc1 = floor(rp1 * brick->f_dims);

      // index offsets to neighbor cells
      const float f_idx_dx0 = f_bc0.x;
      const float f_idx_dy0 = f_bc0.y*brick->f_dims.x;
      const float f_idx_dz0 = f_bc0.z*brick->f_dims.x*brick->f_dims.y;
      
      const float f_idx_dx1 = f_bc1.x;
      const float f_idx_dy1 = f_bc1.y*brick->f_dims.x;
      const float f_idx_dz1 = f_bc1.z*brick->f_dims.x*brick->f_dims.y;
      
#define DOCORNER(X,Y,Z)                                                 \
      if (valid_z##Z & valid_y##Y & valid_x##X) {                       \
        const int idx = (int)(f_idx_dx##X+f_idx_dy##Y+f_idx_dz##Z);     \
        set(oct.c[C##Z##Y##X],v[idx],brick->cellWidth,isLeaf);          \
      }                                                                 \
        // assert(idx >= 0);                                               \
        // assert(idx < brick->dims.x*brick->dims.y*brick->dims.z);        \
      
      DOCORNER(0,0,0);
      DOCORNER(0,0,1);
      DOCORNER(0,1,0);
      DOCORNER(0,1,1);
      DOCORNER(1,0,0);
      DOCORNER(1,0,1);
      DOCORNER(1,1,0);
      DOCORNER(1,1,1);
      // print("done corners\n");
    }
  }
  // for (uniform int i=0;i<8;i++)
  //   assert(oct.c[i].width > 0.f);
  //   print("c[%]\nv %\nw %\n",i,oct.c[i].value,oct.c[i].width);
  // print("DONE\n");
}







