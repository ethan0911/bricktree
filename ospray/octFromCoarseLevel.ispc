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

#include "Octant.ih"
#include "ChomboVolume.ih"


/* the octant method in which the boundary is defined by the coarser
   side - this is 'correct' in that it is interpolating and continuous
   even across level boundaries. it can, however, lead to weird
   patterns at boundaries in that the coarse levels can 'bleed' across
   the boundary.

   To explain this pattern, imagine a boundary between a coarse region
   on the left and a fine one on the right. The left has a constant 0,
   the right is refined to (0,1,1,1). The fine region would also have
   a representation on the coarse level, which would be
   0.75=avg(0,1,1,1). Now the value we'd expect at the boundary is 0,
   because both the coarse cell on the left is 0, as well as the
   left-most of the fine values. However, at the boundary we'd let
   interpolation on the coarse level define the value, which would be
   the average of the coarse cell on the left (0) and that on the
   right (0.75), which is 0.385. Ie, gonig from the left to the right
   we'd see a function that is first 0 (in the center of the coarse
   cell), would then interpolate up to 0.385 (interpolated value on
   coarse level) at the boundary, would then interpolate back down to
   0 again (left-most value on fine level), and then interpolate back
   up to 1 (next value(s) on the fine level). This 'ghost' spike at
   0.38 gives a visible artifact.
 */
extern float Chombo_octFromCoarseLevel(void *uniform _self, const vec3f &pos)
{
  const ChomboVolume *uniform self = (const ChomboVolume *uniform)_self;
  const Chombo *uniform chom = &self->chombo;

  // ------------------------------------------------------------------
  // determine cell and octant
  // ------------------------------------------------------------------
  CellRef cell = findCell(chom,pos,0.f);
  const vec3f cc = centerOf(cell);
  const vec3f sign = make_vec3f(pos.x < cc.x ? -1.f : +1.f,
                                pos.y < cc.y ? -1.f : +1.f,
                                pos.z < cc.z ? -1.f : +1.f);
  const vec3f sideID = make_vec3f(pos.x < cc.x ? 0.f : 1.f,
                                  pos.y < cc.y ? 0.f : 1.f,
                                  pos.z < cc.z ? 0.f : 1.f);
  const vec3f sidePos = cell.pos + sideID*cell.width;
  Octant oct;
  oct.P0 = pos;
  oct.P1 = nextafter(pos + sign * make_vec3f(cell.width),sign);
  
  // ------------------------------------------------------------------
  // determine all eight neighbors on cell's level
  // ------------------------------------------------------------------
  findEightCellsNotOrdered(chom,oct,cell.width);

  // ------------------------------------------------------------------
  // check if we're at a boundary to a COARSER region. for this
  // method, it's fine if the neighbors are finer (since in this case
  // _we_ are the coarser ones that define the boundary value; but if
  // the neighbor if coarser then it's the neighor that defines the
  // value.
  // ------------------------------------------------------------------
  for (uniform int i=1;i<8;i++) {
    if (oct.c[i].width != oct.c[0].width) {
      const vec3f cornerPos = make_vec3f((i&1)?oct.P1.x:oct.P0.x,
                                         (i&2)?oct.P1.y:oct.P0.y,
                                         (i&4)?oct.P1.z:oct.P0.z);
      oct.c[i].value = Chombo_octFromCoarseLevel(_self,cornerPos);
    }
  }
  
  const float rcpWidth = 1.f/oct.c[C000].width;
  return lerp(oct,abs(pos-cc)*rcpWidth);
}
