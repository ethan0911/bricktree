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
#include "CompOctant.ih"

void findEightCellsNotOrdered(const Chombo *uniform self,
                              CompactOctant &o,
                              const float minWidth)
{
  clear(o);
  const vec3f _P0 = clamp(o.P0,
                          make_vec3f(0.f),
                          self->bounds.upper-make_vec3f(0.5f*self->finestLevelCellWidth));
  const vec3f _P1 = clamp(o.P1,
                          make_vec3f(0.f),
                          self->bounds.upper-make_vec3f(0.5f*self->finestLevelCellWidth));
  // print("P0:");out(_P0);print("\n");
  // print("P1:");out(_P1);print("\n");
  const varying float *const uniform p0 = &_P0.x;
  const varying float *const uniform p1 = &_P1.x;

  uniform FindStack stack[16];
  uniform FindStack *uniform stackPtr = pushStack(&stack[0],0,true);
  
  uniform int32 leafList[64];
  uniform int32 numLeaves = 0;

  while (stackPtr > stack) {
    --stackPtr;
    // print("Popping % %\n",stackPtr-stack,stackPtr->active);
    if (stackPtr->active) {
      uniform uint32 nodeID = stackPtr->nodeID;
      while (1) {
        const uniform KDTreeNode &node = self->node[nodeID];
        const uniform uint32 childID = getOfs(node);
        // print("node % child %\n",nodeID,childID);
        if (isLeaf(node)) {
          leafList[numLeaves++] = childID;
          break;
        } else {
          const uniform int dim = getDim(node);
          const varying float pos = getPos(node);
          const bool goLeft  = min(p0[dim],p1[dim]) < pos;
          const bool goRight = max(p0[dim],p1[dim]) >= pos;
          // print("plane % %\nL%\nR%\n",dim,pos,goLeft,goRight);
          
          if (any(goLeft)) {
            if (any(goRight)) 
              stackPtr = pushStack(stackPtr,childID+1,goRight);

            if (!goLeft) 
              break;
            nodeID = childID;
          } else {
            if (!goRight)
              break;
            nodeID = childID+1;
          }
        }
      }
    }
  }

  // print("numleaves %\n",numLeaves);
  for (uniform int leafID=0;leafID<numLeaves;leafID++) {
    for (uniform int i=leafList[leafID];any(true);i++) {
      const ChomboBrick *uniform brick = self->item[i];
      if (brick->cellWidth >= minWidth) {
        // print("brick width %\n",brick->cellWidth);
        const float *uniform v = brick->value;
        const vec3f rp0 = (o.P0 - brick->bounds.lower) * brick->bounds_scale;
        const vec3f rp1 = (o.P1 - brick->bounds.lower) * brick->bounds_scale;

        const bool valid_x0 = _P0.x >= brick->bounds.lower.x & _P0.x < brick->bounds.upper.x;
        const bool valid_x1 = _P1.x >= brick->bounds.lower.x & _P1.x < brick->bounds.upper.x;
        const bool valid_y0 = _P0.y >= brick->bounds.lower.y & _P0.y < brick->bounds.upper.y;
        const bool valid_y1 = _P1.y >= brick->bounds.lower.y & _P1.y < brick->bounds.upper.y;
        const bool valid_z0 = _P0.z >= brick->bounds.lower.z & _P0.z < brick->bounds.upper.z;
        const bool valid_z1 = _P1.z >= brick->bounds.lower.z & _P1.z < brick->bounds.upper.z;
        
        const vec3f f_bc0 = floor(rp0 * brick->f_dims);
        const vec3f f_bc1 = floor(rp1 * brick->f_dims);
        
        // index offsets to neighbor cells
        const float f_idx_dx0 = f_bc0.x;
        const float f_idx_dy0 = f_bc0.y*brick->f_dims.x;
        const float f_idx_dz0 = f_bc0.z*brick->f_dims.x*brick->f_dims.y;

        const float f_idx_dx1 = f_bc1.x;
        const float f_idx_dy1 = f_bc1.y*brick->f_dims.x;
        const float f_idx_dz1 = f_bc1.z*brick->f_dims.x*brick->f_dims.y;


        if (valid_z0) {
        
          if (valid_x0 & valid_y0) {
            o.width[0][0][0] = brick->cellWidth;
            o.value[0][0][0] = v[(int)(f_idx_dx0+f_idx_dy0+f_idx_dz0)];
          }
          if (valid_x1 & valid_y0) {
            o.width[0][0][1] = brick->cellWidth;
            o.value[0][0][1] = v[(int)(f_idx_dx1+f_idx_dy0+f_idx_dz0)];
          }

          if (valid_x0 & valid_y1) {
            o.width[0][1][0] = brick->cellWidth;
            o.value[0][1][0] = v[(int)(f_idx_dx0+f_idx_dy1+f_idx_dz0)];
          }
          if (valid_x1 & valid_y1) {
            o.width[0][1][1] = brick->cellWidth;
            o.value[0][1][1] = v[(int)(f_idx_dx1+f_idx_dy1+f_idx_dz0)];
          }
        }


        if (valid_z1) {
          if (valid_x0 & valid_y0) {
            o.width[1][0][0] = brick->cellWidth;
            o.value[1][0][0] = v[(int)(f_idx_dx0+f_idx_dy0+f_idx_dz1)];
          }
          if (valid_x1 & valid_y0) {
            o.width[1][0][1] = brick->cellWidth;
            o.value[1][0][1] = v[(int)(f_idx_dx1+f_idx_dy0+f_idx_dz1)];
          }

          if (valid_x0 & valid_y1) {
            o.width[1][1][0] = brick->cellWidth;
            o.value[1][1][0] = v[(int)(f_idx_dx0+f_idx_dy1+f_idx_dz1)];
          }
          if (valid_x1 & valid_y1) {
            o.width[1][1][1] = brick->cellWidth;
            o.value[1][1][1] = v[(int)(f_idx_dx1+f_idx_dy1+f_idx_dz1)];
          }
        }
        break;
      }
    }
  }
}





