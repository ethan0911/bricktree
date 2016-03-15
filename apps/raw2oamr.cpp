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
      cout << "  ./ospRaw2Octree <inFile.raw> <args>" << endl;
      cout << "with args:" << endl;
      cout << " -dims <nx> <ny> <nz>   : input dimensions" << endl;
      cout << " --format <uint8|float> : input voxel format" << endl;
      cout << " --depth <maxlevels>    : use maxlevels octree refinement levels" << endl;
      cout << " -o <outfilename.xml>   : output file name" << endl;
      cout << " -t <threshold>         : threshold of which nodes to split or not (float val rel to min/max)" << endl;
      exit(msg != "");
    }

    AMR *oamrFromVolume(const Array3D<float> *input,
                        int maxLevels,
                        float threshold)
    {
      FromArray3DBuilder builder;

      cout << "read input, now building AMR Octree" << endl;
      AMR *oct = builder.makeAMR(input,maxLevels,threshold);
      cout << "done building" << endl;
      return oct;
    }

    extern "C" int main(int ac, char **av)
    {
      std::string inFileName  = "";
      std::string outFileName = "";
      std::string format      = "float";
      int         maxLevels   = 4;
      float       threshold   = .01f;
      vec3i       dims        = vec3i(0);
      vec3i       repeat      = vec3i(0);

      for (int i=1;i<ac;i++) {
        const std::string arg = av[i];
        if (arg == "-o")
          outFileName = av[++i];
        else if (arg == "--depth" || arg == "-depth" || 
                 arg == "--max-levels" || arg == "-ml")
          maxLevels = atoi(av[++i]);
        else if (arg == "--threshold" || arg == "-t")
          threshold = atof(av[++i]);
        else if (arg == "--repeat" || arg == "-r") {
          repeat.x = atof(av[++i]);
          repeat.y = atof(av[++i]);
          repeat.z = atof(av[++i]);
        } else if (arg == "--format")
          format = av[++i];
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
       
      const Array3D<float> *input = NULL;
      cout << "going to load RAW file:" << endl;
      cout << "  input file is   " << inFileName << endl;
      cout << "  expected format " << format << endl;
      cout << "  expected dims   " << dims << endl;
      if (format == "float") {
        input = loadRAW<float>(inFileName,dims);
      } else if (format == "uint8") {
        const Array3D<uint8> *input_uint8 = loadRAW<uint8>(inFileName,dims);
        input = new Array3DAccessor<uint8,float>(input_uint8);
      } else
        throw std::runtime_error("unsupported format '"+format+"'");
      cout << "loading complete." << endl;

      if (repeat.x != 0) {
        cout << "artificially blowing up to size " << repeat << endl;
        input = new Array3DRepeater<float>(input,repeat);
      }

      AMR *amr = oamrFromVolume(input,maxLevels,threshold);
      assert(amr);
      amr->writeTo(outFileName);
      
      return 0;
    }

  } // ::ospray::amr
} // ::ospary
