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
#include "AMR.h"

namespace ospray {
  namespace amr {

    template<typename T>
    const char *formatNameString();

    template<>
    const char *formatNameString<uint8>() { return "int8"; }
    template<>
    const char *formatNameString<float>() { return "float"; }

    template<typename T>
    void AMR<T>::writeTo(const std::string &outFileName)
    {
      const std::string binFileName = outFileName+"bin";
      FILE *bin = fopen(binFileName.c_str(),"wb");
      FILE *osp = fopen(outFileName.c_str(),"w");

      fwrite(&dimensions,sizeof(dimensions),1,bin);

      size_t numRootCells = this->rootCell.size();
      fwrite(&numRootCells,sizeof(numRootCells),1,bin);
      fwrite(&rootCell[0],rootCell.size()*sizeof(rootCell[0]),1,bin);

      size_t numOctCells = this->octCell.size();
      fwrite(&numOctCells,sizeof(numOctCells),1,bin);
      fwrite(&octCell[0],octCell.size()*sizeof(octCell[0]),1,bin);

      size_t dataSize = ftell(bin);

      fprintf(osp,"<?xml?>\n");
      fprintf(osp,"<ospray>\n");
      fprintf(osp,"  <MultiOctreeAMR format=\"%s\"\n",formatNameString<T>());
      fprintf(osp,"             size=\"%li\" ofs=\"0\"\n",dataSize);
      fprintf(osp,"             />\n");
      fprintf(osp,"</ospray>\n");
      
      fclose(bin);
      fclose(osp);
    }

    template struct AMR<float>;
    template struct AMR<uint8>;

  }
}
