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
    void readMem(const unsigned char *&ptr, T &t)
    {
      memcpy(&t,ptr,sizeof(t));
      ptr += sizeof(t);
    }
    template<typename T>
    void readMem(const unsigned char *&ptr, std::vector<T> &t, size_t num)
    {
      t.resize(num);
      memcpy(&t[0],ptr,num*sizeof(T));
      ptr += num*sizeof(T);
    }

    template<typename voxel_t>
    AMR<voxel_t> *AMR<voxel_t>::loadFrom(const char *fileName)
    {
      FILE *file = fopen(fileName,"rb");
      fseek(file,0,SEEK_END);
      size_t sz = ftell(file);
      fseek(file,0,SEEK_SET);
      void *mem = malloc(sz);
      size_t rc = fread(mem,1,sz,file);
      assert(rc == sz);
      fclose(file);
      AMR<voxel_t> *amr = new AMR<voxel_t>;
      amr->mmapFrom((unsigned char *)mem);
      return amr;
    }

    template<typename T>
    void AMR<T>::mmapFrom(const unsigned char *mmappedPtr)
    {
      const unsigned char *mem = mmappedPtr;
      readMem(mem,this->dimensions);
      PRINT(dimensions);

      size_t numRootCells;
      readMem(mem,numRootCells);
      PRINT(numRootCells);
      readMem(mem,this->rootCell,numRootCells);

      size_t numOctCells;
      readMem(mem,numOctCells);
      PRINT(numOctCells);
      readMem(mem,this->octCell,numOctCells);
    }

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
      fprintf(osp,"  <MultiOctreeAMR voxelType=\"%s\"\n",formatNameString<T>());
      fprintf(osp,"             size=\"%li\" ofs=\"0\"\n",dataSize);
      fprintf(osp,"             />\n");
      fprintf(osp,"</ospray>\n");
      
      fclose(bin);
      fclose(osp);
    }

    template<typename T>
    float AMR<T>::sample(const vec3f &pos) const 
    { assert(0); return .5f; }

    template struct AMR<float>;
    template struct AMR<uint8>;

  } // ::ospray::amr
} // ::ospray
