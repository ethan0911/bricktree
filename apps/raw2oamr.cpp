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

#undef NDEBUG

// ospray
#include "amr/AMR.h"
#include "amr/FromArray3DBuilder.h"

namespace ospray {
  namespace amr {

    using std::endl;
    using std::cout;

    void error(const std::string &msg = "")
    {
      if (msg != "") {
        cout << "*********************************" << endl;
        cout << "Error: " << msg << endl;
        cout << "*********************************" << endl << endl;
      }
      cout << "Usage" << endl;
      cout << "  ./ospOAmrResample <inFile.ospbin> <args>" << endl;
      cout << "with args:" << endl;
      cout << " -dims <nx> <ny> <nz>   : *out*put dimensions" << endl;
      cout << " -o <outfilename.raw>   : output file name" << endl;
      exit(msg != "");
    }
    
    extern "C" int main(int ac, char **av)
    {
      std::string inFileName  = "";
      std::string outFileName = "";
      vec3i       dims        = vec3f(0);

      for (int i=1;i<ac;i++) {
        const std::string arg = av[i];
        if (arg == "-o")
          outFileName = av[++i];
        else if (arg == "-dims" || arg == "--dimensions") {
          dims.x = atoi(av[++i]);
          dims.y = atoi(av[++i]);
          dims.z = atoi(av[++i]);
        }
        else if (arg[0] != '-')
          inFileName = av[i];
        else
          error("unknown arg '"+arg+"'");
      }
      
      if (dims == vec3i(0))
        error("no input dimensions (-dims) specified");
      if (inFileName == "")
        error("no input file specified");
      if (outFileName == "")
        error("no output file specified");
      
      AMR<float> *amr = AMR<float>::loadFrom(inFileName.c_str());
      assert(amr);


      ActualArray3D<float> *resampled = new ActualArray3D<float>(dims);
      for (int iz=0;iz<dims.z;iz++)
        for (int iy=0;iy<dims.y;iy++)
          for (int ix=0;ix<dims.x;ix++) {
            vec3f pos((vec3f(ix,iy,iz)+vec3f(.5f))/vec3f(dims));
          }
      return 0;
    }
    
#if 0
    FILE *out = fopen(outFileName.c_str(),"wb");
    assert(out);
    fwrite(resampled->value,resampled->numElements(),sizeof(float),out);
    fclose(out);
#endif
    
  } // ::ospray::amr
} // ::ospary
