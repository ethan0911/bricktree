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

#include "CompOctant.ih"

inline float nextafter(const float f, const float s)
{
  const float af = abs(f);
  const float nf = f+s*af*(1.f/(float)(1<<22));
#ifndef NDEBUG
  if (nf == f)
    print("NEXTAFTER NOT WORKING");
#endif
  return nf;
}

inline vec3f nextafter(const vec3f v, const vec3f sign)
{ return make_vec3f(nextafter(v.x,sign.x),
                    nextafter(v.y,sign.y),
                    nextafter(v.z,sign.z)); }



/* the octant method in which the boundary is defined by the coarser
   side */
extern float octMethodBoundaryFromCoarse(const uniform Chombo *uniform self,
                                         const vec3f &pos)
{
  // ------------------------------------------------------------------
  // determine cell and octant
  // ------------------------------------------------------------------
  CellRef cell = findCell(self,pos,0.f);
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
  findEightCellsNotOrdered(self,oct,cell.width);

  // ------------------------------------------------------------------
  // check if we're at a boundary to a COARSER region. for this
  // method, it's fine if the neighbors are finer (since in this case
  // _we_ are the coarser ones that define the boundary value; but if
  // the neighbor if coarser then it's the neighor that defines the
  // value.
  // ------------------------------------------------------------------
  for (uniform int i=1;i<8;i++) {
    if (oct.c[i].width != oct.c[0].width)
      const vec3f cornerPos = make_vec3f((i&1)?oct.P1.x:oct.P0.x,
                                         (i&2)?oct.P1.y:oct.P0.y,
                                         (i&4)?oct.P1.z:oct.P0.z);
    oct.c[i].value = octMethodBoundaryFromCoarse(self,cornerPos);
  }
  
  const float rcpWidth = 1.f/oct.c[C000].width;
  return lerp(oct,abs(pos-cc)*rcpWidth);
}
