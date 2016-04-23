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

void findEightCellsNotOrdered(const Chombo *uniform self,
                              Octant &oct,
                              const float tgtWidth)
{
#if 0
  set(oct.c[C000],findLeafCell(self,make_vec3f(oct.P0.x,oct.P0.y,oct.P0.z)));
  set(oct.c[C001],findLeafCell(self,make_vec3f(oct.P1.x,oct.P0.y,oct.P0.z)));

  set(oct.c[C010],findLeafCell(self,make_vec3f(oct.P0.x,oct.P1.y,oct.P0.z)));
  set(oct.c[C011],findLeafCell(self,make_vec3f(oct.P1.x,oct.P1.y,oct.P0.z)));

  set(oct.c[C100],findLeafCell(self,make_vec3f(oct.P0.x,oct.P0.y,oct.P1.z)));
  set(oct.c[C101],findLeafCell(self,make_vec3f(oct.P1.x,oct.P0.y,oct.P1.z)));

  set(oct.c[C110],findLeafCell(self,make_vec3f(oct.P0.x,oct.P1.y,oct.P1.z)));
  set(oct.c[C111],findLeafCell(self,make_vec3f(oct.P1.x,oct.P1.y,oct.P1.z)));
  return;
#endif
  // clear(o);
  const uniform float hcw = 0.5f*self->finestLevelCellWidth;
  const vec3f _P0 = clamp(oct.P0,
                          make_vec3f(0.f),
                          self->bounds.upper-make_vec3f(hcw));
  const vec3f _P1 = clamp(oct.P1,
                          make_vec3f(0.f),
                          self->bounds.upper-make_vec3f(hcw));
  const varying float *const uniform p0 = &_P0.x;
  const varying float *const uniform p1 = &_P1.x;

  uniform FindStack stack[64];
  uniform FindStack *uniform stackPtr = pushStack(&stack[0],0,true);
  
  uniform int32 leafList[programCount*8];
  uniform int32 numLeaves = 0;

  while (stackPtr > stack) {
    --stackPtr;
    if (stackPtr->active) {
      uniform uint32 nodeID = stackPtr->nodeID;
      while (1) {
        const uniform KDTreeNode &node = self->node[nodeID];
        const uniform uint32 childID = getOfs(node);
        if (isLeaf(node)) {

          // if (numLeaves == programCount * 8) {
          //   print("OVERFLOW OF LEAVES!?\n");
          //   print("P0 ");out(_P0);print("\n");
          //   print("P1 ");out(_P1);print("\n");
          //   for (uniform int i=0;i<numLeaves;i++) {
          //     const ChomboLeaf *uniform leaf = &self->leaf[childID];
          //     print("leaf % : % % % - % % %\n",
          //           i,
          //           leaf->bounds.lower.x,
          //           leaf->bounds.lower.y,
          //           leaf->bounds.lower.z,
          //           leaf->bounds.upper.x,
          //           leaf->bounds.upper.y,
          //           leaf->bounds.upper.z);
          //   }
          // }


          // const ChomboLeaf *uniform leaf = &self->leaf[childID];
          
          // const bool valid_x0 = _P0.x >= leaf->bounds.lower.x & _P0.x < leaf->bounds.upper.x;
          // const bool valid_y0 = _P0.y >= leaf->bounds.lower.y & _P0.y < leaf->bounds.upper.y;
          // const bool valid_z0 = _P0.z >= leaf->bounds.lower.z & _P0.z < leaf->bounds.upper.z;
          
          // const bool valid_x1 = _P1.x >= leaf->bounds.lower.x & _P1.x < leaf->bounds.upper.x;
          // const bool valid_y1 = _P1.y >= leaf->bounds.lower.y & _P1.y < leaf->bounds.upper.y;
          // const bool valid_z1 = _P1.z >= leaf->bounds.lower.z & _P1.z < leaf->bounds.upper.z;

          
          // const bool anyValid = 
          //   valid_x0 | valid_x1 | 
          //   valid_y0 | valid_y1 | 
          //   valid_z0 | valid_z1;
          // if (!(anyValid))
          //   break;

          leafList[numLeaves++] = childID;
          break;
        } else {
          const uniform int dim = getDim(node);
          const uniform float pos = getPos(node);
          const bool goLeft  = min(p0[dim],p1[dim]) < pos;
          const bool goRight = max(p0[dim],p1[dim]) >= pos;

          if (none(goLeft)) {
            nodeID = childID+1;
          } else if (none(goRight)) {
            nodeID = childID;
          } else {
            stackPtr = pushStack(stackPtr,childID+1,goRight);
            if (!goLeft) break;
            nodeID = childID;
          }
        }
      }
    }
  }

  // -------------------------------------------------------
  // now, process leaves we found
  // -------------------------------------------------------
  foreach_unique (desired_width in tgtWidth) {
    for (uniform int leafID=0;leafID<numLeaves;leafID++) {
      const ChomboLeaf *uniform leaf = &self->leaf[leafList[leafID]];

      const bool valid_x0 = _P0.x >= leaf->bounds.lower.x & _P0.x < leaf->bounds.upper.x;
      const bool valid_y0 = _P0.y >= leaf->bounds.lower.y & _P0.y < leaf->bounds.upper.y;
      const bool valid_z0 = _P0.z >= leaf->bounds.lower.z & _P0.z < leaf->bounds.upper.z;
      
      const bool valid_x1 = _P1.x >= leaf->bounds.lower.x & _P1.x < leaf->bounds.upper.x;
      const bool valid_y1 = _P1.y >= leaf->bounds.lower.y & _P1.y < leaf->bounds.upper.y;
      const bool valid_z1 = _P1.z >= leaf->bounds.lower.z & _P1.z < leaf->bounds.upper.z;

      const bool anyValid = 
        valid_x0 | valid_x1 | 
        valid_y0 | valid_y1 | 
        valid_z0 | valid_z1;
      if (!(anyValid))
        continue;

      uniform int brickID = 0;
      uniform bool isLeaf = true;
      const ChomboBrick *uniform brick = leaf->brickList[brickID];
      while (brick->cellWidth < desired_width) {
        brick = leaf->brickList[++brickID];
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
        set(oct.c[C##Z##Y##X],v[idx],brick->cellWidth,isLeaf);            \
      }                                                                 \
      
      DOCORNER(0,0,0);
      DOCORNER(0,0,1);
      DOCORNER(0,1,0);
      DOCORNER(0,1,1);
      DOCORNER(1,0,0);
      DOCORNER(1,0,1);
      DOCORNER(1,1,0);
      DOCORNER(1,1,1);
    }
  }
}