/*! find the eight corner values given by all combinations of
    {P0,P1}{xyz}. do NOT set the width values, and always traverse
    down to the leaf level */
extern void findEightCornerValuesOrdered(const Chombo *uniform self,
                                         CompactOctant &o)
{
  const vec3f _P0 = clamp(o.P0,
                          make_vec3f(0.f),
                          self->bounds.upper-make_vec3f(0.5f*self->finestLevelCellWidth));
  const vec3f _P1 = clamp(o.P1,
                          make_vec3f(0.f),
                          self->bounds.upper-make_vec3f(0.5f*self->finestLevelCellWidth));
  const varying float *const uniform p0 = &_P0.x;
  const varying float *const uniform p1 = &_P1.x;

  uniform FindStack stack[64];
  uniform FindStack *uniform stackPtr = pushStack(&stack[0],0,true);
  
  while (stackPtr > stack) {
    --stackPtr;
    if (stackPtr->active) {
      uniform uint32 nodeID = stackPtr->nodeID;
      while (1) {
        const uniform KDTreeNode &node = self->node[nodeID];
        const uniform uint32 childID = getOfs(node);

        if (isLeaf(node)) {
          const ChomboBrick *uniform brick = self->item[childID];
          const float *uniform v = brick->value;
          
          const bool valid_x0 = _P0.x >= brick->bounds.lower.x;
          const bool valid_x1 = _P1.x < brick->bounds.upper.x;
          const bool valid_y0 = _P0.y >= brick->bounds.lower.y;
          const bool valid_y1 = _P1.y < brick->bounds.upper.y;
          const bool valid_z0 = _P0.z >= brick->bounds.lower.z;
          const bool valid_z1 = _P1.z < brick->bounds.upper.z;

          const float f_bc_x0 = floor((o.P0.x-brick->bounds.lower.x)*brick->bounds_scale.x*brick->f_dims.x);
          const float f_bc_x1 = floor((o.P1.x-brick->bounds.lower.x)*brick->bounds_scale.x*brick->f_dims.x);
          const float f_bc_y0 = floor((o.P0.y-brick->bounds.lower.y)*brick->bounds_scale.y*brick->f_dims.y);
          const float f_bc_y1 = floor((o.P1.y-brick->bounds.lower.y)*brick->bounds_scale.y*brick->f_dims.y);
          
          // index offsets to neighbor cells
          const float f_idx_dx0 = f_bc_x0;
          const float f_idx_dy0 = f_bc_y0*brick->f_dims.x;
          
          const float f_idx_dx1 = f_bc_x1;
          const float f_idx_dy1 = f_bc_y1*brick->f_dims.x;
          
          
          if (valid_z0) {
            const float f_bc_z0 = floor((o.P0.z-brick->bounds.lower.z)*brick->bounds_scale.z*brick->f_dims.z);
            
            const float f_idx_dz0 = f_bc_z0*brick->f_dims.x*brick->f_dims.y;
            
            if (valid_x0 & valid_y0) 
              o.value[0][0][0] = v[(int)(f_idx_dx0+f_idx_dy0+f_idx_dz0)];
            if (valid_x1 & valid_y0) 
              o.value[0][0][1] = v[(int)(f_idx_dx1+f_idx_dy0+f_idx_dz0)];
            
            if (valid_x0 & valid_y1) 
              o.value[0][1][0] = v[(int)(f_idx_dx0+f_idx_dy1+f_idx_dz0)];
            if (valid_x1 & valid_y1) 
              o.value[0][1][1] = v[(int)(f_idx_dx1+f_idx_dy1+f_idx_dz0)];
          }
          
          
          if (valid_z1) {
            const float f_bc_z1 = floor((o.P1.z-brick->bounds.lower.z)*brick->bounds_scale.z*brick->f_dims.z);
            const float f_idx_dz1 = f_bc_z1*brick->f_dims.x*brick->f_dims.y;
            
            if (valid_x0 & valid_y0) 
              o.value[1][0][0] = v[(int)(f_idx_dx0+f_idx_dy0+f_idx_dz1)];
            if (valid_x1 & valid_y0) 
              o.value[1][0][1] = v[(int)(f_idx_dx1+f_idx_dy0+f_idx_dz1)];
            
            if (valid_x0 & valid_y1) 
              o.value[1][1][0] = v[(int)(f_idx_dx0+f_idx_dy1+f_idx_dz1)];
            if (valid_x1 & valid_y1) 
              o.value[1][1][1] = v[(int)(f_idx_dx1+f_idx_dy1+f_idx_dz1)];
          }
          
          break;
        } else {
          const uniform uint32 dim = getDim(node);
          const varying float pos = getPos(node);
          const bool goLeft  = p0[dim] < pos;
          const bool goRight = p1[dim] >= pos;
          
          if (any(goLeft)) {
            if (any(goRight)) 
              stackPtr = pushStack(stackPtr,childID+1,goRight);

            nodeID = childID;
            if (!goLeft) 
              break;
          } else {
            if (!goRight)
              break;
            nodeID = childID+1;
          }
        }
      }
    }
  }
  
  // for (uniform uint32 leafID=0;leafID<numLeaves;leafID++) {
  //   const ChomboBrick *uniform brick = self->item[leafList[leafID]];
  //   const float *uniform v = brick->value;
    
  //   const bool valid_x0 = _P0.x >= brick->bounds.lower.x & _P0.x < brick->bounds.upper.x;
  //   const bool valid_x1 = _P1.x >= brick->bounds.lower.x & _P1.x < brick->bounds.upper.x;
  //   const bool valid_y0 = _P0.y >= brick->bounds.lower.y & _P0.y < brick->bounds.upper.y;
  //   const bool valid_y1 = _P1.y >= brick->bounds.lower.y & _P1.y < brick->bounds.upper.y;
  //   const bool valid_z0 = _P0.z >= brick->bounds.lower.z & _P0.z < brick->bounds.upper.z;
  //   const bool valid_z1 = _P1.z >= brick->bounds.lower.z & _P1.z < brick->bounds.upper.z;

  //   const float f_bc_x0 = floor((o.P0.x-brick->bounds.lower.x)*brick->bounds_scale.x*brick->f_dims.x);
  //   const float f_bc_x1 = floor((o.P1.x-brick->bounds.lower.x)*brick->bounds_scale.x*brick->f_dims.x);
  //   const float f_bc_y0 = floor((o.P0.y-brick->bounds.lower.y)*brick->bounds_scale.y*brick->f_dims.y);
  //   const float f_bc_y1 = floor((o.P1.y-brick->bounds.lower.y)*brick->bounds_scale.y*brick->f_dims.y);
    
  //   // index offsets to neighbor cells
  //   const float f_idx_dx0 = f_bc_x0;
  //   const float f_idx_dy0 = f_bc_y0*brick->f_dims.x;

  //   const float f_idx_dx1 = f_bc_x1;
  //   const float f_idx_dy1 = f_bc_y1*brick->f_dims.x;
    
    
  //   if (valid_z0) {
  //     const float f_bc_z0 = floor((o.P0.z-brick->bounds.lower.z)*brick->bounds_scale.z*brick->f_dims.z);

  //     const float f_idx_dz0 = f_bc_z0*brick->f_dims.x*brick->f_dims.y;
      
  //     if (valid_x0 & valid_y0) {
  //       //o.width[0][0][0] = brick->cellWidth;
  //       o.value[0][0][0] = v[(int)(f_idx_dx0+f_idx_dy0+f_idx_dz0)];
  //     }
  //     if (valid_x1 & valid_y0) {
  //       //o.width[0][0][1] = brick->cellWidth;
  //       o.value[0][0][1] = v[(int)(f_idx_dx1+f_idx_dy0+f_idx_dz0)];
  //     }
      
  //     if (valid_x0 & valid_y1) {
  //       //o.width[0][1][0] = brick->cellWidth;
  //       o.value[0][1][0] = v[(int)(f_idx_dx0+f_idx_dy1+f_idx_dz0)];
  //     }
  //     if (valid_x1 & valid_y1) {
  //       //o.width[0][1][1] = brick->cellWidth;
  //       o.value[0][1][1] = v[(int)(f_idx_dx1+f_idx_dy1+f_idx_dz0)];
  //     }
  //   }
    
    
  //   if (valid_z1) {
  //     const float f_bc_z1 = floor((o.P1.z-brick->bounds.lower.z)*brick->bounds_scale.z*brick->f_dims.z);
  //     const float f_idx_dz1 = f_bc_z1*brick->f_dims.x*brick->f_dims.y;

  //     // const uint32 bc_z1 = (uint32)((o.P1.z-brick->bounds.lower.z)*brick->bounds_scale.z*brick->f_dims.z);
  //     if (valid_x0 & valid_y0) {
  //       //o.width[1][0][0] = brick->cellWidth;
  //       o.value[1][0][0] = v[(int)(f_idx_dx0+f_idx_dy0+f_idx_dz1)];
  //     }
  //     if (valid_x1 & valid_y0) {
  //       //o.width[1][0][1] = brick->cellWidth;
  //       o.value[1][0][1] = v[(int)(f_idx_dx1+f_idx_dy0+f_idx_dz1)];
  //     }
      
  //     if (valid_x0 & valid_y1) {
  //       //o.width[1][1][0] = brick->cellWidth;
  //       o.value[1][1][0] = v[(int)(f_idx_dx0+f_idx_dy1+f_idx_dz1)];
  //     }
  //     if (valid_x1 & valid_y1) {
  //       //o.width[1][1][1] = brick->cellWidth;
  //       o.value[1][1][1] = v[(int)(f_idx_dx1+f_idx_dy1+f_idx_dz1)];
  //     }
  //   }
  // }
}
