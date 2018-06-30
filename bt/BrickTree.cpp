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
#include "ospcommon/xml/XML.h"
#include "BrickTreeBuilder.h"

// stdlib, for mmap
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
#  include <windows.h>
#else
#  include <sys/mman.h>
#endif
#include <fcntl.h>
#include <string>
#include <cstring>

// O_LARGEFILE is a GNU extension.
#ifdef __APPLE__
#define  O_LARGEFILE  0
#endif

namespace ospray {
  namespace bt {

    using std::cout;
    using std::endl;
    using std::flush;

    template <>
    const char *typeToString<uint8_t>()
    {
      return "uint8";
    };
    template <>
    const char *typeToString<float>()
    {
      return "float";
    };
    template <>
    const char *typeToString<double>()
    {
      return "double";
    };

    template <int N, typename T>
    BrickTree<N, T>::BrickTree()
    {
    }

    template <int N, typename T>
    BrickTree<N, T>::~BrickTree()
    {
      if (!valueBrick)
        free(valueBrick);
      if (!indexBrick)
        free(indexBrick);
      if (!brickInfo)
        free(brickInfo);
      if (!valueBricksStatus)
        //delete valueBricksStatus;
        free(valueBricksStatus);
    }

    template <int N, typename T>
    void BrickTree<N, T>::ValueBrick::clear()
    {
      array3D::for_each(vec3i(N), [&](const vec3i &idx) {
        value[idx.z][idx.y][idx.x] = 0.f;
      });
    }

    template <int N, typename T>
    void BrickTree<N, T>::IndexBrick::clear()
    {
      array3D::for_each(vec3i(N), [&](const vec3i &idx) {
        childID[idx.z][idx.y][idx.x] = invalidID();
      });
    }

    /*! map this one from a binary dump that was created by the
     * bricktreebuilder/raw2bricks tool */
    template <int N, typename T>
    void BrickTree<N, T>::mapOSP(const FileName &brickFileBase,
                                 int blockID,
                                 vec3i treeCoord)
    {
      char blockFileName[10000];
      sprintf(blockFileName,"%s-brick%06i.osp",
              brickFileBase.str().c_str(),(int)blockID);

      std::shared_ptr<xml::XMLDoc> doc = xml::readXML(blockFileName);
      if (!doc)
        throw std::runtime_error("could not read brick tree .osp file '" +
                                 std::string(blockFileName) + "'");
      std::shared_ptr<xml::Node> osprayNode =
        std::make_shared<xml::Node>(doc->child[0]);
      assert(osprayNode->name == "ospray");

      std::shared_ptr<xml::Node> brickTreeNode =
        std::make_shared<xml::Node>(osprayNode->child[0]);
      assert(brickTreeNode->name == "BrickTree");

      avgValue = std::stof(brickTreeNode->getProp("averageValue"));
      sscanf(brickTreeNode->getProp("valueRange").c_str(),
             "%f %f",&valueRange.x,&valueRange.y);
      nBrickSize = std::stoi(brickTreeNode->getProp("brickSize"));
      sscanf(brickTreeNode->getProp("validSize").c_str(),
             "%i %i %i",&validSize.x,&validSize.y,&validSize.z);

      std::shared_ptr<xml::Node> indexBricksNode =
        std::make_shared<xml::Node>(brickTreeNode->child[0]);
      std::shared_ptr<xml::Node> valueBricksNode =
        std::make_shared<xml::Node>(brickTreeNode->child[1]);
      std::shared_ptr<xml::Node> indexBrickOfNode =
        std::make_shared<xml::Node>(brickTreeNode->child[2]);

      numIndexBricks = std::stoll(indexBricksNode->getProp("num"));
      indexBricksOfs = std::stoll(indexBricksNode->getProp("ofs"));

      numValueBricks = std::stoll(valueBricksNode->getProp("num"));
      valueBricksOfs = std::stoll(valueBricksNode->getProp("ofs"));

      numBrickInfos   = std::stoll(indexBrickOfNode->getProp("num"));
      indexBrickOfOfs = std::stoll(indexBrickOfNode->getProp("ofs"));

      valueBrick = (ValueBrick *)malloc(sizeof(ValueBrick) * numValueBricks);
      indexBrick = (IndexBrick *)malloc(sizeof(IndexBrick) * numIndexBricks);
      brickInfo  = (BrickInfo *)malloc(sizeof(BrickInfo) * numBrickInfos);

      // ValueBrick *vb;
      // for (int i = 0; i < numValueBricks; i++) {
      //   vb = (typename BrickTree<N, T>::ValueBrick *)(valueBrick + i);
      //   for (int x = 0; x < N; ++x)
      //     for (int y = 0; y < N; ++y)
      //       for (int z = 0; z < N; ++z)
      //         vb->value[z][y][x] = 0.2f * blockID + 0.1f;
      // }

      //valueBricksStatus = new BrickStatus[numValueBricks]();
      valueBricksStatus = 
        (BrickStatus*)malloc(sizeof(BrickStatus)*numValueBricks);

      sprintf(blockFileName,
              "%s-brick%06i.ospbin",
              brickFileBase.str().c_str(),
              (int)blockID);

      FILE *file = fopen(blockFileName, "rb");
      if (!file)
        throw std::runtime_error("could not open brick bin file " +
                                 std::string(blockFileName));
      fseek(file, indexBricksOfs, SEEK_SET);
      fread(indexBrick, sizeof(IndexBrick), numIndexBricks, file);
      fseek(file, indexBrickOfOfs, SEEK_SET);
      fread(brickInfo, sizeof(BrickInfo), numBrickInfos, file);

      fclose(file);
    }

