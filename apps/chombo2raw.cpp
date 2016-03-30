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

#include "sg/ChomboReader.h"
#include "hdf5.h"
#include "amr/Array3D.h"
#include "amr/Range.h"

namespace ospray {
  namespace chombo {

    using namespace ospcommon;

    using std::endl;
    using std::cout;

    std::vector<chombo::Level *> chombo;
    vec3i rootGridDims;
    Range<float> range = empty;

    int compID = 0;
    std::string outFileName = "chombo_resampled";
    std::string inFileName = "";

    vec3i dims(0);

    void error(const std::string &msg)
    {
      cout << "error: " << msg << endl << endl;
      cout << "usage:" << endl;
      cout << "\tospChomboToRaw inFile.hdf5 -dims x y z -o outFileName --compID i" << endl;
      exit(1);
    }

    float resample(const vec3f unitPos)
    {
      float f = 0.f;
      for (int levelID=0;levelID<chombo.size();levelID++) {
        chombo::Level *l = chombo[levelID];
        vec3i localPos = vec3i(unitPos * vec3f(rootGridDims) / (float)l->dt);
        for (int i=0;i<l->boxes.size();i++) {
          if (l->boxes[i].contains(localPos)) {
            // f = levelID / (chombo.size()-1.f);//1.f;
            
            vec3i cell = localPos - l->boxes[i].lower;
            vec3i size = l->boxSize(i);

            size_t ofs
              = l->offsets[i]
              + cell.x
              + cell.y * size.x
              + cell.z * size.x*size.y
              + compID * size.x*size.y*size.z;

            f = (l->data[ofs] - range.lo) / (range.hi - range.lo);
          }
        }
      }
      // cout << "pos " << unitPos << " val " << f << endl;
      return f;
    }

    extern "C" int main(int ac, char **av)
    {
      for (int i=1;i<ac;i++) {
        const std::string arg = av[i];
        PRINT(arg);
        if (arg == "-o")
          outFileName = av[++i];
        else if (arg == "--compID")
          compID = atoi(av[++i]);
        else if (arg == "-dims" || arg == "--dims") {
          dims.x = atoi(av[++i]);
          dims.y = atoi(av[++i]);
          dims.z = atoi(av[++i]);
        } else if (arg == "-dims3") {
          dims.z = dims.y = dims.x = atoi(av[++i]);
        } else if (arg[0] == '-')
          error("unkonwn argument " + arg);
        else
          inFileName = arg;
      }
      const std::string fileName = inFileName;
      if (reduce_min(dims) <= 0)
        error("no dims specified (-dims x y z)");
      if (fileName == "")
        error("no input file specified");
      
      chombo = chombo::parseChombo(fileName);
      PRINT(fileName);
      rootGridDims = vec3i(0);
      for (int i=0;i<chombo[0]->boxes.size();i++)
        rootGridDims = max(rootGridDims,chombo[0]->boxes[i].upper);
      rootGridDims = rootGridDims;
      rootGridDims += vec3i(1);
      PRINT(rootGridDims);

      for (int l=0;l<chombo.size();l++)
        for (int i=0;i<chombo[l]->data.size();i++)
          range.extend(chombo[l]->data[i]);
      PRINT(range);

      amr::ActualArray3D<float> raw(dims);
      for (int iz=0;iz<dims.z;iz++)
        for (int iy=0;iy<dims.y;iy++)
          for (int ix=0;ix<dims.x;ix++) {
            raw.set(vec3i(ix,iy,iz),resample((vec3f(ix,iy,iz)+vec3f(.5f))/vec3f(dims)));
          }

      std::string rawFileName = outFileName+".raw";
      std::string ospFileName = outFileName+".osp";

      {
        FILE *file = fopen(rawFileName.c_str(),"wb");
        fwrite(raw.value,raw.numElements(),sizeof(float),file);
        fclose(file);
      }

      {
        FILE *file = fopen(ospFileName.c_str(),"w");
        fprintf(file,"<?xml version=\"1.0\"?>\n");
        fprintf(file,"<volume name=\"volume\">\n");
        fprintf(file,"  <dimensions> %i %i %i </dimensions>\n",dims.x,dims.y,dims.z);
        fprintf(file,"  <voxelType> float </voxelType>\n");
        fprintf(file,"  <samplingRate> 1.0 </samplingRate>\n");
        fprintf(file,"  <filename> %s </filename>\n",rawFileName.c_str());
        fprintf(file,"</volume>\n");
        fclose(file);
      }
      return 0;
    }
  }
}
