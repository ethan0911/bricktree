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

#include "../amr/BrickTreeBuilder.h"

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
      cout << "  ./ospRawSlice <inFile.raw> <args>" << endl;
      cout << "with args:" << endl;
      cout << " -dims <nx> <ny> <nz>   : input dimensions" << endl;
      cout << " --format|-f|-if <uint8|float|double> : input voxel type" << endl;
      cout << " -o <outfilename(ppm)>   : output file name" << endl;
      cout << " -x|-y|-z <sliceCoord(in 0..dims.{x|y|z})>" << endl;
      exit(msg != "");
    }

  // helper function to write the rendered image as PPM file
  void writePPM(const char *fileName,
      const int sizeX, const int sizeY,
      const uint32_t *pixel)
  {
    FILE *file = fopen(fileName, "wb");
    fprintf(file, "P6\n%i %i\n255\n", sizeX, sizeY);
    unsigned char *out = (unsigned char *)alloca(3*sizeX);
    for (int y = 0; y < sizeY; y++) {
      const unsigned char *in = (const unsigned char *)&pixel[(sizeY-1-y)*sizeX];
      for (int x = 0; x < sizeX; x++) {
        out[3*x + 0] = in[4*x + 0];
        out[3*x + 1] = in[4*x + 1];
        out[3*x + 2] = in[4*x + 2];
      }
      fwrite(out, 3*sizeX, sizeof(char), file);
    }
    fprintf(file, "\n");
    fclose(file);
  }

    template<typename T>
    const Array3D<T> *openInput(const std::string &format,
                                const vec3i &dims,
                                const std::string &inFileName)
    {
      const Array3D<T> *input = NULL;
      cout << "mmapping RAW file (" << dims << ":" << typeToString<T>() << "):" << inFileName << endl;
      // cout << "  input file is   " << inFileName[0] << endl;
      // cout << "  expected format " << typeToString<T>() << endl;
      // cout << "  expected dims   " << dims << endl;
        
      if (format == "") {
        input = mmapRAW<T>(inFileName,dims);
      } else if (format == "uint8") {
        const Array3D<uint8> *input_uint8 = mmapRAW<uint8>(inFileName,dims);
        input = new Array3DAccessor<uint8,T>(input_uint8);
      } else if (format == "float") {
        const Array3D<float> *input_float = mmapRAW<float>(inFileName,dims);
        input = new Array3DAccessor<float,T>(input_float);
      } else if (format == "double") {
        const Array3D<double> *input_double = mmapRAW<double>(inFileName,dims);
        input = new Array3DAccessor<double,T>(input_double);
      } else
        throw std::runtime_error("unsupported format '"+format+"'");
        
      // cout << "loading complete." << endl;
      return input;
    }


    extern "C" int main(int ac, char **av)
    {
      int sliceDim = -1;
      int slicePos = -1;
      std::vector<std::string> inFileName;
      std::string outFileName = "";
      std::string inputFormat = "";
      vec3i       dims        = vec3i(0);
      box3i       clipBox(vec3i(-1),vec3i(-1));

      for (int i=1;i<ac;i++) {
        const std::string arg = av[i];
        if (arg == "-o")
          outFileName = av[++i];
        else if (arg == "--format" || arg == "-f" || arg == "-if")
          inputFormat = av[++i];
        else if (arg == "-dims" || arg == "--dimensions") {
          dims.x = atoi(av[++i]);
          dims.y = atoi(av[++i]);
          dims.z = atoi(av[++i]);
        }
        else if (arg == "--clip" || arg == "--sub-box") {
          clipBox.lower.x = atof(av[++i]);
          clipBox.lower.y = atof(av[++i]);
          clipBox.lower.z = atof(av[++i]);
          clipBox.upper = clipBox.lower;
          clipBox.upper.x += atof(av[++i]);
          clipBox.upper.y += atof(av[++i]);
          clipBox.upper.z += atof(av[++i]);
        }
        else if (arg == "-x") {
          sliceDim = 0;
          slicePos = atoi(av[++i]);
        }
        else if (arg == "-y") {
          sliceDim = 1;
          slicePos = atoi(av[++i]);
        }
        else if (arg == "-z") {
          sliceDim = 2;
          slicePos = atoi(av[++i]);
        }
        else if (arg[0] != '-')
          inFileName.push_back(av[i]);
        else
          error("unknown arg '"+arg+"'");
      }
      if (dims == vec3i(0))
        error("no input dimensions (-dims) specified");
      if (inFileName.empty())
        error("no input file(s) specified");
      if (outFileName == "")
        error("no output file specified");

      if (clipBox.lower == vec3i(-1))
        clipBox.lower = vec3i(0);
      if (clipBox.upper == vec3i(-1))
        clipBox.upper = dims;
      else
        clipBox.upper = min(clipBox.upper,dims);
      assert(!clipBox.empty());
      
      if (sliceDim < 0 || sliceDim > 2)
        error("no slice specified");

      if (slicePos < 0 || slicePos >= (clipBox.upper[sliceDim]-clipBox.lower[sliceDim]))
        error("invalid slice position - not in input dims/clipbox");




      int K = sliceDim;
      int U = (sliceDim+1)%3;
      int V = (sliceDim+2)%3;
      vec3i where_00 = vec3i(0); where_00[K] = slicePos;
      vec3i where_du = vec3i(0); where_du[U] = 1;
      vec3i where_dv = vec3i(0); where_dv[V] = 1;
      int NU = clipBox.size()[U];
      int NV = clipBox.size()[V];

      const Array3D<float> *input = openInput<float>(inputFormat,dims,inFileName[0]);
      input = new SubBoxArray3D<float>(input,clipBox);

      Range<float> range = empty;
      for (int v=0;v<NV;v++) 
        for (int u=0;u<NU;u++) {
          const vec3i where = where_00+(u)*where_du+(v)*where_dv;
          float f = input->get(where);
          range.extend(f);
        }
      PRINT(range);
      uint32_t *pixel = new uint32_t[NU*NV];
      for (int v=0;v<NV;v++) 
        for (int u=0;u<NU;u++) {
          float f = input->get(where_00+u*where_du+v*where_dv);
          f = (f - range.lo) / (range.hi-range.lo+1e-10f);
          int i = int(255.f*f+.5f);
          pixel[u+v*NU] = i | (i << 8) | (i << 16) | (i<<24);
        }

      cout << "writing output" << endl;
      writePPM(outFileName.c_str(),NU,NV,pixel);
      delete[] pixel;
      
      return 0;
    }

  }
}
