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
#include "hdf5.h"
#include "common/box.h"

namespace ospray {
  namespace HDF {
    using namespace ospcommon;

    using std::endl;
    using std::cout;

    struct ChomboLevel {
      ChomboLevel(int levelID) : levelID(levelID) {};

      int levelID;
      // apparently, the refinement factor of this level
      double dt; 
      std::vector<box3i> boxes;
      std::vector<double> data;

      void parseBoxes(hid_t file);
      void parseData(hid_t file);
      void parse(hid_t file);
    };
    
    std::vector<ChomboLevel *> parseChombo(const std::string fileName);

    extern "C" int main(int ac, char **av) 
    {
      const std::string fileName = av[1];
      std::vector<ChomboLevel *> level = parseChombo(fileName);
      cout << "found " << level.size() << " levels" << endl;
      for (int i=0;i<level.size();i++)
        PRINT(level[i]->dt);

      return 0;
    }

  }
}


