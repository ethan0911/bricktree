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
// ospray/sg
#include "sg/volume/Volume.h"
// hdf
#include "common/box.h"

namespace ospray {
  namespace chombo {

    struct Level {
      Level(int levelID) : levelID(levelID) {};
      
      int levelID;
      // apparently, the refinement factor of this level
      double dt; 

      inline ospcommon::vec3i boxSize(const size_t boxID) const
      { return boxes[boxID].size()+ospcommon::vec3i(1); }
      
      std::vector<ospcommon::box3i> boxes;
      std::vector<int>    offsets;
      std::vector<double> data;
    };

    struct Chombo {
      std::vector<Level *>     level;
      //! array of component names
      std::vector<std::string> component;

      static Chombo *parse(const std::string &fileName);
    };

  }
}
