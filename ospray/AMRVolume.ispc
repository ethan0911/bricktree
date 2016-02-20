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
#include "AMRVolume.ih"
// embree
#include "embree2/rtcore.isph"

export void *uniform AMRVolume_create(void *uniform cppEquivalent) 
{
  AMRVolume *uniform self = uniform new uniform AMRVolume;
  Volume_Constructor(&self->super,cppEquivalent);

  return self;
};

  //! The value at the given sample location in world coordinates.
static varying float AMRVolume_sample(void *uniform _self, 
                                      const varying vec3f &worldCoordinates)
{
  AMRVolume *uniform self = (AMRVolume *uniform)_self;
  assert(self);

  // print("sample\n%\n%\n%\n",worldCoordinates.x,worldCoordinates.y,worldCoordinates.z);
  return .5f; //worldCoordinates.x;
}

  //! The gradient at the given sample location in world coordinates.
static varying vec3f AMRVolume_gradient(void *uniform _self, 
                                        const varying vec3f &worldCoordinates)
{
  return make_vec3f(1,0,0);
}

  //! Find the next hit point in the volume for ray casting based renderers.
static void AMRVolume_intersect(void *uniform _self, 
                                varying Ray &ray)
{
  AMRVolume *uniform self = (AMRVolume *uniform) _self;

  // TODO: clean this up, properly implemnt an integrator with
  // variable step size...
  const uniform float step
    = self->super.samplingStep / self->super.samplingRate;

  ray.t0 += step;
}



export void AMRVolume_set(void *uniform _self,
                          void *uniform _xf,
                          const uniform vec3i &dimensions,
                          void *uniform a,
                          void *uniform b)
{
  AMRVolume *uniform self = (AMRVolume *uniform)_self;
  assert(self);

  self->super.boundingBox.lower = make_vec3f(0.f);
  self->super.boundingBox.upper = make_vec3f(dimensions);

  self->super.volumeClippingBox.lower = make_vec3f(0.f);
  self->super.volumeClippingBox.upper = make_vec3f(dimensions);

  self->super.intersect = &AMRVolume_intersect;
  self->super.computeSample = &AMRVolume_sample;
  self->super.computeGradient = &AMRVolume_gradient;

  self->super.samplingRate = 1.f;
  self->super.samplingStep = .1f;
  self->super.gradientShadingEnabled = 0;

  self->super.transferFunction = (TransferFunction *uniform)_xf;
}



