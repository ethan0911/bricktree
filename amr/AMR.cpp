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

    AMR *AMR::loadFrom(const char *fileName)
    {
      FILE *file = fopen(fileName,"rb");
      fseek(file,0,SEEK_END);
      size_t sz = ftell(file);
      fseek(file,0,SEEK_SET);
      void *mem = malloc(sz);
      size_t rc = fread(mem,1,sz,file);
      assert(rc == sz);
      fclose(file);
      AMR *amr = new AMR;
      amr->mmapFrom((unsigned char *)mem);
      return amr;
    }

    void AMR::mmapFrom(const unsigned char *mmappedPtr)
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

    void AMR::writeTo(const std::string &outFileName)
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
      fprintf(osp,"  <MultiOctreeAMR voxelType=\"float\"\n");
      fprintf(osp,"             size=\"%li\" ofs=\"0\"\n",dataSize);
      fprintf(osp,"             />\n");
      fprintf(osp,"</ospray>\n");
      
      fclose(bin);
      fclose(osp);
    }



    Range<float> AMR::getValueRange(const int32_t subCellID) const
    {
      Range<float> vox = empty;
      if (subCellID < 0) return vox;

      const AMR::OctCell &oc = this->octCell[subCellID];
      for (int iz=0;iz<2;iz++)
        for (int iy=0;iy<2;iy++)
          for (int ix=0;ix<2;ix++) {
            vox.extend(oc.child[iz][iy][ix].ccValue);
            vox.extend(getValueRange(oc.child[iz][iy][ix].childID));
          }
      return vox;
    }
    
    Range<float> AMR::getValueRange(const vec3i &rootCellID) const
    {
      size_t idx = rootCellID.x + dimensions.x*(rootCellID.y+dimensions.y*rootCellID.z);
      Range<float> range = rootCell[idx].ccValue;
      range.extend(this->getValueRange(rootCell[idx].childID));
      return range;
    }
    
    Range<float> AMR::getValueRange() const
    {
      Range<float> vox = empty;
      for (int iz=0;iz<dimensions.z;iz++)
        for (int iy=0;iy<dimensions.y;iy++)
          for (int ix=0;ix<dimensions.x;ix++)
            vox.extend(this->getValueRange(vec3i(ix,iy,iz)));
      return vox;
    }
    float AMR::sample(const vec3f &unitPos)
    {
      throw std::runtime_error("sample() not (re-)implemented in scalar form, yet (after throwing out all template code)");
      return 0.f;
    }



//     // assuming that pos is in 0,0,0-1,1,1
//     /* this variant samples into the root cells ONLY, with nearest neighbor sampling */
//     float AMR<float>::sampleRootCellOnly(const vec3f &unitPos) const 
//     { 
//       // cout << "-------------------------------------------------------" << endl;
//       // PRINT(unitPos);

//       const vec3f gridPos = unitPos * vec3f(dimensions);
//       // PRINT(gridPos);
//       const vec3f floorGridPos = floor(clamp(gridPos,vec3f(0.f),vec3f(dimensions)-vec3f(1.f)));
//       const vec3i gridIdx = vec3i(floorGridPos);
//       // PRINT(gridIdx);
//       vec3f frac = gridPos - floorGridPos;

//       const int cellID = gridIdx.x + dimensions.x*(gridIdx.y + dimensions.y*(gridIdx.z));
//       // PRINT(cellID);
//       // PRINT(rootCell[cellID].ccValue);
//       return rootCell[cellID].ccValue;
//     }

//     template<typename T>
//     /*! this version will sample down to whatever octree leaf gets
//         hit, but without interpolation, just nearest neighbor */
//     // assuming that pos is in 0,0,0-1,1,1
//     float AMR<T>::sampleFinestLeafNearestNeighbor(const vec3f &unitPos) const 
//     { 
//       // cout << "-------------------------------------------------------" << endl;
//       // PRINT(unitPos);

//       const vec3f gridPos = unitPos * vec3f(dimensions);
//       // PRINT(gridPos);
//       const vec3f floorGridPos = floor(clamp(gridPos,vec3f(0.f),vec3f(dimensions)-vec3f(1.f)));
//       const vec3i gridIdx = vec3i(floorGridPos);
//       // PRINT(gridIdx);
//       vec3f frac = gridPos - floorGridPos;

