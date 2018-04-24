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
#include "BrickTree.ih"
// embree
#include "embree2/rtcore.isph"

//! Find the next hit point in the volume for ray casting based renderers.
static void BrickTreeVolume_stepRay(void *uniform _self, 
                                      varying Ray &ray,
                                      const varying float samplingRate)
{
    BrickTreeVolume *uniform self = (BrickTreeVolume *uniform) _self;
  
  // TODO: clean this up, properly implemnt an integrator with
  // variable step size...
  const float step// = self->super.samplingRate;
    = self->super.samplingStep / samplingRate; //self->super.samplingRate;
  ray.t0 += step;
}

extern "C" unmasked uniform float BrickTree_scalar_sample(void *uniform cppObject,
                                                                 uniform vec3f &samplePos);

static float BrickTreeVolume_sample(void *uniform _self, const vec3f &samplePos)
{
  BrickTreeVolume *uniform self = (BrickTreeVolume *uniform)_self;
  uniform vec3f uSamplePos[programCount];
  uniform float uReturnValue[programCount];
  uSamplePos[programIndex] = samplePos;
  foreach_active(lane)
    uReturnValue[lane] = BrickTree_scalar_sample(self->cppSampler,uSamplePos[lane]);
  return uReturnValue[programIndex];
}

extern "C" unmasked uniform vec3f BrickTree_scalar_computeGradient(void *uniform cppObject,
                                                                   uniform vec3f &samplePos);

static vec3f BrickTreeVolume_computeGradient(void *uniform _self, const vec3f &samplePos)
{
    BrickTreeVolume *uniform self = (BrickTreeVolume *uniform)_self;
    uniform vec3f uSamplePos[programCount];
    uniform vec3f uReturnValue[programCount];
    uSamplePos[programIndex] = samplePos;
    foreach_active(lane)
	uReturnValue[lane] = BrickTree_scalar_computeGradient(self->cppSampler,uSamplePos[lane]);
    return uReturnValue[programIndex];
}


export void *uniform BrickTreeVolume_create(void *uniform cppEquivalent) 
{
    BrickTreeVolume *uniform self = uniform new uniform BrickTreeVolume;
    Volume_Constructor(&self->super,cppEquivalent);
  
    return self;
};
 
export void BrickTreeVolume_set(void *uniform _self,
                                void *uniform _xf,
                                const uniform vec3i &validSize,
                                /*! pointer to the c++ side object */
                                void *uniform cppObject,
                                void *uniform cppSampler)
{
    BrickTreeVolume *uniform self = (BrickTreeVolume *uniform)_self;
    assert(self);
  
    self->super.boundingBox.lower = make_vec3f(0.f);
    self->super.boundingBox.upper = make_vec3f(validSize);

    self->super.volumeClippingBox.lower = make_vec3f(0.f);
    self->super.volumeClippingBox.upper = make_vec3f(validSize);
    // print("#osp:bricktree: bounds\n%\n%\n%\n%\n%\n%\n",
	//   self->super.boundingBox.lower.x,
	//   self->super.boundingBox.lower.y,
	//   self->super.boundingBox.lower.z,
	//   self->super.boundingBox.upper.x,
	//   self->super.boundingBox.upper.y,
	//   self->super.boundingBox.upper.z);
  
    self->super.computeGradient = &BrickTreeVolume_computeGradient;
    //self->super.adaptiveSampling = true;
  
    self->super.samplingRate = 0.125f;//samplingRate;
    self->super.samplingStep = 0.5f;
  
    self->super.gradientShadingEnabled = 0;
  
    self->super.transferFunction = (TransferFunction *uniform)_xf;

    self->super.stepRay          = &BrickTreeVolume_stepRay;
    self->super.sample           = &BrickTreeVolume_sample;    
    
    self->cppObject              = cppObject;
    self->cppSampler             = cppSampler;
}