    template <int N, typename T>
    void BrickTree<N, T>::mapOspBin(const FileName &brickFileBase,
                                    size_t blockID)
    {
      // mmap the binary file
      char blockFileName[10000];
      sprintf(blockFileName,
              "%s-brick%06i.ospbin",
              brickFileBase.str().c_str(),
              (int)blockID);

      FILE *file = fopen(blockFileName, "rb");
      if (!file)
        throw std::runtime_error("could not open brick bin file " +
                                 std::string(blockFileName));

      fseek(file, indexBricksOfs, SEEK_SET);

      fread(indexBrick, sizeof(IndexBrick), numIndexBricks, file);
      fseek(file, valueBricksOfs, SEEK_SET);

      fread(valueBrick, sizeof(ValueBrick), numValueBricks, file);
      fseek(file, indexBrickOfOfs, SEEK_SET); 

      fread(brickInfo, sizeof(BrickInfo), numBrickInfos, file);
      fclose(file);
    }

    template <int N, typename T>
    void BrickTree<N, T>::loadTreeByBrick(const FileName &brickFileBase,
                                          size_t blockID,
                                          std::vector<vec2i> vbReqList)
    {
      char blockFileName[10000];
      sprintf(blockFileName,"%s-brick%06i.ospbin",
              brickFileBase.str().c_str(),(int)blockID);

      FILE *file = fopen(blockFileName, "rb");
      if (!file)
        throw std::runtime_error("could not open brick bin file " +
                                 std::string(blockFileName));

      PING;
      std::vector<vec2i>::iterator it = vbReqList.begin();
      for (; it != vbReqList.end(); it++) {
        loadBricks(file, *it);
      }
      fclose(file);
    }

    template <int N, typename T>
    void BrickTree<N, T>::loadTreeByBrick(const FileName &brickFileBase,
                                          size_t treeID)
    {
      char blockFileName[10000];
      sprintf(blockFileName,"%s-brick%06i.ospbin",
              brickFileBase.str().c_str(),(int)treeID);
      FILE *file = fopen(blockFileName, "rb");
      if (!file)
        throw std::runtime_error("could not open brick bin file " +
                                 std::string(blockFileName));
      for (size_t i = 0; i < numValueBricks; i++) {
        //LoadBricks aBrick(VALUEBRICK, i);
        //loadBricks(file,aBrick);         
        if (!valueBricksStatus[i].isLoaded &&
            valueBricksStatus[i].isRequested) {
          LoadBricks aBrick(VALUEBRICK, i);
          loadBricks(file, aBrick);
        }
      }
      fclose(file);
    }