//       const int cellID = gridIdx.x + dimensions.x*(gridIdx.y + dimensions.y*(gridIdx.z));
//       const typename AMR<T>::Cell *cell = &rootCell[cellID];
//       while (cell->childID >= 0) {
//         const typename AMR<T>::OctCell &oc = this->octCell[cell->childID];
//         int ix=0, iy=0, iz=0;
//         if (frac.x >= .5f) { ix = 1; frac.x -= .5f; }
//         if (frac.y >= .5f) { iy = 1; frac.y -= .5f; }
//         if (frac.z >= .5f) { iz = 1; frac.z -= .5f; }
//         frac *= 2.f;

//         cell = &oc.child[iz][iy][ix];
//       }
//       return cell->ccValue;
//     }

//     template<typename T>
//     const typename AMR<T>::Cell *AMR<T>::findCell(const AMR<T>::CellIdx &requestedIdx, 
//                                                   AMR<T>::CellIdx &actualIdx) const 
//     { 
//       actualIdx = requestedIdx.rootAncestorIndex();
      
//       if (actualIdx.x < 0 || 
//           actualIdx.y < 0 || 
//           actualIdx.z < 0 ||
//           actualIdx.x >= dimensions.x ||
//           actualIdx.y >= dimensions.y ||
//           actualIdx.z >= dimensions.z) 
//         {
//           actualIdx = CellIdx::invalidIndex();
//           return NULL;
//         }

//       int trailBit = 1 << (requestedIdx.m);

//       const int cellID = actualIdx.x + dimensions.x*(actualIdx.y + dimensions.y*(actualIdx.z));
//       const typename AMR<T>::Cell *cell = &rootCell[cellID];

//       while (actualIdx.m < requestedIdx.m && cell->childID >= 0) {

//         trailBit >>= 1;

//         const typename AMR<T>::OctCell &oc = this->octCell[cell->childID];

//         const int ix = (requestedIdx.x & trailBit) ? 1 : 0;
//         const int iy = (requestedIdx.y & trailBit) ? 1 : 0;
//         const int iz = (requestedIdx.z & trailBit) ? 1 : 0;
//         cell = &oc.child[iz][iy][ix];

//         actualIdx.x = 2*actualIdx.x + ix;
//         actualIdx.y = 2*actualIdx.y + iy;
//         actualIdx.z = 2*actualIdx.z + iz;
//         actualIdx.m ++;

//       }

//       return cell;
//     }

//     template<typename T>
//     const typename AMR<T>::Cell *AMR<T>::findLeafCell(const vec3f &gridPos, AMR<T>::CellIdx &cellIdx) const 
//     { 
//       // const vec3f gridPos = unitPos * vec3f(dimensions);
//       const vec3f floorGridPos = floor(clamp(gridPos,vec3f(0.f),vec3f(dimensions)-vec3f(1.f)));
//       const vec3i gridIdx = vec3i(floorGridPos);
//       vec3f frac = gridPos - floorGridPos;
//       cellIdx = CellIdx(gridIdx,0);

//       const int cellID = gridIdx.x + dimensions.x*(gridIdx.y + dimensions.y*(gridIdx.z));
//       const typename AMR<T>::Cell *cell = &rootCell[cellID];
//       while (cell->childID >= 0) {
//         const typename AMR<T>::OctCell &oc = this->octCell[cell->childID];
//         int ix=0, iy=0, iz=0;
//         if (frac.x >= .5f) { ix = 1; frac.x -= .5f; }
//         if (frac.y >= .5f) { iy = 1; frac.y -= .5f; }
//         if (frac.z >= .5f) { iz = 1; frac.z -= .5f; }
//         frac *= 2.f;

//         cellIdx.x = 2*cellIdx.x + ix;
//         cellIdx.y = 2*cellIdx.y + iy;
//         cellIdx.z = 2*cellIdx.z + iz;
//         cellIdx.m ++;

//         cell = &oc.child[iz][iy][ix];
//       }
//       return cell; 
//     }

//     template<typename T>
//     struct Octant {
//       typedef typename AMR<T>::Cell Cell;
//       typedef typename AMR<T>::CellIdx CellIdx;

