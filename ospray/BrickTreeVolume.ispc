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
#include "ospray/common/Model.ih"
#include "ospray/geometry/Geometry.ih"
#include "ospray/transferFunction/LinearTransferFunction.ih"
// this module
#include "BrickTreeVolume.ih"
// embree
#include "rtcore.isph"

// Functions that defined in CPP files
extern "C" unmasked uniform float BrickTree_scalar_sample(void *uniform cppObject, uniform vec3f &samplePos);
// TODO gradient shading is not working
extern "C" unmasked uniform vec3f BrickTree_scalar_computeGradient(void *uniform cppObject, uniform vec3f &samplePos);

inline void getVoxelAddress(void *uniform _self,
                     const varying vec3i &coord,
                     varying int &blockID)
{
  BrickTreeVolume *uniform self = (BrickTreeVolume *uniform) _self;
  int x = coord.x / self->blockWidth;
  int y = coord.y / self->blockWidth;
  int z = coord.z / self->blockWidth;
  x = (x >= self->gridSize.x) ? (self->gridSize.x -1) : x;
  y = (y >= self->gridSize.y) ? (self->gridSize.y -1) : y;
  z = (z >= self->gridSize.z) ? (self->gridSize.z -1) : z;
  blockID = x + y * self->gridSize.x + z * self->gridSize.y * self->gridSize.x;
}

inline void getVoxel(void *uniform _self,
                     const varying vec3i &coord,
                     varying float &value)
{
  int blockID;
  getVoxelAddress(_self, coord, blockID);
  value = 0.2f;
}

// Find the next hit point in the volume for ray casting based renderers.
static void BrickTreeVolume_stepRay(void *uniform _self, 
                                    varying Ray &ray,
                                    const varying float samplingRate)
{
  BrickTreeVolume *uniform self = (BrickTreeVolume *uniform) _self;
  // TODO: clean this up, properly implemnt an integrator with
  // variable step size...
  const float step = self->super.samplingStep / samplingRate; 
  ray.t0 += step;
}

static float BrickTreeVolume_sample(void *uniform _self, const vec3f &samplePos)
{
  // BrickTreeVolume *uniform self = (BrickTreeVolume *uniform)_self;
  // // Lower and upper corners of the box straddling the voxels to be interpolated.
  // const vec3i voxelIndex_0 = to_int(samplePos);
  // const vec3i voxelIndex_1 = voxelIndex_0 + 1;
  // // Fractional coordinates within the lower corner voxel used during interpolation.
  // const vec3f fractionalLocalCoordinates = samplePos - to_float(voxelIndex_0);
  // // Look up the voxel values to be interpolated.
  // float voxelValue_000;
  // float voxelValue_001;
  // float voxelValue_010;
  // float voxelValue_011;
  // float voxelValue_100;
  // float voxelValue_101;
  // float voxelValue_110;
  // float voxelValue_111;
  // getVoxel(self, make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_0.z), voxelValue_000);
  // getVoxel(self, make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_0.z), voxelValue_001);
  // getVoxel(self, make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_0.z), voxelValue_010);
  // getVoxel(self, make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_0.z), voxelValue_011);
  // getVoxel(self, make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_1.z), voxelValue_100);
  // getVoxel(self, make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_1.z), voxelValue_101);
  // getVoxel(self, make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_1.z), voxelValue_110);
  // getVoxel(self, make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_1.z), voxelValue_111);

  // // Interpolate the voxel values.
  // const float voxelValue_00 = voxelValue_000 + fractionalLocalCoordinates.x * (voxelValue_001 - voxelValue_000);
  // const float voxelValue_01 = voxelValue_010 + fractionalLocalCoordinates.x * (voxelValue_011 - voxelValue_010);
  // const float voxelValue_10 = voxelValue_100 + fractionalLocalCoordinates.x * (voxelValue_101 - voxelValue_100);
  // const float voxelValue_11 = voxelValue_110 + fractionalLocalCoordinates.x * (voxelValue_111 - voxelValue_110);
  // const float voxelValue_0  = voxelValue_00  + fractionalLocalCoordinates.y * (voxelValue_01  - voxelValue_00 );
  // const float voxelValue_1  = voxelValue_10  + fractionalLocalCoordinates.y * (voxelValue_11  - voxelValue_10 );
  // const float volumeSample  = voxelValue_0   + fractionalLocalCoordinates.z * (voxelValue_1   - voxelValue_0  );

  // return volumeSample;

  BrickTreeVolume *uniform self = (BrickTreeVolume *uniform)_self;
  uniform vec3f uSamplePos[programCount];
  uniform float uReturnValue[programCount];
  uSamplePos[programIndex] = samplePos;
  foreach_active(lane) {
    uReturnValue[lane] =
      BrickTree_scalar_sample(self->cppSampler, uSamplePos[lane]);
  }
  return uReturnValue[programIndex];
}

static vec3f BrickTreeVolume_computeGradient(void *uniform _self, const vec3f &samplePos)
{
  BrickTreeVolume *uniform self = (BrickTreeVolume *uniform)_self;
  uniform vec3f uSamplePos[programCount];
  uniform vec3f uReturnValue[programCount];
  uSamplePos[programIndex] = samplePos;
  foreach_active(lane) {
    uReturnValue[lane] = 
      BrickTree_scalar_computeGradient(self->cppSampler,uSamplePos[lane]);
  }
  return uReturnValue[programIndex];
}

export void *uniform BrickTreeVolume_create(void *uniform cppEquivalent) 
{
    BrickTreeVolume *uniform self = uniform new uniform BrickTreeVolume;
    Volume_Constructor(&self->super,cppEquivalent);
    return self;
};

export void BrickTreeVolume_set(void *uniform _self,
                                const uniform vec3i &validSize,
                                const uniform vec3i &gridSize,
                                const uniform int &brickSize,
                                const uniform int &blockWidth,
                                /*! pointer to the c++ side object */
                                void *uniform cppObject,
                                void *uniform cppSampler)
{
    BrickTreeVolume *uniform self = (BrickTreeVolume *uniform)_self;
    assert(self);
    self->super.boundingBox.lower = make_vec3f(0.f);
    self->super.boundingBox.upper = make_vec3f(validSize);
    self->super.computeGradient  = BrickTreeVolume_computeGradient;
    self->super.stepRay          = BrickTreeVolume_stepRay;
    self->super.sample           = BrickTreeVolume_sample;
    self->cppObject              = cppObject;
    self->cppSampler             = cppSampler;
    // print("#osp:bt: bounds (%\t%\t%\t)\n"
    //       "                (%\t%\t%\t)\n",
	  // self->super.boundingBox.lower.x,
	  // self->super.boundingBox.lower.y,
	  // self->super.boundingBox.lower.z,
	  // self->super.boundingBox.upper.x,
	  // self->super.boundingBox.upper.y,
	  // self->super.boundingBox.upper.z);

    self->validSize = validSize;
    self->gridSize = gridSize;
    self->brickSize = brickSize;
    self->blockWidth = blockWidth;
}
