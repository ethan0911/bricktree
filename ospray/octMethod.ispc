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

extern float octMethodSimple(const uniform Chombo *uniform self,
                             const vec3f &pos)
{
  CellRef cell = findCell(self,pos,0.f);
  const vec3f cc = centerOf(cell);
  const vec3f sign = make_vec3f(pos.x < cc.x ? -1.f : +1.f,
                                pos.y < cc.y ? -1.f : +1.f,
                                pos.z < cc.z ? -1.f : +1.f);
  const vec3f sideID = make_vec3f(pos.x < cc.x ? 0.f : 1.f,
                                  pos.y < cc.y ? 0.f : 1.f,
                                  pos.z < cc.z ? 0.f : 1.f);
  const vec3f sidePos = cell.pos + sideID*cell.width;
  CompactOctant co;
  co.P0 = pos;
  co.P1 = pos + sign * make_vec3f(cell.width);

  findEightCellsNotOrdered(self,co,cell.width);

  const float rcpWidth = 1.f/co.width[0][0][0];
  const float dx = abs(pos.x-cc.x)*rcpWidth;
  const float dy = abs(pos.y-cc.y)*rcpWidth;
  const float dz = abs(pos.z-cc.z)*rcpWidth;
  
  const float f000 = co.value[0][0][0];
  const float f001 = co.value[0][0][1];
  const float f010 = co.value[0][1][0];
  const float f011 = co.value[0][1][1];
  const float f100 = co.value[1][0][0];
  const float f101 = co.value[1][0][1];
  const float f110 = co.value[1][1][0];
  const float f111 = co.value[1][1][1];

  return f100;
  // return f111;
  
  const float f00 = (1.f-dx)*f000 + dx*f001;
  const float f01 = (1.f-dx)*f010 + dx*f011;
  const float f10 = (1.f-dx)*f100 + dx*f101;
  const float f11 = (1.f-dx)*f110 + dx*f111;

  const float f0 = (1.f-dy)*f00+dy*f01;
  const float f1 = (1.f-dy)*f10+dy*f11;

  const float f = (1.f-dz)*f0+dz*f1;
  return f;


  // float maxWidth = co.width[0][0][0];
  // float minWidth = co.width[0][0][0];
  // for (uniform int iz=0;iz<2;iz++)
  //   for (uniform int iy=0;iy<2;iy++)
  //     for (uniform int ix=0;ix<2;ix++) {
  //       maxWidth = max(maxWidth,co.width[iz][iy][ix]);
  //       minWidth = min(minWidth,co.width[iz][iy][ix]);
  //     }

  // // interpolation weights
  // vec3f w = (pos - cc) / 

  // if (maxWidth == 0.f) {
  //   out(co);
  //   print("FAULT\n");
  // }
  // static float lastMaxWidth = 0.f;
  // if (maxWidth != lastMaxWidth) {
  //   print("maxw %\n",maxWidth);
  //   lastMaxWidth = maxWidth;
  // }
  // return maxWidth;
  // if (maxWidth > 0.125f) 
  //   print("maxwidth %\n",maxWidth);
  // if (maxWidth != minWidth) {
  //   return 255.f;
  // } else {
  //   return min(128,co.value[0][0][0]);
  // }
  // return co.value[0][0][0];
}