//       const AMR<T> *amr;

//       // corner cells - 0,0,0 is the current cell, the others are the
//       // neihgbors in +/- x/y/z; 'ptr' is the pointer to the cell,
//       // 'idx' its logical index
//       const Cell *cCellPtr[2][2][2];
//       CellIdx     cCellIdx[2][2][2];
      
//       // octant value for interpolation
//       float       octValue[2][2][2];
      
//       Octant(const AMR<T> *amr)
//         : amr(amr)
//       {}

//       void faceNeighbor(const vec3i &d /*!< the delta in which we go */)
//       {
//         const Cell *cPtr = cCellPtr[0][0][0];
//         CellIdx     cIdx = cCellIdx[0][0][0];
        
//         const Cell *nPtr = cCellPtr[d.z][d.y][d.x];
//         CellIdx     nIdx = cCellIdx[d.z][d.y][d.x];

//         if (nIdx.m == cIdx.m) {
//           // neighbor does exist on this level
//           octValue[d.z][d.y][d.x] = 0.5f*(cPtr->ccValue+nPtr->ccValue);
//         } else {
//           // THIS IS STILL WRONG
//           octValue[d.z][d.y][d.x] = 0.5f*(cPtr->ccValue+nPtr->ccValue);
//         }
//       }

//       /* TODO: make du and dv template args; all this is hardcoded and
//          can be inlined */
//       void edgeNeighbor(const vec3i &du, const vec3i &dv  /*!< the delta in which we go */)
//       {
//         const vec3i duv = du + dv; // actually, a logical 'or' is what
//                                    // we're looking for - can do that
//                                    // through templates later on

//         // cell itself
//         const Cell *cell = cCellPtr[0][0][0];
//         CellIdx     idx = cCellIdx[0][0][0];

//         // cell in du direction
//         const Cell *cell_du = cCellPtr[du.z][du.y][du.x];
//         CellIdx     idx_du = cCellIdx[du.z][du.y][du.x];

//         // cell in dv direction
//         const Cell *cell_dv = cCellPtr[dv.z][dv.y][dv.x];
//         CellIdx     idx_dv = cCellIdx[dv.z][dv.y][dv.x];

//         // cell in du+dv direction
//         const Cell *cell_duv = cCellPtr[duv.z][duv.y][duv.x];
//         CellIdx     idx_duv = cCellIdx[duv.z][duv.y][duv.x];

//         int minLevel = min(min(idx_du.m,idx_dv.m),idx_duv.m);
//         if (minLevel == idx.m) {
//           // matching
//           octValue[duv.z][duv.y][duv.x] = 0.25f*(cell->ccValue+
//                                                  cell_du->ccValue+
//                                                  cell_dv->ccValue+
//                                                  cell_duv->ccValue);
//         } else {
//           // THIS IS STILL WRONG
//           octValue[duv.z][duv.y][duv.x] = 0.25f*(cell->ccValue+
//                                                  cell_du->ccValue+
//                                                  cell_dv->ccValue+
//                                                  cell_duv->ccValue);
//         }
//       }

//       void cornerNeighbor()
//       {
//         octValue[1][1][1] = 0.125f*(cCellPtr[0][0][0]->ccValue+
//                                     cCellPtr[0][0][1]->ccValue+
//                                     cCellPtr[0][1][0]->ccValue+
//                                     cCellPtr[0][1][1]->ccValue+
//                                     cCellPtr[1][0][0]->ccValue+
//                                     cCellPtr[1][0][1]->ccValue+
//                                     cCellPtr[1][1][0]->ccValue+
//                                     cCellPtr[1][1][1]->ccValue);
//       }

//       vec3f interpolate(const vec3f &gridPos) 
//       {
//         CellIdx cellIdx;
//         const Cell *cell = amr->findLeafCell(gridPos,cellIdx);
//         const vec3f cellCenter = cellIdx.centerPos();
        
//         const int dx = gridPos.x < cellCenter.x ? -1 : +1;
//         const int dy = gridPos.y < cellCenter.y ? -1 : +1;
//         const int dz = gridPos.z < cellCenter.z ? -1 : +1;

