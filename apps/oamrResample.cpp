
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
    
    void createSliceImage(AMR<float> *amr,
                          const vec3f &org,
                          const vec3f &dx, 
                          const vec3f &dy,
                          const char *name)
    {
      FILE *file = fopen(name,"wb");
      assert(file);

      int sizeX=512, sizeY=sizeX;

      Range<float> range = amr->getValueRange();

      fprintf(file,"P6\n%i %i\n255\n",sizeX,sizeY);
      for (int y=0;y<sizeY;y++)
        for (int x=0;x<sizeX;x++) {
          vec3f pos
            = org
            + (x+.5f)/sizeX * dx
            + (y+.5f)/sizeY * dy;
          float f = amr->sample(pos);
          f = (f-range.lo)/(range.hi-range.lo);
          int t = int(255*f);
          fwrite(&t,1,1,file);
          fwrite(&t,1,1,file);
          fwrite(&t,1,1,file);
        }
      fprintf(file,"\n");
      cout << "successfully written slice file " << name << endl;
      fclose(file);
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

      if (1) {
        createSliceImage(amr,vec3f(0,0,0.5f),vec3f(1,0,0),vec3f(0,1,0),"centerSlice_x.ppm");
        createSliceImage(amr,vec3f(0,0.5f,0),vec3f(1,0,0),vec3f(0,0,1),"centerSlice_y.ppm");
        createSliceImage(amr,vec3f(0.5f,0,0),vec3f(0,1,0),vec3f(0,0,1),"centerSlice_z.ppm");
        return 1;
      }


      ActualArray3D<float> *resampled = new ActualArray3D<float>(dims);
      for (int iz=0;iz<dims.z;iz++)
        for (int iy=0;iy<dims.y;iy++)
          for (int ix=0;ix<dims.x;ix++) {
            vec3f pos((vec3f(ix,iy,iz)+vec3f(.5f))/vec3f(dims));
            resampled->value[resampled->indexOf(vec3i(ix,iy,iz))] = amr->sample(pos);
          }
    
      FILE *out = fopen(outFileName.c_str(),"wb");
      assert(out);
      fwrite(resampled->value,resampled->numElements(),sizeof(float),out);
      fclose(out);

      return 0;
    }
    
  } // ::ospray::amr
} // ::ospary
