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

#include "CellQuery.ih"

/*! perform tri-linear interpolation on the level of the leaf cell
  that the sample is in.  note that THIS IS NOT CORRECT across level
  boundaries, as a sample may be in a corse cell, but its neighbor
  might be subdivided, which this one ignores */
varying float 
MSAMR_interpolateOnCurrentCellLevel(void *uniform _self, 
                                    const varying vec3f &worldSpacePos)
{
  MSAMRVolume *uniform self = (MSAMRVolume *uniform)_self;
  const CellDesc desiredCD = toGridSpace(self,worldSpacePos);

  CellQuery vertex[2][2][2];

  vertex[0][0][0] = cellQuery(self,desiredCD);
  const CellDesc foundCD = vertex[0][0][0].cd;
  const vec3f foundCC = centerOf(foundCD);

  const int dx = desiredCD.pos.x >= foundCC.x ? +1 : -1;
  const int dy = desiredCD.pos.y >= foundCC.y ? +1 : -1;
  const int dz = desiredCD.pos.z >= foundCC.z ? +1 : -1;

  vertex[0][0][1] = cellQuery(self,neighborCD(foundCD,make_vec3i(dx,00,00)));
  vertex[0][1][0] = cellQuery(self,neighborCD(foundCD,make_vec3i(00,dy,00)));
  vertex[0][1][1] = cellQuery(self,neighborCD(foundCD,make_vec3i(dx,dy,00)));
  vertex[1][0][0] = cellQuery(self,neighborCD(foundCD,make_vec3i(00,00,dz)));
  vertex[1][0][1] = cellQuery(self,neighborCD(foundCD,make_vec3i(dx,00,dz)));
  vertex[1][1][0] = cellQuery(self,neighborCD(foundCD,make_vec3i(00,dy,dz)));
  vertex[1][1][1] = cellQuery(self,neighborCD(foundCD,make_vec3i(dx,dy,dz)));

  vec3f frac = abs(foundCC - desiredCD.pos) * rcp(foundCD.width);
  // out(frac);

  const float f000 = vertex[0][0][0].value;
  const float f001 = vertex[0][0][1].value;
  const float f010 = vertex[0][1][0].value;
  const float f011 = vertex[0][1][1].value;
  const float f100 = vertex[1][0][0].value;
  const float f101 = vertex[1][0][1].value;
  const float f110 = vertex[1][1][0].value;
  const float f111 = vertex[1][1][1].value;

  const float f00 = (1.f-frac.x)*f000 + frac.x*f001;
  const float f01 = (1.f-frac.x)*f010 + frac.x*f011;
  const float f10 = (1.f-frac.x)*f100 + frac.x*f101;
  const float f11 = (1.f-frac.x)*f110 + frac.x*f111;

  const float f0 = (1.f-frac.y)*f00 + frac.y*f01;
  const float f1 = (1.f-frac.y)*f10 + frac.y*f11;

  const float f = (1.f-frac.z)*f0 + frac.z*f1;
  return f;
}

