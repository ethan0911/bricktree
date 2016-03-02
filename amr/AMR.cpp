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

    using std::cout;
    using std::endl;

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
    Range<T> AMR<T>::getValueRange(const int32_t subCellID) const
    {
      Range<T> vox = empty;
      if (subCellID < 0) return vox;

      const typename AMR<T>::OctCell &oc = this->octCell[subCellID];
      for (int iz=0;iz<2;iz++)
        for (int iy=0;iy<2;iy++)
          for (int ix=0;ix<2;ix++) {
            vox.extend(oc.child[iz][iy][ix].ccValue);
            vox.extend(getValueRange(oc.child[iz][iy][ix].childID));
          }
      return vox;
    }
    
    template<typename T>
    Range<T> AMR<T>::getValueRange(const vec3i &rootCellID) const
    {
      size_t idx = rootCellID.x + dimensions.x*(rootCellID.y+dimensions.y*rootCellID.z);
      Range<T> range = rootCell[idx].ccValue;
      range.extend(this->getValueRange(rootCell[idx].childID));
      return range;
    }
    
    template<typename T>
    Range<T> AMR<T>::getValueRange() const
    {
      Range<T> vox = empty;
      for (int iz=0;iz<dimensions.z;iz++)
        for (int iy=0;iy<dimensions.y;iy++)
          for (int ix=0;ix<dimensions.x;ix++)
            vox.extend(this->getValueRange(vec3i(ix,iy,iz)));
      return vox;
    }



    template<typename T>
    // assuming that pos is in 0,0,0-1,1,1
    /* this variant samples into the root cells ONLY, with nearest neighbor sampling */
    float AMR<T>::sampleRootCellOnly(const vec3f &unitPos) const 
    { 
      // cout << "-------------------------------------------------------" << endl;
      // PRINT(unitPos);

      const vec3f gridPos = unitPos * vec3f(dimensions);
      // PRINT(gridPos);
      const vec3f floorGridPos = floor(clamp(gridPos,vec3f(0.f),vec3f(dimensions)-vec3f(1.f)));
      const vec3i gridIdx = vec3i(floorGridPos);
      // PRINT(gridIdx);
      vec3f frac = gridPos - floorGridPos;

      const int cellID = gridIdx.x + dimensions.x*(gridIdx.y + dimensions.y*(gridIdx.z));
      // PRINT(cellID);
      // PRINT(rootCell[cellID].ccValue);
      return rootCell[cellID].ccValue;
    }

    template<typename T>
    /*! this version will sample down to whatever octree leaf gets
        hit, but without interpolation, just nearest neighbor */
    // assuming that pos is in 0,0,0-1,1,1
    float AMR<T>::sampleFinestLeafNearestNeighbor(const vec3f &unitPos) const 
    { 
      // cout << "-------------------------------------------------------" << endl;
      // PRINT(unitPos);

      const vec3f gridPos = unitPos * vec3f(dimensions);
      // PRINT(gridPos);
      const vec3f floorGridPos = floor(clamp(gridPos,vec3f(0.f),vec3f(dimensions)-vec3f(1.f)));
      const vec3i gridIdx = vec3i(floorGridPos);
      // PRINT(gridIdx);
      vec3f frac = gridPos - floorGridPos;

      const int cellID = gridIdx.x + dimensions.x*(gridIdx.y + dimensions.y*(gridIdx.z));
      const typename AMR<T>::Cell *cell = &rootCell[cellID];
      while (cell->childID >= 0) {
        const typename AMR<T>::OctCell &oc = this->octCell[cell->childID];
        int ix=0, iy=0, iz=0;
        if (frac.x >= .5f) { ix = 1; frac.x -= .5f; }
        if (frac.y >= .5f) { iy = 1; frac.y -= .5f; }
        if (frac.z >= .5f) { iz = 1; frac.z -= .5f; }
        frac *= 2.f;

        cell = &oc.child[iz][iy][ix];
      }
      return cell->ccValue;
    }

    template<typename T>
    const typename AMR<T>::Cell *AMR<T>::findCell(const AMR<T>::CellIdx &requestedIdx, 
                                                  AMR<T>::CellIdx &actualIdx) const 
    { 
      actualIdx = requestedIdx.rootAncestorIndex();
      int trailBit = 1 << (requestedIdx.m);

      const int cellID = actualIdx.x + dimensions.x*(actualIdx.y + dimensions.y*(actualIdx.z));
      const typename AMR<T>::Cell *cell = &rootCell[cellID];

      while (actualIdx.m < requestedIdx.m && cell->childID >= 0) {
        const typename AMR<T>::OctCell &oc = this->octCell[cell->childID];

        const int ix = (requestedIdx.x & trailBit) ? 1 : 0;
        const int iy = (requestedIdx.y & trailBit) ? 1 : 0;
        const int iz = (requestedIdx.z & trailBit) ? 1 : 0;
        cell = &oc.child[iz][iy][ix];

        actualIdx.x = 2*actualIdx.x + ix;
        actualIdx.y = 2*actualIdx.y + iy;
        actualIdx.z = 2*actualIdx.z + iz;
        actualIdx.m ++;

        trailBit >>= 1;
      }

      return cell;
    }

    template<typename T>
    const typename AMR<T>::Cell *AMR<T>::findLeafCell(const vec3f &gridPos, AMR<T>::CellIdx &cellIdx) const 
    { 
      // const vec3f gridPos = unitPos * vec3f(dimensions);
      const vec3f floorGridPos = floor(clamp(gridPos,vec3f(0.f),vec3f(dimensions)-vec3f(1.f)));
      const vec3i gridIdx = vec3i(floorGridPos);
      vec3f frac = gridPos - floorGridPos;
      cellIdx = CellIdx(gridIdx,0);

      const int cellID = gridIdx.x + dimensions.x*(gridIdx.y + dimensions.y*(gridIdx.z));
      const typename AMR<T>::Cell *cell = &rootCell[cellID];
      while (cell->childID >= 0) {
        const typename AMR<T>::OctCell &oc = this->octCell[cell->childID];
        int ix=0, iy=0, iz=0;
        if (frac.x >= .5f) { ix = 1; frac.x -= .5f; }
        if (frac.y >= .5f) { iy = 1; frac.y -= .5f; }
        if (frac.z >= .5f) { iz = 1; frac.z -= .5f; }
        frac *= 2.f;

        cellIdx.x = 2*cellIdx.x + ix;
        cellIdx.y = 2*cellIdx.y + iy;
        cellIdx.z = 2*cellIdx.z + iz;
        cellIdx.m ++;

        cell = &oc.child[iz][iy][ix];
      }
      return cell; 
    }

    template<typename T>
    /*! this version will sample down to whatever octree leaf gets
        hit, but without interpolation, just nearest neighbor */
    // assuming that pos is in 0,0,0-1,1,1
    vec3f AMR<T>::sample(const vec3f &unitPos) const 
    { 
      const vec3f gridPos = unitPos * vec3f(dimensions);

      CellIdx cellIdx;
      const AMR<T>::Cell *cell = findLeafCell(gridPos,cellIdx);
      const vec3f cellCenter = cellIdx.centerPos();

      const int dx = unitPos.x < cellCenter.x ? -1 : +1;
      const int dy = unitPos.y < cellCenter.y ? -1 : +1;
      const int dz = unitPos.z < cellCenter.z ? -1 : +1;

      // corner cells - 0,0,0 is the current cell, the others are the
      // neihgbors in +/- x/y/z; 'ptr' is the pointer to the cell,
      // 'idx' its logical index
      const Cell *cCellPtr[2][2][2];
      CellIdx     cCellIdx[2][2][2];

      cCellPtr[0][0][0] = cell;
      cCellIdx[0][0][0] = cellIdx;

      for (int iz=0;iz<2;iz++)
        for (int iy=0;iy<2;iy++)
          for (int ix=0;ix<2;ix++) {
            if (ix==0 && iy==0 && iz==0) 
              // we already have this cell ...
              continue;
            cCellPtr[iz][iy][ix] = findCell(cellIdx.neighborIndex(vec3i(ix*dx,iy*iy,ix*dz)),
                                            cCellIdx[iz][iy][ix]);
          }
      // now, have all 8 neighbor cells of octant. get the 8 octant values
      float       octValue[2][2][2];
      octValue[0][0][0] = cell->ccValue;
      for (int iz=0;iz<2;iz++)
        for (int iy=0;iy<2;iy++)
          for (int ix=0;ix<2;ix++) {
            // for now, just use the cell, whatever level it might be on - THIS IS WRONG
            octValue[iz][iy][ix] = cCellPtr[iz][iy][ix]->ccValue;
          }
      
      // compute interpolation weights
      const float octWidth = 0.5f / (1 << cellIdx.m); // note we actually only need 1/octWidth below)
      const float fx = fabsf(cellCenter.x-gridPos.x) / octWidth;
      const float fy = fabsf(cellCenter.y-gridPos.y) / octWidth;
      const float fz = fabsf(cellCenter.z-gridPos.z) / octWidth;
            
      return vec3f(fx,fy,fz);
      return vec3f(cell->ccValue) / 255.f;
    }


    template struct AMR<float>;
    template struct AMR<uint8>;

  } // ::ospray::amr
} // ::ospray
