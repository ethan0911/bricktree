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

#pragma once

// ospray
#include "ospcommon/math.h"
#include "ospray/volume/Volume.h"
// bt base
#include "../bt/BrickTree.h"

// hack for lerp3
namespace ospcommon {
template<typename T>
  __forceinline T lerp3(const float x0, const float x1, const float x2,
                        const float x3, const float x4, const float x5,
                        const float x6, const float x7,
                        const T &u, const T &v, const T &i) {
    return (1.0f - i) * lerp2<T>(x0,x1,x2,x3,u,v) + 
           i * lerp2<T>(x4,x5,x6,x7,u,v);
   }
};

// other headers
#include <mutex>
#include <thread>

namespace ospray {
  namespace bt {

    /*! abstract base class for any type of scalar volume sampler
     * we will eventually specialize this for bricktree further below
     */
    struct ScalarVolumeSampler
    {
      /*! compute sample at given position */
      virtual float sample(const vec3f &pos) const = 0;

      /*! compute gradient at given position */
      virtual vec3f computeGradient(const vec3f &pos) const = 0;
    };

    static std::mutex mtx;

    /*! \brief The actual C++ implementation of an ospray bricktree volume
     *  type
     *
     *  note this only maintains all kind of ospray state fo rthis
     *  object; the actual sampling is done in the sampler defined in
     *  the implementation file
     */
    struct BrickTreeVolume : public ospray::Volume
    {
      BrickTreeVolume();

      //! \brief common function to help printf-debugging
      virtual std::string toString() const
      {
        return "ospray::bt::BrickTreeVolume";
      }

      //! Allocate storage and populate the volume.
      virtual void commit();

      //! Copy voxels into the volume at the given index
      //  (non-zero return value indicates success).
      virtual int setRegion(const void *source,
                            const vec3i &index,
                            const vec3i &count) override;

      //get the blockID in the bricktree volume
      size_t getBlockID(const vec3f &pos);

      /*! create specialization of sampler for given type and brick size
       */
      template <typename T, int N>
      ScalarVolumeSampler *createSamplerTN();
      /*! create specialization of sampler for given type */
      template <typename T>
      ScalarVolumeSampler *createSamplerT();
      /*! create specialization of sampler given values for brick size
       *  an dtype
       */
      ScalarVolumeSampler *createSampler();
      ScalarVolumeSampler *sampler;

      bool finished = false;

      vec3i gridSize;
      vec3i validSize;
      vec3f validFractionOfRootGrid;
      int depth;
      int brickSize;
      int blockWidth;
      std::string fileName;
      std::string format;
      OSPDataType voxelType;
      // actual volume bounds for distributed parallel rendering
      box3f volBounds;
    };

    /*! the actual sampler code for a bricktree; to be specialized for
      bricksize, voxel type, etcpp */
    template <typename T, int N>
    struct BrickTreeForestSampler : public ScalarVolumeSampler
    {
      BrickTreeForestSampler(BrickTreeVolume *btv) : btv(btv)
      {
        //PING;
        forest = std::make_shared<bt::BrickTreeForest<N, T>>(
            btv->gridSize, btv->validSize, FileName(btv->fileName).dropExt());

        if(forest != NULL){
          btv->volBounds = forest->forestBounds;
        }
        //exit(0);
      }

      /*! compute sample at given position */
      virtual float sample(const vec3f &pos) const override
      {
        //PING;
        vec3f coord = pos; //* btv->validSize;

        vec3i low    = (vec3i)coord;
        vec3i upper  = low + vec3i(1);
        vec3f factor = coord - (vec3f)low;

        float v;
#if 0
        size_t blockId = btv->getBlockID(low);
        //auto bt = forest->tree[blockId];
        auto bt = forest->tree.find(blockId);
        v = bt->second.avgValue;//second.findValue(low, btv->blockWidth);
#else
        if (low.x == btv->validSize.x - 1) {
          float neighborValue[2][2];
          for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
              size_t blockId = btv->getBlockID((vec3f)(low + vec3i(0, j, i)));
              if (blockId >= btv->gridSize.product())
                throw std::runtime_error("Overflow the block tree vector!!");
              //auto bt = forest->tree[blockId];
              auto bt = forest->tree.find(blockId);
              neighborValue[i][j] =
                  bt->second.findValue(low + vec3i(0, j, j), btv->blockWidth);
            }
          }
          v = lerp2<float>(neighborValue[0][0],
                           neighborValue[0][1],
                           neighborValue[1][0],
                           neighborValue[1][1],
                           factor.y,
                           factor.z);
        } else if (low.y == btv->validSize.y - 1) {
          float neighborValue[2][2];
          for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
              size_t blockId = btv->getBlockID((vec3f)(low + vec3i(j, 0, i)));
              if (blockId >= btv->gridSize.product())
                throw std::runtime_error("Overflow the block tree vector!!");
              //auto bt = forest->tree[blockId];
              auto bt = forest->tree.find(blockId);
              neighborValue[i][j] =
                  bt->second.findValue(low + vec3i(j, 0, i), btv->blockWidth);
            }
          }
          v = lerp2<float>(neighborValue[0][0],
                           neighborValue[0][1],
                           neighborValue[1][0],
                           neighborValue[1][1],
                           factor.x,
                           factor.z);
        } else if (low.z == btv->validSize.z - 1) {
          float neighborValue[2][2];
          for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
              size_t blockId = btv->getBlockID((vec3f)(low + vec3i(j, i, 0)));
              if (blockId >= btv->gridSize.product())
                throw std::runtime_error("Overflow the block tree vector!!");
              //auto bt = forest->tree[blockId];
              auto bt = forest->tree.find(blockId);
              neighborValue[i][j] =
                  bt->second.findValue(low + vec3i(j, i, 0), btv->blockWidth);
            }
          }
          v = lerp2<float>(neighborValue[0][0],
                           neighborValue[0][1],
                           neighborValue[1][0],
                           neighborValue[1][1],
                           factor.x,
                           factor.y);
        } else {
          float neighborValue[2][2][2];
          array3D::for_each(vec3i(2), [&](const vec3i &idx) {
            size_t blockId = btv->getBlockID((vec3f)(low + idx));
            if (blockId >= btv->gridSize.product())
              throw std::runtime_error("Overflow the block tree vector!!");
            // auto bt = forest->tree[blockId];
            auto bt = forest->tree.find(blockId);
            if (!bt->second.isLoaded) {
              neighborValue[idx.z][idx.y][idx.x] = bt->second.avgValue;
              auto btRequested                   = forest->treeBinDataRequested;
              // mtx.lock();
              // if (btRequested.find(blockId) != btRequested.end()) {
              //   forest->treeBinDataRequested.insert(blockId);
              // }
              // mtx.unlock();
            } else {
              neighborValue[idx.z][idx.y][idx.x] =
                  bt->second.findValue(low + idx, btv->blockWidth);
            }
          });
          v = lerp3<float>(neighborValue[0][0][0],
                           neighborValue[0][0][1],
                           neighborValue[0][1][0],
                           neighborValue[0][1][1],
                           neighborValue[1][0][0],
                           neighborValue[1][0][1],
                           neighborValue[1][1][0],
                           neighborValue[1][1][1],
                           factor.x,
                           factor.y,
                           factor.z);
        }
#endif

        return v;
      }

      /*! compute gradient at given position */
      virtual vec3f computeGradient(const vec3f &pos) const override
      {
        // PING;
        return vec3f(1, 0, 0);
      }



      std::shared_ptr<bt::BrickTreeForest<N, T>> forest;
      BrickTreeVolume *btv;
    };

  }  // namespace bt
}  // namespace ospray
