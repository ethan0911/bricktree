#include "CompOctant.ih"


// /*! initial find operation for a small octant: this will traverse down
//   to the leaf of 'samplePos', and fill in at least oct[0,0,0]. If
//   any other cells on the given leaf brick fit any other corners of
//   this octant, they will get filled in, too */
// extern void initialFind(const Chombo *uniform self,
//                         CompactOctant &oct,
//                         const varying vec3f &pos,
//                         const float minWidth)
// {
//   clear(oct);
//   const varying float *const uniform  samplePos = &pos.x;
  
//   const uint32 kdLeafNodeID = findLeaf(self,pos);
//   foreach_unique (nodeID in kdLeafNodeID) {
//     const uniform KDTreeNode &node = self->node[nodeID];
//     for (uniform int i=getOfs(node);any(true);i++) {
//       const ChomboBrick *uniform brick = self->item[i];
//       if (brick->cellWidth >= minWidth) {
//         // //print("enter leaf\n");
//         const float *uniform v = brick->value;
//         const vec3f relBrickPos
//           = (pos - brick->bounds.lower) * brick->bounds_scale;
//         // brick coords: integer cell coordinates inside brick
//         // OPT: the same calculations as below, just in
//         // floats. this works as long as all values we calculate
//         // with are fraction-less values (so essentially ints) and
//         // fit into 24 bits mantissa (which they easily should for
//         // any brick
//         const float cw = brick->cellWidth;
//         const vec3f f_bc = floor(relBrickPos * brick->f_dims);
//         const float f_idx = f_bc.x + brick->f_dims.x*(f_bc.y+brick->f_dims.y*(f_bc.z));
//         const uint32 idx = (int)f_idx;
//         const vec3f P0 = brick->bounds.lower + f_bc*cw;
//         oct.P0 = P0;
//         oct.width[0][0][0] = cw;
//         oct.value[0][0][0] = v[idx];
        
//         const vec3f lcc = centerOf(oct);
//         // octant signs:
//         const float sx = pos.x < lcc.x ? -1.f : +1.f;
//         const float sy = pos.y < lcc.y ? -1.f : +1.f;
//         const float sz = pos.z < lcc.z ? -1.f : +1.f;
        
//         // index offsets to neighbor cells
//         const float f_idx_dx = sx;
//         const float f_idx_dy = sy*brick->f_dims.x;
//         const float f_idx_dz = sz*brick->f_dims.x*brick->f_dims.y;
        
//         // coordinates of neighbor
//         const vec3f P1 = P0+make_vec3f(sx,sy,sz)*cw;
//         oct.P1 = P1;

//         const bool dx_valid = P1.x >= brick->bounds.lower.x & P1.x < brick->bounds.upper.x;
//         const bool dy_valid = P1.y >= brick->bounds.lower.y & P1.y < brick->bounds.upper.y;
//         const bool dz_valid = P1.z >= brick->bounds.lower.z & P1.z < brick->bounds.upper.z;
        
//         if (dx_valid) {
//           oct.width[0][0][1] = cw;
//           oct.value[0][0][1] = v[(int)(f_idx+f_idx_dx)];
//         }
//         if (dy_valid) {
//           oct.value[0][1][0] = v[(int)(f_idx+f_idx_dy)];
//           oct.width[0][1][0] = cw;
//           if (dx_valid) {
//             oct.value[0][1][1] = v[(int)(f_idx+f_idx_dx+f_idx_dy)];
//             oct.width[0][1][1] = cw;
//           }
//         }
//         if (dz_valid) {
//           oct.value[1][0][0] = v[(int)(f_idx+f_idx_dz)];
//           oct.width[1][0][0] = cw;
//           if (dx_valid) {
//             oct.value[1][0][1] = v[(int)(f_idx+f_idx_dx+f_idx_dz)];
//             oct.width[1][0][1] = cw;
//           }
//           if (dy_valid) {
//             oct.value[1][1][0] = v[(int)(f_idx+f_idx_dy+f_idx_dz)];
//             oct.width[1][1][0] = cw;
//             if (dx_valid) {
//               oct.value[1][1][1] = v[(int)(f_idx+f_idx_dx+f_idx_dy+f_idx_dz)];
//               oct.width[1][1][1] = cw;
//             }
//           }
//         }
//         break;
//       }
//     }
//   }
// }



















