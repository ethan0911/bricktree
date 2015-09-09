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

      size_t numGrids = gridVec.size();
      fwrite(&numGrids,sizeof(size_t),1,bin);
      for (int i=0;i<numGrids;i++) {
        Grid *grid = gridVec[i];
        fwrite(&grid->cellDims,sizeof(grid->cellDims),1,bin);
        fwrite(&grid->numCells,sizeof(grid->numCells),1,bin);
        fwrite(grid->cell,sizeof(*grid->cell),grid->numCells,bin);
        fwrite(&grid->voxelDims,sizeof(grid->voxelDims),1,bin);
        fwrite(&grid->numVoxels,sizeof(grid->numVoxels),1,bin);
        fwrite(grid->voxel,sizeof(*grid->voxel),grid->numVoxels,bin);
      }

      
      size_t dataSize = ftell(bin);

      fprintf(osp,"<?xml?>\n");
      fprintf(osp,"<ospray>\n");
      fprintf(osp,"  <AMRVolume format=\"%s\"\n",formatNameString<T>());
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