//         cCellPtr[0][0][0] = cell;
//         cCellIdx[0][0][0] = cellIdx;
        
//         // first: first all neighbor cells (or their ancestors if they do not exist
//         for (int iz=0;iz<2;iz++)
//           for (int iy=0;iy<2;iy++)
//             for (int ix=0;ix<2;ix++) {
//               if (ix==0 && iy==0 && iz==0) 
//                 // we already have this cell ...
//                 continue;
//               cCellPtr[iz][iy][ix] = amr->findCell(cellIdx.neighborIndex(vec3i(ix*dx,
//                                                                                iy*dy,
//                                                                                iz*dz)),
//                                                    cCellIdx[iz][iy][ix]);
//             }
        
// #if 0
//         // =======================================================
//         // now, have all 8 neighbor cells of octant. get the 8 interpolated octant values
//         // root of octant itself
//         octValue[0][0][0] = cell->ccValue;
//         // ..................................................................
//         // face neighbors
//         faceNeighbor(vec3i(1,0,0));
//         faceNeighbor(vec3i(0,1,0));
//         faceNeighbor(vec3i(0,0,1));
//         // ..................................................................
//         // edge neighbors
//         edgeNeighbor(vec3i(1,0,0),vec3i(0,1,0));
//         edgeNeighbor(vec3i(0,1,0),vec3i(0,0,1));
//         edgeNeighbor(vec3i(0,0,1),vec3i(1,0,0));
//         // ..................................................................
//         // corner neighbors
//         cornerNeighbor();
// #endif

//         // =======================================================
//         // compute interpolation weights
        
// #if 0
//         const float octWidth = 0.5f / (1 << cellIdx.m); // note we actually only need 1/octWidth below)
//         const float fx = fabsf(cellCenter.x-gridPos.x) / octWidth;
//         const float fy = fabsf(cellCenter.y-gridPos.y) / octWidth;
//         const float fz = fabsf(cellCenter.z-gridPos.z) / octWidth;
//         return vec3f(fx,fy,fz);
// #else
//         const float octWidth = 1.f / (1 << cellIdx.m); // note we actually only need 1/octWidth below)
//         const float fx = fabsf(cellCenter.x-gridPos.x) / octWidth;
//         const float fy = fabsf(cellCenter.y-gridPos.y) / octWidth;
//         const float fz = fabsf(cellCenter.z-gridPos.z) / octWidth;

//         const float f000 = cCellPtr[0][0][0]->ccValue;
//         const float f001 = cCellPtr[0][0][1]->ccValue;
//         const float f010 = cCellPtr[0][1][0]->ccValue;
//         const float f011 = cCellPtr[0][1][1]->ccValue;
//         const float f100 = cCellPtr[1][0][0]->ccValue;
//         const float f101 = cCellPtr[1][0][1]->ccValue;
//         const float f110 = cCellPtr[1][1][0]->ccValue;
//         const float f111 = cCellPtr[1][1][1]->ccValue;
//         return
//           (1.f/255.f)*vec3f((1.f-fz)*((1.f-fy)*((1.f-fx)*f000+
//                                                 (    fx)*f001)+
//                                       (    fy)*((1.f-fx)*f010+
//                                                 (    fx)*f011))+
//                             (    fz)*((1.f-fy)*((1.f-fx)*f100+
//                                                 (    fx)*f101)+
//                                       (    fy)*((1.f-fx)*f110+
//                                                 (    fx)*f111)));
        
// #endif
//       }
      
//     };

//     template<typename T>
//     /*& recrusively trickly down the tree, accumulating all leaves'
//       hat-shaped basis functions that overlap our sample pos */
//     void AMR<T>::accumulateHatBasisFunctions(const vec3f &gridPos,
//                                              const Cell *cell,
//                                              const CellIdx &cellIdx,
//                                              float &num, float &den) const
//     {
//       const vec3f cellCenter = cellIdx.centerPos();
//       // PRINT(cellIdx);
//       // PRINT(cellCenter);
//       const vec3f delta = abs(cellCenter - gridPos);
//       const float supportWidth = 1.f/(1<<cellIdx.m);

