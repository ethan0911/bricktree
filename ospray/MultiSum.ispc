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

// ospray
#include "ospray/math/vec.ih"
#include "ospray/math/AffineSpace.ih"
#include "ospray/common/Ray.ih"
#include "ospray/geometry/Geometry.ih"
#include "ospray/common/Model.ih"
#include "ospray/transferFunction/LinearTransferFunction.ih"
// this module
#include "MultiSum.ih"
// embree
#include "embree2/rtcore.isph"

struct TentAlgoStackEntry {
  int32 blockID;
  vec3f blockPos;
  float blockWidth;
};

/*! descriptor for a cell. rather than elaborate cell and level
    indices we instead use the global base position (the lower left
    front corner) and width, which are equivalent) */
struct CellDesc {
  /*! integer coordinates of cell - ie, the int coordinates of the
      root cell */
  vec3i rootCellCoords;
  /*! and the linear ID of that root cell */
  int   rootCellID;
  /*! base coordinates of that block, in that tree (in [0,1]^3) */
  vec3f blockPos;
  /*! fractional component in that block */
  vec3f frac;
  float blockWidth;
  int blockID;
};

inline CellDesc findLeafCell(const MultiSumAMRVolume *uniform self, 
                             const varying vec3f pos)
{
  CellDesc cell;
  cell.rootCellCoords = make_vec3i(pos);
  cell.rootCellID
    = (cell.rootCellCoords.x + 
       self->rootGridDims.x*(cell.rootCellCoords.y +
                             self->rootGridDims.y*(cell.rootCellCoords.z)));
  
  foreach_unique (rootCellID in cell.rootCellID) {
    uniform SumDataBlock *uniform dataBlockArray
      = self->dataBlock+self->firstDataBlockOfTree[rootCellID];
    uniform SumIndexBlock *uniform indexBlockArray
      = self->indexBlock+self->firstIndexBlockOfTree[rootCellID];
    uniform int32 *uniform offsetOfArray 
      = (uniform int32*)self->blockInfo + self->firstDataBlockOfTree[rootCellID];
    
    int blockID = 0;
    cell.blockPos = make_vec3f(0.f);
    cell.blockWidth = 1.f;
    cell.frac = pos - make_vec3f(cell.rootCellCoords);

    while (1) {
      const SumDataBlock  *dataBlock  = dataBlockArray + blockID;
      const SumIndexBlock *indexBlock = NULL;
      const int indexBlockID = offsetOfArray[blockID];
      const float cellWidth = cell.blockWidth * 0.25f;
      if (indexBlockID >= 0)
        indexBlock = indexBlockArray+indexBlockID;
      
      const vec3i cID = make_vec3i(4.f * cell.frac);
      int childID;
      if (indexBlock && ((childID = indexBlock->child[cID.z][cID.y][cID.x]) >= 0)) {
        // cell we hit is a leaf cell...
        const vec3f childPos = make_vec3f(cID)*cellWidth;
        cell.blockPos = cell.blockPos + childPos;
        cell.frac   = 4.f * (cell.frac - childPos);
        cell.blockWidth = cellWidth;
        blockID = childID;
        continue;
      } else {
        break;
      }
    }
  }
  return cell;
}

