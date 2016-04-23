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

typedef float (*uniform BackFill)(const Chombo *uniform chom, 
                                  const Octant &oct);
typedef float (*uniform SampleFunc)(const Chombo *uniform chom, 
                                    uniform BackFill backFill,
                                    const vec3f &sample);

float backFillCellValue(const Chombo *uniform chom, 
                        const Octant &oct)
{
  return oct.c[0].value;
}

float backFillCoarsestLevel(const Chombo *uniform chom, 
                            const Octant &oct)
{ 
  const float maxWidth = max(max(max(oct.c[0].width,oct.c[1].width),
                                 max(oct.c[2].width,oct.c[3].width)),
                             max(max(oct.c[4].width,oct.c[5].width),
                                 max(oct.c[6].width,oct.c[7].width)));
#if 1
  const vec3f pos = oct.P0;
  const vec3f floor_shifted_P0 = maxWidth*floor((pos - make_vec3f(0.5f*maxWidth))/maxWidth);
  Octant oct;
  oct.P0 = floor_shifted_P0;
  oct.P1 = floor_shifted_P0 + make_vec3f(maxWidth);
  findEightCellsNotOrdered(chom,oct,maxWidth);
  const float rcpWidth = 1.f/maxWidth;
  const vec3f cc = floor_shifted_P0 + make_vec3f(0.5f*maxWidth);
  return lerp(oct,abs(pos-cc)*rcpWidth);
#else
  // ------------------------------------------------------------------
  // determine cell and octant
  // ------------------------------------------------------------------
  const vec3f pos = oct.P0;
  CellRef cell = findCell(chom,pos,maxWidth);
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
  const float rcpWidth = 1.f/oct.c[C000].width;
  return lerp(oct,abs(pos-cc)*rcpWidth);
#endif
}

float sideNeighbor(const Chombo *uniform chom, 
                  uniform SampleFunc sampleFunc,
                  uniform BackFill backFill,
                  const Octant &oct,
                  uniform int side)
{
   // if neighbor on COARSER level, ask for value at this position
  if (oct.c[side].width > oct.c[0].width) {
    // at least one of the three neighbors is coarser --> ask neighbor
    const vec3f nextP = make_vec3f(side&1?oct.P1.x:oct.P0.x,
                                   side&2?oct.P1.y:oct.P0.y,
                                   side&4?oct.P1.z:oct.P0.z);
    return sampleFunc(chom,backFill,nextP);
  }
  
  // test for 'backfill' case: is neighbor finer?
  if (oct.c[side].width < oct.c[0].width) {
    // no adjacent cell is coarser, but at least one is finer --> backfill
    return backFill(chom,oct);
  }
  
  // nothing special to do (ie, we're in a uniform region, all
  // neighbors are on same level): just interpolate
  return 0.5f*(oct.c[side].value+oct.c[0].value);
}

float edgeNeighbor(const Chombo *uniform chom, 
                   SampleFunc sampleFunc,
                   BackFill backFill,
                   Octant &oct,
                   uniform int sideA, 
                   uniform int sideB)
{
  const uniform int edge = sideA|sideB;
  const float maxWidth = max(max(oct.c[0].width,oct.c[edge].width),
                             max(oct.c[sideA].width,oct.c[sideB].width));
  const float minWidth = min(min(oct.c[0].width,oct.c[edge].width),
                             min(oct.c[sideA].width,oct.c[sideB].width));

  if (maxWidth > oct.c[0].width) {
    // at least one of the three neighbors is coarser --> ask neighbor
    const vec3f nextP = make_vec3f(edge&1?oct.P1.x:oct.P0.x,
                                   edge&2?oct.P1.y:oct.P0.y,
                                   edge&4?oct.P1.z:oct.P0.z);
    return sampleFunc(chom,backFill,nextP);
  }
  
  if (minWidth < oct.c[0].width) {
    // no adjacent cell is coarser, but at least one is finer --> backfill
    return backFill(chom,oct);
  }
  
  // nothing special to do (ie, we're in a uniform region, all
  // neighbors are on same level): just interpolate
  return 0.25f*(oct.c[0].value+oct.c[edge].value+
                oct.c[sideA].value+oct.c[sideB].value);
}

