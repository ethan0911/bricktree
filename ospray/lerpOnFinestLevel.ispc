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

  // translate to grid space on finest grid level
  const float fx = (pos.x-hcw)/cw;
  const float fy = (pos.y-hcw)/cw;
  const float fz = (pos.z-hcw)/cw;


  const vec3f _P = make_vec3f((pos.x <= hcw) ? -1.f : (float)(int)(fx),
                              (pos.y <= hcw) ? -1.f : (float)(int)(fy),
                              (pos.z <= hcw) ? -1.f : (float)(int)(fz));
  OctCorners co;
  co.P0 = (_P+make_vec3f(0.5f,0.5f,0.5f))*cw;
  co.P1 = co.P0 + make_vec3f(cw);
  findEightCellsNotOrdered(chom,co,0.f);
  const vec3f w = (pos-hcw)/cw - _P;
  return lerp(co,w);
  // const float f000 = co.value[0][0][0];
  // const float f001 = co.value[0][0][1];
  // const float f010 = co.value[0][1][0];
  // const float f011 = co.value[0][1][1];
  // const float f100 = co.value[1][0][0];
  // const float f101 = co.value[1][0][1];
  // const float f110 = co.value[1][1][0];
  // const float f111 = co.value[1][1][1];

  // const float dx = fx - x0;
  // const float dy = fy - y0;
  // const float dz = fz - z0;

  // const float f00 = (1.f-dx)*f000 + dx*f001;
  // const float f01 = (1.f-dx)*f010 + dx*f011;
  // const float f10 = (1.f-dx)*f100 + dx*f101;
  // const float f11 = (1.f-dx)*f110 + dx*f111;

  // const float f0 = (1.f-dy)*f00+dy*f01;
  // const float f1 = (1.f-dy)*f10+dy*f11;

  // const float f = (1.f-dz)*f0+dz*f1;
  // return f;
}


