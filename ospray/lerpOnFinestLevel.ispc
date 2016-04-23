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

/*! interpolate on the finest level */
extern varying float Chombo_lerpOnFinestLevel(void *uniform _self, 
                                              const varying vec3f &pos)
{
  const ChomboVolume *uniform self = (ChomboVolume *)_self;
  const Chombo *uniform chom = &self->chombo;

  const float cw = chom->finestLevelCellWidth;
  const float hcw = 0.5f * cw;

  // grid coordinates
  const vec3f C0 = floor((pos+make_vec3f(hcw))/make_vec3f(cw))-make_vec3f(1.f);
  // const vec3f C0 = floor((pos-make_vec3f(hcw))/make_vec3f(cw));
  const vec3f cc = C0 * make_vec3f(cw) + make_vec3f(hcw);
  OctCorners oc;
  oc.P0 = cc;
  oc.P1 = oc.P0 + make_vec3f(cw);

  findEightCellsNotOrdered(chom,oc,0.f);

  const vec3f w = abs(pos-cc) / cw;
  return lerp(oc,w);
}


