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
        free(valueBricksStatus);
      if(!vbIdxByLevelStride)
        free(vbIdxByLevelStride);
      
      for(int i = 0 ; i < depth ; i++){
        if(!vbIdxByLevelBuffers[i])
          free(vbIdxByLevelBuffers[i]);
      }

      if(!vbIdxByLevelBuffers)
        free(vbIdxByLevelBuffers);
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
      std::shared_ptr<xml::Node> osprayNode = std::make_shared<xml::Node>(doc->child[0]);
      assert(osprayNode->name == "ospray");

      std::shared_ptr<xml::Node> brickTreeNode = std::make_shared<xml::Node>(osprayNode->child[0]);
      assert(brickTreeNode->name == "BrickTree");

      avgValue = std::stof(brickTreeNode->getProp("averageValue"));
      sscanf(brickTreeNode->getProp("valueRange").c_str(),"%f %f",&valueRange.x,&valueRange.y);
      nBrickSize = std::stoi(brickTreeNode->getProp("brickSize"));
      sscanf(brickTreeNode->getProp("validSize").c_str(),"%i %i %i",&validSize.x,&validSize.y,&validSize.z);

      depth = log(max(validSize.x,validSize.y,validSize.z))/log(nBrickSize);

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

      valueBricksStatus = (BrickStatus*)malloc(sizeof(BrickStatus)*numValueBricks);

      sprintf(blockFileName,"%s-brick%06i.ospbin",brickFileBase.str().c_str(),(int)blockID);

      FILE *file = fopen(blockFileName, "rb");
      if (!file)
        throw std::runtime_error("could not open brick bin file " + std::string(blockFileName));
      fseek(file, indexBricksOfs, SEEK_SET);
      fread(indexBrick, sizeof(IndexBrick), numIndexBricks, file);
      fseek(file, indexBrickOfOfs, SEEK_SET);
      fread(brickInfo, sizeof(BrickInfo), numBrickInfos, file);

      fclose(file);

      //reorganize the valuebrick buffer by bricktree level
      vbIdxByLevelBuffers = (size_t**)malloc(sizeof(size_t*) * depth);
      vbIdxByLevelStride = (size_t*)malloc(sizeof(size_t) * depth);

      for (int i = 0; i < depth; i++) {
        std::vector<size_t> vbsByLevel = getValueBrickIDsByLevel(i);
        vbIdxByLevelStride[i] = vbsByLevel.size();
        *(vbIdxByLevelBuffers + i) = (size_t*)malloc(sizeof(size_t) * vbIdxByLevelStride[i]);
        std::copy(vbsByLevel.begin(),vbsByLevel.end(), *(vbIdxByLevelBuffers + i));
      }

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
                                          std::vector<int> vbReqList)
    {
      char blockFileName[10000];
      sprintf(blockFileName,
              "%s-brick%06i.ospbin",
              brickFileBase.str().c_str(),
              (int)blockID);

      FILE *file = fopen(blockFileName, "rb");
      if (!file)
        throw std::runtime_error("could not open brick bin file " +
                                 std::string(blockFileName));

      std::vector<int>::iterator it = vbReqList.begin();
      for (; it != vbReqList.end(); it++) {
        LoadBricks aBrick(VALUEBRICK, *it);
        loadBricks(file, aBrick);
        // printf("treeID:%d, vbid: %d, vbSize:%ld \n",
        //     blockID,
        //     *it,
        //     numValueBricks);
      }
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

      if(valueBricksStatus[cBrickID].isLoaded){
        // current brick is loaded 
        vb = (typename BrickTree<N, T>::ValueBrick *)(valueBrick + cBrickID);
        return vb->value[cPos.z][cPos.y][cPos.x];
      } else if (valueBricksStatus[cBrickID].isRequested != 0 ){
        // current brick has been requested but not yet loaded
        // return the loaded parent brick or return the average value
        if(valueBricksStatus[pBrickID].isLoaded != 0){
          vb = (typename BrickTree<N, T>::ValueBrick *)(valueBrick + pBrickID);
          return vb->value[pPos.z][pPos.y][pPos.x];
        } else{
          return this->avgValue;
        }
      } else{
        // request current brick if it is not requested
        valueBricksStatus[cBrickID].isRequested = 1;
      }

      return this->avgValue;

#else

      vb = (typename BrickTree<N, T>::ValueBrick *)(valueBrick + cBrickID);
      return vb->value[cPos.z][cPos.y][cPos.x];

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
    double BrickTree<N, T>::ValueBrick::computeWeightedAverage( // coordinates of lower-left-front
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
