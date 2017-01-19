// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
#include "common/xml/XML.h"

namespace ospray {
  namespace bt {

    using std::cout;
    using std::endl;
    using std::flush;

    template<>
    const char *typeToString<uint8_t>() { return "uint8"; };
    template<>
    const char *typeToString<float>() { return "float"; };
    template<>
    const char *typeToString<double>() { return "double"; };



    template<int N, typename T>
    void BrickTree<N,T>::ValueBrick::clear()
    {
      array3D::for_each(vec3i(N),[&](const vec3i &idx) {
          value[idx.z][idx.y][idx.x] = 0.f;
        });
    }

    template<int N, typename T>
    void BrickTree<N,T>::IndexBrick::clear()
    {
      array3D::for_each(vec3i(N),[&](const vec3i &idx) {
          childID[idx.z][idx.y][idx.x] = invalidID();
        });
    }


    /*! map this one from a binary dump that was created by the bricktreebuilder/raw2bricks tool */
    template<int N, typename T>
    void BrickTree<N,T>::map(const FileName &brickFileBase, size_t blockID, const vec3i &treeCoords)
    {
      char blockFileName[10000];
      sprintf(blockFileName,"%s-brick%06i.osp",brickFileBase.str().c_str(),(int)blockID);

      std::shared_ptr<xml::XMLDoc> doc = xml::readXML(blockFileName);
      if (!doc)
        throw std::runtime_error("could not read brick tree .osp file '"+std::string(blockFileName)+"'");

      
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
            vec3i cellEnd   = min(cellBegin + vec3i(cellSize),maxSize);

            float weight = (cellEnd-cellBegin).product();
            if (weight > 0.f) {
              den += weight;
              num += weight * value[iz][iy][ix];
            }
          }

      den += 1e-8;
      assert(den != 0.);
      return num / den;
    }

    template const char *typeToString<uint8_t>();
    template const char *typeToString<float>();
    template const char *typeToString<double>();

    template struct BrickTree<2,uint8_t>;
    template struct BrickTree<4,uint8_t>;
    template struct BrickTree<8,uint8_t>;
    template struct BrickTree<16,uint8_t>;
    template struct BrickTree<32,uint8_t>;
    template struct BrickTree<64,uint8_t>;

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
