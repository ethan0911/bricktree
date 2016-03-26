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

/*! perform tri-linear interpolation on the finest cell grid level -
  ie, coarse regions will be entirely flat inside the coarse region, 
  and have a linar fine-cell blend where they abut */
varying float MSAMR_interpolateOnFinestLevel(void *uniform _self, 
                                             const varying vec3f &worldSpacePos)
{
  print("not yet implemented");
  return 0.f;
}