float cornerNeighbor(const Chombo *uniform chom, 
                     SampleFunc sampleFunc,
                     BackFill backFill,
                     Octant &oct)
{
  const float maxWidth = max(max(max(oct.c[0].width,oct.c[1].width),
                                 max(oct.c[2].width,oct.c[3].width)),
                             max(max(oct.c[4].width,oct.c[5].width),
                                 max(oct.c[6].width,oct.c[7].width)));
  const float minWidth = min(min(min(oct.c[0].width,oct.c[1].width),
                                 min(oct.c[2].width,oct.c[3].width)),
                             min(min(oct.c[4].width,oct.c[5].width),
                                 min(oct.c[6].width,oct.c[7].width)));
  // if (minWidth != maxWidth)
  //   print("mine %\n min %\n max %\n",oct.c[0].width,minWidth,maxWidth);
  if (maxWidth > oct.c[0].width) {
    // at least one of the three neighbors is coarser --> ask neighbor
    const vec3f nextP = oct.P1;
    return sampleFunc(chom,backFill,nextP);
  }
  
  if (minWidth < oct.c[0].width) {
    // no adjacent cell is coarser, but at least one is finer --> backfill
    return backFill(chom,oct);
  }

  // nothing special to do (ie, we're in a uniform region, all
  // neighbors are on same level): just interpolate
  return 0.125f*(oct.c[0].value+
                 oct.c[1].value+
                 oct.c[2].value+
                 oct.c[3].value+
                 oct.c[4].value+
                 oct.c[5].value+
                 oct.c[6].value+
                 oct.c[7].value);
}


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
float fromCoarseCorners(const Chombo *uniform chom, 
                        BackFill backFill, 
                        const vec3f &pos)
{
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
  oct.P0 = cc;
  oct.P1 = nextafter(cc + sign * make_vec3f(cell.width),sign);
  // oct.P0 = pos;
  // oct.P1 = nextafter(pos + sign * make_vec3f(cell.width),sign);
  
  // ------------------------------------------------------------------
  // determine all eight neighbors on cell's level
  // ------------------------------------------------------------------
  findEightCellsNotOrdered(chom,oct,cell.width);

  // ------------------------------------------------------------------
  // fill in octant corners
  // ------------------------------------------------------------------
  float corner[8];

  // cell itself never gets changed
  corner[0] = oct.c[0].value;

  // BackFill backFill = &backFillCellValue;
  // side neighbors
  corner[1] = sideNeighbor(chom,&fromCoarseCorners,backFill,oct,1);
  corner[2] = sideNeighbor(chom,&fromCoarseCorners,backFill,oct,2);
  corner[4] = sideNeighbor(chom,&fromCoarseCorners,backFill,oct,4);

  // edge neighbor
  corner[3] = edgeNeighbor(chom,&fromCoarseCorners,backFill,oct,1,2);
  corner[5] = edgeNeighbor(chom,&fromCoarseCorners,backFill,oct,1,4);
  corner[6] = edgeNeighbor(chom,&fromCoarseCorners,backFill,oct,2,4);

  // corner neighbor
  corner[7] = cornerNeighbor(chom,&fromCoarseCorners,backFill,oct);
  
  for (uniform int i=0;i<8;i++)
    oct.c[i].value = corner[i];
  const float rcpHalfWidth = 2.f/oct.c[C000].width;
  return lerp(oct,abs(pos-cc)*rcpHalfWidth);
}

extern float Chombo_octFromCoarseCorners(void *uniform _self, const vec3f &pos)
{
  const ChomboVolume *uniform self = (const ChomboVolume *uniform)_self;
  const Chombo *uniform chom = &self->chombo;
  BackFill backFill = &backFillCoarsestLevel;
  return fromCoarseCorners(chom,backFill,pos);
}

extern float Chombo_octFromCoarseCellValue(void *uniform _self, const vec3f &pos)
{
  const ChomboVolume *uniform self = (const ChomboVolume *uniform)_self;
  const Chombo *uniform chom = &self->chombo;
  BackFill backFill = &backFillCellValue;
  return fromCoarseCorners(chom,backFill,pos);
}