    template <int N, typename T>
    void BrickTree<N, T>::loadBricks(FILE *file, vec2i vbListInfo)
    {
      fseek(file, valueBricksOfs + vbListInfo.x * sizeof(ValueBrick), 
            SEEK_SET);
      fread((ValueBrick *)(valueBrick + vbListInfo.x),
            sizeof(ValueBrick),vbListInfo.y,file);
      for (int i = 0; i < vbListInfo.y; i++)
        valueBricksStatus[vbListInfo.x + i].isLoaded = true;
    }

    template <int N, typename T>
    void BrickTree<N, T>::loadBricks(FILE *file, LoadBricks aBrick)
    {
      fseek(file, valueBricksOfs + aBrick.brickID * sizeof(ValueBrick), 
            SEEK_SET);
      fread((ValueBrick *)(valueBrick + aBrick.brickID),
            sizeof(ValueBrick),1,file);
      valueBricksStatus[aBrick.brickID].isLoaded = true;
    }

    template <int N, typename T>
    const T BrickTree<N, T>::findBrickValue(const int blockID, 
                                            const size_t cBrickID,
                                            const vec3i cPos,
                                            const size_t pBrickID,
                                            const vec3i pPos)
    {
      ValueBrick *vb = NULL;

#if STREAM_DATA
      if (!valueBricksStatus[cBrickID].isRequested) {
        // request this brick if it is not requested
        valueBricksStatus[cBrickID].isRequested = 1;
      } else if(!valueBricksStatus[cBrickID].isLoaded) {
        // return average value if this brick is requested but not loaded
        if (cBrickID == 0) { // root node, return average value of the tree
          return this->avgValue;
        } else {
          // inner node, return the average value of this node which is 
          // stored in the parent node
          if (!valueBricksStatus[pBrickID].isLoaded)
            return this->avgValue;
          else {
            vb = (typename BrickTree<N, T>::ValueBrick *)
              (valueBrick + pBrickID);
            return vb->value[pPos.z][pPos.y][pPos.x];
          }
        }
      } else {
        vb = (typename BrickTree<N, T>::ValueBrick *)(valueBrick + cBrickID);
        return vb->value[cPos.z][cPos.y][cPos.x];
      }

      // if (!valueBricksStatus[cBrickID].isLoaded) {
      //   // request this brick if it is not requested
      //   if (!valueBricksStatus[cBrickID].isRequested) {
      //     valueBricksStatus[cBrickID].isRequested = 1;
      //   }
      //   // return average value if this brick is requested but not loaded
      //   if (cBrickID == 0)  // root node, return average value of the tree
      //     return this->avgValue;
      //   else {
      //     // inner node, return the average value of this node which is 
      //     // stored
      //     // in the parent node
      //     if (!valueBricksStatus[pBrickID].isLoaded)
      //       return this->avgValue;
      //     else {
      //       vb = (typename BrickTree<N, T>::ValueBrick *)(valueBrick +
      //                                                     pBrickID);
      //       return vb->value[pPos.z][pPos.y][pPos.x];
      //     }
      //   }
      // } else {
      //   vb = (typename BrickTree<N, T>::ValueBrick *)
      //     (valueBrick + cBrickID);      
      //   // blockID 3 cBrickID 265097 cPos 2 0 1 value 2.39878
      //   // if (vb->value[cPos.z][cPos.y][cPos.x] > 1.f)
      //   // if (blockID == 3 && cBrickID == 265097 && 
      //   //     cPos.x == 2 && cPos.y == 0 && cPos.z == 1)
      //   // std::cout << "blockID " << blockID << " "
      //   //           << "cBrickID " << cBrickID << " "
      //   //           << "cPos " 
      //   //           << cPos.x << " " 
      //   //           << cPos.y << " " 
      //   //           << cPos.z << " "
      //   //           << "value " << vb->value[cPos.z][cPos.y][cPos.x]
      //   //           << std::endl;        
      //   return vb->value[cPos.z][cPos.y][cPos.x];
      // }

      return this->avgValue;

#else

      vb = (typename BrickTree<N, T>::ValueBrick *)(valueBrick + brickID);
      return vb->value[cellPos.z][cellPos.y][cellPos.x];

#endif
    }

