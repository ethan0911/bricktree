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

#pragma once

// amr base
#include "ChomboInput.h"

namespace ospray {
  namespace amr {

    /*! this structure defines only the format of the INPUT of chombo
        data - ie, what we get from the scene graph or applicatoin */
    ChomboInput::ChomboInput(const OSPData brickInfoData, 
                             const OSPData brickDataData)
    {
    }

  } // ::ospray::amr
} // ::ospray

