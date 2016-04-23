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
#include "ChomboVolume.ih"

// ------------------------------------------------------------------
/*! "blend from coarser levels". check if we're in a uniform region,
    and if so do lerp. if not, blend values from lower level. NOTE
    THAT QUALITATIVELY THIS IS THE SAME AS ONE OF THE OCT METHODS */
// ------------------------------------------------------------------
float blendFromCoarser_onLevel(const Chombo *uniform chom, const vec3f &pos, float level);

float Chombo_blendFromCoarser(void *uniform _self, const vec3f &pos)
{
  const ChomboVolume *uniform self = (const ChomboVolume *uniform)_self;
  const Chombo *uniform chom = &self->chombo;
  return blendFromCoarser_onLevel(chom,pos,0.f);
}

float blendFromCoarser_onLevel(const Chombo *uniform chom, const vec3f &pos, float level)
{
  const CellRef cell = findCell(chom,pos,level); //findLeafCell(chom,pos);

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
  oct.P1 = nextafter(pos + sign * make_vec3f(cell.width),sign);
  findEightCellsNotOrdered(chom,oct,cell.width);

  float min_dx = 1.f, min_dy = 1.f, min_dz = 1.f;
  float scl_dx = 1.f, scl_dy = 1.f, scl_dz = 1.f;
  float cornerWeight[8] = { 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f };

  // sides
  if (oct.c[C001].width > oct.c[C000].width) {
    min_dx = min(min_dx,oct.c[C001].width);
    cornerWeight[C001] = 0.f;
  }
  if (oct.c[C010].width > oct.c[C000].width) {
    min_dy = min(min_dy,oct.c[C010].width);
    cornerWeight[C010] = 0.f;
  }
  if (oct.c[C100].width > oct.c[C000].width) {
    min_dz = min(min_dz,oct.c[C100].width);
    cornerWeight[C100] = 0.f;
  }

  // edges
  if (oct.c[C011].width > oct.c[C000].width) {
    min_dx = min(min_dx,oct.c[C011].width);
    min_dy = min(min_dy,oct.c[C011].width);
    cornerWeight[C011] = 0.f;
  }

  if (oct.c[C101].width > oct.c[C000].width) {
    min_dx = min(min_dx,oct.c[C101].width);
    min_dz = min(min_dz,oct.c[C101].width);
    cornerWeight[C101] = 0.f;
  }

  if (oct.c[C110].width > oct.c[C000].width) {
    min_dy = min(min_dy,oct.c[C110].width);
    min_dz = min(min_dz,oct.c[C110].width);
    cornerWeight[C110] = 0.f;
  }
  
  // corner
  if (oct.c[C111].width > oct.c[C000].width) {
    min_dx = min(min_dx,oct.c[C111].width);
    min_dy = min(min_dy,oct.c[C111].width);
    min_dz = min(min_dz,oct.c[C111].width);
    cornerWeight[C111] = 0.f;
  }

  bool border_dx = min_dx < 1.f;
  bool border_dy = min_dy < 1.f;
  bool border_dz = min_dz < 1.f;
  
  const float rcpWidth = 1.f/oct.c[C000].width;
  if (!(border_dx | border_dy | border_dz)) {
    return lerp(oct,abs(pos-cc)*rcpWidth);
  }

  if (border_dx) oct.c[C001].value = oct.c[C000].value;
  if (border_dy) oct.c[C010].value = oct.c[C000].value;
  if (border_dz) oct.c[C100].value = oct.c[C000].value;

  if (border_dx | border_dy) oct.c[C011].value = oct.c[C000].value;
  if (border_dx | border_dz) oct.c[C101].value = oct.c[C000].value;
  if (border_dy | border_dz) oct.c[C110].value = oct.c[C000].value;

  if (border_dx | border_dy | border_dz) oct.c[C111].value = oct.c[C000].value;

  vec3f rel = abs(pos-cc)*rcpWidth;
  float weight = lerp(cornerWeight,rel);
  
  return 
    weight * lerp(oct,rel) +
    (1.f-weight) * blendFromCoarser_onLevel(chom,pos,min(min(min_dx,min_dy),min_dz));
}
