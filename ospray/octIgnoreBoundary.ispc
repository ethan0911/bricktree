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

/*! the simplest variant of an octant method - determines the octant
    of the leaf cell the poitn is in, then finds the 8 neighboring
    dual octants of same or coarser level. If NOT at a level boundary
    that is the same as interpolating at the given level; across a
    cell boundary it is NOT smooth, but close. */
extern float Chombo_octIgnoreBoundaries(void *uniform _self, const vec3f &pos)
{
  const ChomboVolume *uniform self = (const ChomboVolume *uniform)_self;
  const Chombo *uniform chom = &self->chombo;

  CellRef cell = findCell(chom,pos,0.f);
  const vec3f cc = centerOf(cell);
  const vec3f sign = make_vec3f(pos.x < cc.x ? -1.f : +1.f,
                                pos.y < cc.y ? -1.f : +1.f,
                                pos.z < cc.z ? -1.f : +1.f);
  const vec3f sideID = make_vec3f(pos.x < cc.x ? 0.f : 1.f,
                                  pos.y < cc.y ? 0.f : 1.f,
                                  pos.z < cc.z ? 0.f : 1.f);
  const vec3f sidePos = cell.pos + sideID*cell.width;
  OctCorners oct;
  oct.P0 = pos;
  oct.P1 = pos + sign * make_vec3f(cell.width);

  findEightCellsNotOrdered(chom,oct,cell.width);

  const float rcpWidth = 1.f/oct.c[C000].width;
  return lerp(oct,abs(pos-cc)*rcpWidth);
}