// #if 0
//       // PRINT(cell->ccValue);
//       const vec3f weights = max(vec3f(1.f) - delta * (1.f/supportWidth),vec3f(0.f));
//       const float w = weights.x * weights.y * weights.z;
//       num += w * cell->ccValue;
//       den += w;
//       return;
// #endif
//       // PRINT(delta);
//       // PRINT(supportWidth);
//       /* FIXME: this following print doesn't resolve w/ my compiler, even though
//          it should (see ../amr/Octree.h) 

// wald@latte ~/Projects/ospray/modules/module-amr-volume $ icpc --version
// icpc (ICC) 15.0.3 20150407
// Copyright (C) 1985-2015 Intel Corporation.  All rights reserved.
//       */
//       // PRINT(cell);
//       // if (cellIdx.m == 0) 
//       //   PRINT(cell-&rootCell[0]);
//       // else
//       //   PRINT(cell-&octCell[0].child[0][0][0]);
//       if (reduce_max(delta) > supportWidth) return;

//       // PRINT(cell->childID);
//       if (cell->childID >= 0) {
//         const OctCell &oc = octCell[cell->childID];
//         for (int iz=0;iz<2;iz++)
//           for (int iy=0;iy<2;iy++)
//             for (int ix=0;ix<2;ix++) {
//               CellIdx ci = cellIdx.childIndex(vec3i(ix,iy,iz));
//               // cout << "child[" << vec3i(ix,iy,iz) << "] = " << ci << endl;
//               accumulateHatBasisFunctions(gridPos,&oc.child[iz][iy][ix],
//                                           ci,num,den);
//             }
//       } else {
//         // an actual leaf cell - evaluate the weights, and accumulate
//         const vec3f weights = vec3f(1.f) - delta * (1.f/supportWidth);
//         const float w = weights.x * weights.y * weights.z;
//         if (cellIdx.m > 0) {
//           cout << "LEAF" << endl;
//           // PRINT(w);
//           PRINT(cellIdx);
//           PRINT(cell->ccValue);
//           cout << "ofs " << (cell - &octCell[0].child[0][0][0]) << endl;
//         }        
//         // if (cell->ccValue != 0.f) {
//         //   exit(0);
//         // }
//         den += w;
//         num += w * cell->ccValue;
//         // PRINT(num);
//       }
//     }

//     template<typename T>
//     void AMR<T>::accumulateHatBasisFunctions(const vec3f &gridPos,
//                                              float &num, float &den) const
//     {
//       // cout << "=======================================================" << endl;
//       const vec3f floorGridPos = floor(clamp(gridPos,vec3f(0.f),vec3f(dimensions)-vec3f(1.f)));
//       const vec3i gridIdx = vec3i(floorGridPos);
//       const CellIdx cellIdx = CellIdx(gridIdx,0);
//       const vec3f cellCenter = cellIdx.centerPos();
      
//       // PRINT(gridPos);
//       // PRINT(cellIdx);

//       const int dx = gridPos.x < cellCenter.x ? -1 : +1;
//       const int dy = gridPos.y < cellCenter.y ? -1 : +1;
//       const int dz = gridPos.z < cellCenter.z ? -1 : +1;
       
//       // PING;

//       // first: first all neighbor cells (or their ancestors if they do not exist
//       for (int iz=0;iz<2;iz++)
//         for (int iy=0;iy<2;iy++)
//           for (int ix=0;ix<2;ix++) {
//             CellIdx rootIdx = cellIdx.neighborIndex(vec3i(ix*dx,iy*dy,iz*dz));
//             CellIdx cellIdx;
//             const Cell *cell = findCell(rootIdx,cellIdx);
//             // cout << "Neighbor[" << vec3f(ix,iy,iz) << "] = " << rootIdx << "/" << cell << endl;

//             // PRINT(rootIdx);
//             // PRINT(cell);
//             if (!cell) continue;
            
//       // if (rootIdx.m == 0) 
//       //   PRINT(cell-&rootCell[0]);
//       // else
//       //   PRINT(cell-&octCell[0].child[0][0][0]);

//             accumulateHatBasisFunctions(gridPos,cell,cellIdx,num,den);
//           }
//       den += 1e-8f;
//     }

  } // ::ospray::amr
} // ::ospray