inline float tentApproximation_packet(MultiSumAMRVolume *uniform self,
                                      const varying vec3f &worldCoordinates)
{
  // print("wc\n %\n %\n-> %\n",worldCoordinates.x,worldCoordinates.y,worldCoordinates.z);

  // "packet" variant
  uniform TentAlgoStackEntry stackBase[100];
  uniform TentAlgoStackEntry *uniform stackPtr = stackBase;
  
  const vec3f gridPos = worldCoordinates;
  
  // init numerator and denominator
  float num = 0.f;
  float den = 0.f;
  
  // -------------------------------------------------------
  // do initial stack fill
  // ------------------------------------------------------- 
  // - compute region of cells in root grid
  vec3i root_beg = make_vec3i(floor(gridPos - make_vec3f(.5f)));
  vec3i root_end = make_vec3i(root_beg+make_vec3i(2));
  root_beg = max(root_beg,make_vec3i(0));
  root_end = min(root_end,self->rootGridDims);
  
  // now, throw the 2x2x2 possibly overlapping root cells on the stack.
  for (int _iz=root_beg.z; _iz < root_end.z; _iz++) {
    foreach_unique (tree_iz in _iz) {
      for (int _iy=root_beg.y; _iy < root_end.y; _iy++) {
        foreach_unique (tree_iy in _iy) {
          for (int _ix=root_beg.x; _ix < root_end.x; _ix++) {
            foreach_unique (tree_ix in _ix) {
              const uniform int rootCellID
                = tree_ix + self->rootGridDims.x*(tree_iy+self->rootGridDims.y*(tree_iz));
              // stackPtr->cell = self->rootCell+rootCellID;

              uniform SumDataBlock *uniform dataBlockArray
                = self->dataBlock+self->firstDataBlockOfTree[rootCellID];
              uniform SumIndexBlock *uniform indexBlockArray
                = self->indexBlock+self->firstIndexBlockOfTree[rootCellID];
              uniform int32 *uniform offsetOfArray 
                = (uniform int32*)self->blockInfo + self->firstDataBlockOfTree[rootCellID];
              stackPtr->blockPos = make_vec3f(tree_ix,tree_iy,tree_iz);
              stackPtr->blockWidth = 1.f;
              stackPtr->blockID = 0; //rootCellID;
              stackPtr++;
              
              // -------------------------------------------------------
              // process stack nodes...
              // -------------------------------------------------------
              // note we do NOT know if the stack entries actually
              // overlap - that makes the pushing easier.
              while (stackPtr > stackBase) {
                -- stackPtr;
                uniform TentAlgoStackEntry cur = *stackPtr;
                
                const vec3f base = make_vec3f(cur.blockPos);
                const float blockWidth  = cur.blockWidth;
                // const vec3f cellCenter = base+0.5f*blockWidth;
                // const vec3f delta = absf(cellCenter - gridPos);
                uniform int blockID = cur.blockID;
                SumDataBlock *uniform dataBlock = dataBlockArray + blockID;

                SumIndexBlock *uniform indexBlock = NULL;
                const uniform int indexBlockID = offsetOfArray[blockID];
                if (indexBlockID >= 0)
                  indexBlock = indexBlockArray+indexBlockID;
                //   // leaf node ...
                //   const vec3f weight = max(make_vec3f(0.f),
                //                            make_vec3f(1.f) - delta * rcp(blockWidth));
                //   const float w = reduce_mul(weight);
                //   den += w;
                //   num += w * cur.cell->ccValue;
                // } else {
                  
                  // if (reduce_max(delta) >= blockWidth) 
                  //   continue;

                // print("blockID % indexblock %\n",blockID,indexBlock);

                const uniform float childCellWidth = 0.25f * cur.blockWidth;
                uniform vec3f childCellPos;
                vec3f delta;
                for (uniform int iiz=0;iiz<4;iiz++) {
                  childCellPos.z = cur.blockPos.z + iiz * childCellWidth;
                  delta.z = abs(childCellPos.z + 0.5f * childCellWidth - gridPos.z);
                  if (delta.z < childCellWidth)
                    for (uniform int iiy=0;iiy<4;iiy++) {
                      childCellPos.y = cur.blockPos.y + iiy * childCellWidth;
                      delta.y = abs(childCellPos.y + 0.5f * childCellWidth - gridPos.y);
                      if (delta.y < childCellWidth)
                        for (uniform int iix=0;iix<4;iix++) {
                          childCellPos.x = cur.blockPos.x + iix * childCellWidth;
                          delta.x = abs(childCellPos.x + 0.5f * childCellWidth - gridPos.x);
                          if (delta.x < childCellWidth) {
                            // print("child % % %\n",iix,iiy,iiz);
                            // -------------------------------------------------------
                            // OK, this is a cell that at least one ray overlaps!
                            // -------------------------------------------------------
                            if (indexBlock && (indexBlock->child[iiz][iiy][iix] >= 0)) {
                              stackPtr->blockPos = childCellPos;
                              stackPtr->blockWidth = childCellWidth;
                              stackPtr->blockID = indexBlock->child[iiz][iiy][iix];
                              ++stackPtr;
                              // print("pushing % stackptr %\n",indexBlock->child[iiz][iiy][iix],stackPtr-stackBase);
                            } else {
                              const vec3f weight = max(make_vec3f(0.f),
                                                       make_vec3f(1.f) - delta * rcp(childCellWidth));
                              const float w = reduce_mul(weight);
                              den += w;
                              num += w * dataBlock->value[iiz][iiy][iix];
                              // print("adding % %\n",w,dataBlock->value[iz][iy][ix]);
                            }
                          }
                        }
                    }
                }
              }
            }
          }
        }
      }
    }
  }

  const float result = num / (den + 1e-20f);
  // const float result = num / (255.f*den + 1e-8f);
  // print("returning %\n",result);
  return result;
}
                               