// /*! fill in addiitonal corners, where applicable. 'nextPos' must be a cellRef */
// extern void findOthers(const Chombo *uniform self,
//                        CompactOctant &o,
//                        const vec3f &pos,
//                        const float minWidth)
// {
//   const varying float *const uniform  samplePos = &pos.x;
//   const vec3f P0 = o.P0;
//   const vec3f P1 = clamp(o.P1,
//                          make_vec3f(0.f),
//                          self->bounds.upper-make_vec3f(self->finestLevelCellWidth));
//   const uint32 kdLeafNodeID = findLeaf(self,pos);
//   foreach_unique (nodeID in kdLeafNodeID) {
//     const uniform KDTreeNode &node = self->node[nodeID];
//     for (uniform int i=getOfs(node);any(true);i++) {
//       const ChomboBrick *uniform brick = self->item[i];
//       if (brick->cellWidth >= minWidth) {
//         const bool x0valid = (P0.x >= brick->bounds.lower.x) & (P0.x < brick->bounds.upper.x);
//         const bool x1valid = (P1.x >= brick->bounds.lower.x) & (P1.x < brick->bounds.upper.x);

//         const bool y0valid = (P0.y >= brick->bounds.lower.y) & (P0.y < brick->bounds.upper.y);
//         const bool y1valid = (P1.y >= brick->bounds.lower.y) & (P1.y < brick->bounds.upper.y);
        
//         const bool z0valid = (P0.z >= brick->bounds.lower.z) & (P0.z < brick->bounds.upper.z);
//         const bool z1valid = (P1.z >= brick->bounds.lower.z) & (P1.z < brick->bounds.upper.z);

//         const float *uniform v = brick->value;
//         const vec3f relP0 = (P0 - brick->bounds.lower) * brick->bounds_scale;
//         const vec3f relP1 = (P1 - brick->bounds.lower) * brick->bounds_scale;

//         const vec3f f_bc0 = floor(relP0 * brick->f_dims);
//         const vec3f f_bc1 = floor(relP1 * brick->f_dims);
        
//         const float f_x0 = f_bc0.x;
//         const float f_x1 = f_bc1.x;

//         const float f_y0 = f_bc0.y * brick->f_dims.x;
//         const float f_y1 = f_bc1.y * brick->f_dims.x;

//         const float f_z0 = f_bc0.z * brick->f_dims.x * brick->f_dims.y;
//         const float f_z1 = f_bc1.z * brick->f_dims.x * brick->f_dims.y;

//         const float cw = brick->cellWidth;

//         if (z0valid) {
//           if (y0valid) {
//             if (x0valid) {
//               o.width[0][0][0] = cw;
//               o.value[0][0][0] = v[(int)(f_x0+f_y0+f_z0)];
//             }
//             if (x1valid) {
//               o.width[0][0][1] = cw;
//               o.value[0][0][1] = v[(int)(f_x1+f_y0+f_z0)];
//             }
//           } 
//           if (y1valid) {
//             if (x0valid) {
//               o.width[0][1][0] = cw;
//               o.value[0][1][0] = v[(int)(f_x0+f_y1+f_z0)];
//             }
//             if (x1valid) {
//               o.width[0][1][1] = cw;
//               o.value[0][1][1] = v[(int)(f_x1+f_y1+f_z0)];
//             }
//           }
//         }

//         if (z1valid) {
//           if (y0valid) {
//             if (x0valid) {
//               o.width[1][0][0] = cw;
//               o.value[1][0][0] = v[(int)(f_x0+f_y0+f_z1)];
//             }
//             if (x1valid) {
//               o.width[1][0][1] = cw;
//               o.value[1][0][1] = v[(int)(f_x1+f_y0+f_z1)];
//             }
//           }
//           if (y1valid) {
//             if (x0valid) {
//               o.width[1][1][0] = cw;
//               o.value[1][1][0] = v[(int)(f_x0+f_y1+f_z1)];
//             }
//             if (x1valid) {
//               o.width[1][1][1] = cw;
//               o.value[1][1][1] = v[(int)(f_x1+f_y1+f_z1)];
//             }
//           }
//         }

//         break;
//       }
//     }
//   }
//   //print("/findothers\n");
// }