    template <int N, typename T>
    const T BrickTree<N, T>::findValue(const int blockID, const vec3i &coord,
                                       int blockWidth)
    {
      // return 0.2f;
      // start with the root brick
      int brickSize = blockWidth;
      assert(reduce_max(coord) < brickSize);
      int32_t brickID       = 0;
      int32_t parentBrickID = BrickTree<N, T>::invalidID();
      vec3i cpos(0);
      vec3i parentCellPos(0);
      while (brickSize > N) {
        int cellSize = brickSize / N;

        cpos.x = (coord.x % brickSize) / cellSize;
        cpos.y = (coord.y % brickSize) / cellSize;
        cpos.z = (coord.z % brickSize) / cellSize;

        parentCellPos.x = (coord.x % (brickSize * N)) / brickSize;
        parentCellPos.y = (coord.y % (brickSize * N)) / brickSize;
        parentCellPos.z = (coord.z % (brickSize * N)) / brickSize;

        int32_t ibID = brickInfo[brickID].indexBrickID;
        if (ibID == BrickTree<N, T>::invalidID()) {
          return findBrickValue(blockID, brickID, cpos, 
                                parentBrickID, parentCellPos);
        } else {
          IndexBrick ib        = indexBrick[ibID];
          int32_t childBrickID = ib.childID[cpos.z][cpos.y][cpos.x];
          if (childBrickID == BrickTree<N, T>::invalidID()) {
            return findBrickValue(blockID, brickID, cpos,
                                  parentBrickID, parentCellPos);
          } else {
            parentBrickID = brickID;
            brickID       = childBrickID;
          }
        }
        brickSize = cellSize;
      }

      cpos.x = coord.x % brickSize;
      cpos.y = coord.y % brickSize;
      cpos.z = coord.z % brickSize;
      return findBrickValue(blockID, brickID, cpos, parentBrickID, parentCellPos);
    }

    template <int N, typename T>
    double BrickTree<N, T>::ValueBrick::computeWeightedAverage
    (// coordinates of lower-left-front
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

      for (int iz = 0; iz < N; iz++)
        for (int iy = 0; iy < N; iy++)
          for (int ix = 0; ix < N; ix++) {
            vec3i cellBegin = min(brickCoord * vec3i(brickSize) + 
                                  vec3i(ix, iy, iz) * vec3i(cellSize),
                                  maxSize);
            vec3i cellEnd   = min(cellBegin + vec3i(cellSize), maxSize);

            float weight = (cellEnd - cellBegin).product();
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

    template struct BrickTree<2, uint8_t>;
    template struct BrickTree<4, uint8_t>;
    template struct BrickTree<8, uint8_t>;
    template struct BrickTree<16, uint8_t>;
    template struct BrickTree<32, uint8_t>;
    template struct BrickTree<64, uint8_t>;

    template struct BrickTree<2, float>;
    template struct BrickTree<4, float>;
    template struct BrickTree<8, float>;
    template struct BrickTree<16, float>;
    template struct BrickTree<32, float>;
    template struct BrickTree<64, float>;

    template struct BrickTree<2, double>;
    template struct BrickTree<4, double>;
    template struct BrickTree<8, double>;
    template struct BrickTree<16, double>;
    template struct BrickTree<32, double>;
    template struct BrickTree<64, double>;

  }  // namespace bt
}  // namespace ospray