//! The value at the given sample location in world coordinates.
static varying float MultiSumAMRVolume_sample(void *uniform _self, 
                                              const varying vec3f &worldCoordinates)
{
  MultiSumAMRVolume *uniform self = (MultiSumAMRVolume *uniform)_self;
  assert(self);
  
  // print("%\n",self->validFractionOfRootGrid.z);
  vec3f scaledWC = worldCoordinates * (self->validFractionOfRootGrid);
  float f =  tentApproximation_packet(self,scaledWC);
  // if (reduce_max(worldCoordinates) > .9f)
  return f;
}

//! The gradient at the given sample location in world coordinates.
static varying vec3f MultiSumAMRVolume_gradient(void *uniform _self, 
                                        const varying vec3f &worldCoordinates)
{
  return make_vec3f(1,0,0);
}

//! Find the next hit point in the volume for ray casting based renderers.
static void MultiSumAMRVolume_intersect(void *uniform _self, 
                                varying Ray &ray)
{
  MultiSumAMRVolume *uniform self = (MultiSumAMRVolume *uniform) _self;
  
  // TODO: clean this up, properly implemnt an integrator with
  // variable step size...
  const uniform float step
    = self->super.samplingStep / self->super.samplingRate;
  ray.t0 += step;
}

export void *uniform MultiSumAMRVolume_create(void *uniform cppEquivalent) 
{
  MultiSumAMRVolume *uniform self = uniform new uniform MultiSumAMRVolume;
  Volume_Constructor(&self->super,cppEquivalent);

  return self;
};

export void MultiSumAMRVolume_set(void *uniform _self,
                                  void *uniform _xf,
                                  const uniform vec3i &rootGridDims,
                                  const uniform vec3f &validFractionOfRootGrid,
                                  void *uniform dataBlock,
                                  void *uniform indexBlock,
                                  void *uniform blockInfo,
                                  void *uniform firstDataBlockOfTreeData,
                                  void *uniform firstIndexBlockOfTreeData)
{
  MultiSumAMRVolume *uniform self = (MultiSumAMRVolume *uniform)_self;
  assert(self);
  
  self->super.boundingBox.lower = make_vec3f(0.f);
  self->super.boundingBox.upper = make_vec3f(rootGridDims)*validFractionOfRootGrid; 
  
  self->super.volumeClippingBox.lower = make_vec3f(0.f);
  self->super.volumeClippingBox.upper = make_vec3f(rootGridDims)*validFractionOfRootGrid; 
  
  self->super.intersect = &MultiSumAMRVolume_intersect;
  self->super.computeSample = &MultiSumAMRVolume_sample;
  self->super.computeGradient = &MultiSumAMRVolume_gradient;

  self->super.samplingRate = 1.f;//samplingRate;
  self->super.samplingStep = .1f;

  self->super.gradientShadingEnabled = 0;

  self->super.transferFunction = (TransferFunction *uniform)_xf;

  self->dataBlock  = (SumDataBlock  *)dataBlock;
  self->indexBlock = (SumIndexBlock *)indexBlock;
  self->blockInfo  = (SumBlockInfo  *)blockInfo;
  self->firstIndexBlockOfTree = (int32*)firstIndexBlockOfTreeData;
  self->firstDataBlockOfTree  = (int32*)firstDataBlockOfTreeData;
  // root grid dimensions
  self->rootGridDims = rootGridDims;
  self->validFractionOfRootGrid = validFractionOfRootGrid;

  // print("SET\n");
  // for (uniform int i=0;i<rootGridDims.x*rootGridDims.y*rootGridDims.z;i++)
  //   print("blockinfo[%][0] = %\n",i,self->blockInfo[self->firstDataBlockOfTree[i]].indexBlockID);
}



