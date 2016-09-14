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

// own
#include "BrickTree.h"

namespace ospray {
  namespace bt {

    using std::cout;
    using std::endl;
    using std::flush;

    template<>
    const char *typeToString<uint8>() { return "uint8"; };
    template<>
    const char *typeToString<float>() { return "float"; };
    template<>
    const char *typeToString<double>() { return "double"; };


    BrickTreeBase *BrickTreeBase::mapFrom(const void *ptr, 
                                          const std::string &format, 
                                          int BS, 
                                          size_t superBlockOfs)
    {
      throw std::runtime_error("not implemented yet...");
    }
    


    // template<int N, typename T>
    // void BrickTree<N,T>::saveTo(FILE *bin, size_t &superBlockOfs)
    // {
    //   throw std::runtime_error("not implemented yet...");
    // }


    template<int N, typename T>
    void BrickTree<N,T>::ValueBrick::clear()
    {
      for (int iz=0;iz<N;iz++)
        for (int iy=0;iy<N;iy++)
          for (int ix=0;ix<N;ix++)
            value[iz][iy][ix] = 0.f;
    }

    template<int N, typename T>
    void BrickTree<N,T>::IndexBrick::clear()
    {
      for (int iz=0;iz<N;iz++)
        for (int iy=0;iy<N;iy++)
          for (int ix=0;ix<N;ix++)
            childID[iz][iy][ix] = invalidID();
    }

    // template<int N, typename T>
    // Range<float> BrickTree<N,T>::ValueBrick::getValueRange() const
    // {
    //   Range<float> r = empty;
    //   for (int iz=0;iz<N;iz++)
    //     for (int iy=0;iy<N;iy++)
    //       for (int ix=0;ix<N;ix++)
    //         r.extend(value[iz][iy][ix]);
    //   return r;
    // }

    template<int N, typename T>
    void BrickTree<N,T>::mapFrom(const unsigned char *ptr, 
                           const vec3i &rootGrid,
                           const vec3f &fracOfRootGrid,
                           std::vector<int32_t> numValueBricksPerTree,
                           std::vector<int32_t> numIndexBricksPerTree)
    {
      this->rootGridDims = rootGrid;
      PRINT(rootGridDims);
      this->validFractionOfRootGrid = fracOfRootGrid;

      const size_t numRootCells
        = (size_t)rootGridDims.x 
        * (size_t)rootGridDims.y
        * (size_t)rootGridDims.z; 
      int32_t *firstIndexBrickOfTree = new int32_t[numRootCells];
      int32_t *firstValueBrickOfTree = new int32_t[numRootCells];
      size_t sumIndex = 0;
      size_t sumValue = 0;
      for (size_t i=0;i<numRootCells;i++) {
        firstIndexBrickOfTree[i] = sumIndex;
        firstValueBrickOfTree[i] = sumValue;
        sumValue += numValueBricksPerTree[i];
        sumIndex += numIndexBricksPerTree[i];
      }
      this->firstIndexBrickOfTree = firstIndexBrickOfTree;
      this->firstValueBrickOfTree = firstValueBrickOfTree;
      
      numValueBricks = sumValue;
      numIndexBricks = sumIndex;
      
      valueBrick = (const ValueBrick *)ptr;
      ptr += numValueBricks * sizeof(ValueBrick);
      
      indexBrick = (const IndexBrick *)ptr;
      ptr += numIndexBricks * sizeof(IndexBrick);
      
      brickInfo = (const BrickInfo *)ptr;
    }      

    template<int N, typename T>
    double BrickTree<N,T>::ValueBrick::computeWeightedAverage(// coordinates of lower-left-front
                                                             // voxel, in resp level
                                                             const vec3i &brickCoord,
                                                             // size of bricks in current level
                                                             const int brickSize,
                                                             // maximum size in finest level
                                                             const vec3i &maxSize) const
    {
      int cellSize = brickSize / N;
      
      double num = 0.;
      double den = 0.;
      
      for (int iz=0;iz<N;iz++)
        for (int iy=0;iy<N;iy++)
          for (int ix=0;ix<N;ix++) {
            vec3i cellBegin = min(brickCoord*vec3i(brickSize)+vec3i(ix,iy,iz)*vec3i(cellSize),maxSize);
            vec3i cellEnd = min(cellBegin + vec3i(cellSize),maxSize);

            float weight = reduce_mul(cellEnd-cellBegin);
            if (weight > 0.f) {
              den += weight;
              num += weight * value[iz][iy][ix];
            }
          }

      den += 1e-8;
      assert(den != 0.);
      return num / den;
    }

    template const char *typeToString<uint8>();
    template const char *typeToString<float>();
    template const char *typeToString<double>();

    template struct BrickTree<2,uint8>;
    template struct BrickTree<4,uint8>;
    template struct BrickTree<8,uint8>;
    template struct BrickTree<16,uint8>;
    template struct BrickTree<32,uint8>;
    template struct BrickTree<64,uint8>;

    template struct BrickTree<2,float>;
    template struct BrickTree<4,float>;
    template struct BrickTree<8,float>;
    template struct BrickTree<16,float>;
    template struct BrickTree<32,float>;
    template struct BrickTree<64,float>;

    template struct BrickTree<2,double>;
    template struct BrickTree<4,double>;
    template struct BrickTree<8,double>;
    template struct BrickTree<16,double>;
    template struct BrickTree<32,double>;
    template struct BrickTree<64,double>;

  } // ::ospray::bt
} // ::ospray
