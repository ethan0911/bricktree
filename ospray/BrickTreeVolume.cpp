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

// this - ospray plugin
#include "BrickTreeVolume.h"
// this - actual bricktree data structure
#include "../bt/BrickTree.h"
// ospray
#include "ospray/common/Data.h"
#include "ospray/common/Model.h"
#include "ospray/transferFunction/TransferFunction.h"
// ospcommon
#include "ospcommon/FileName.h"
// ispc exports
#include "BrickTreeVolume_ispc.h"
// stl
#include <map>
#include <set>
#include <stack>

namespace ospray {
  namespace bt {

    using std::cout;
    using std::endl;
    using std::flush;
    using std::ostream;
    BrickTreeVolume::BrickTreeVolume()
      : Volume(),
        sampler(NULL),
        gridSize(-1),
        validSize(-1),
        brickSize(-1),
        fileName("<none>")
    {}

    int BrickTreeVolume::setRegion(const void *source,
                                   const vec3i &index,
                                   const vec3i &count)
    {
      FATAL("'setRegion()' doesn't make sense for BT volumes; "
            "They can only be set from existing data");
    }

    int BrickTreeVolume::getBlockID(const vec3f &pos)
    {
      // vec3i blockIdx = (pos * validSize) / blockWidth;
      // return blockIdx.x + blockIdx.y * gridSize.x +
      //        blockIdx.z * gridSize.y * gridSize.x;
      int xDim = pos.x / blockWidth;  //(pos.x <= blockWidth) ? 0 : 1;
      int yDim = pos.y / blockWidth;  //(pos.y <= blockWidth) ? 0 : 1;
      int zDim = pos.z / blockWidth;  //(pos.z <= blockWidth) ? 0 : 1;
      xDim = (xDim >= gridSize.x) ? gridSize.x - 1 : xDim;
      yDim = (yDim >= gridSize.y) ? gridSize.y - 1 : yDim;
      zDim = (zDim >= gridSize.z) ? gridSize.z - 1 : zDim;
      return xDim + yDim * gridSize.x + zDim * gridSize.y * gridSize.x;
    }

    /*! callback function called by ispc sampling code to compute a
      gradient at given sample pos in this (c++-only) module */
    extern "C" float BrickTree_scalar_sample(ScalarVolumeSampler *cppSampler,
                                             const vec3f &samplePos)
    {
      return cppSampler->sample(samplePos);
    }

    /*! callback function called by ispc sampling code to compute a
      sample in this (c++-only) module */
    extern "C" vec3f BrickTree_scalar_computeGradient(
        ScalarVolumeSampler *cppSampler, const vec3f &samplePos)
    {
      return cppSampler->computeGradient(samplePos);
    }

    template <typename T, int N>
    ScalarVolumeSampler *BrickTreeVolume::createSamplerTN()
    {
      return new BrickTreeForestSampler<T, N>(this);
    }

    template <typename T>
    ScalarVolumeSampler *BrickTreeVolume::createSamplerT()
    {
      if (brickSize == 2)
        return createSamplerTN<T, 2>();
      if (brickSize == 4)
        return createSamplerTN<T, 4>();
      if (brickSize == 8)
        return createSamplerTN<T, 8>();
      if (brickSize == 16)
        return createSamplerTN<T, 16>();
      throw std::runtime_error("BrickTree: unsupported brick size");
    }

    ScalarVolumeSampler *BrickTreeVolume::createSampler()
    {
      if (format == "float")
        return createSamplerT<float>();
      throw std::runtime_error("BrickTree: unsupported format '" + 
                               format +"'");
    }

    //! Allocate storage and populate the volume.
    void BrickTreeVolume::commit()
    {
      // create IE
      if (ispcEquivalent == nullptr)
        ispcEquivalent = ispc::BrickTreeVolume_create(this);
      // update variables
      Volume::updateEditableParameters();
      this->voxelType  = OSP_FLOAT; // TODO right now we only support floats.
      this->gridSize   = getParam3i("gridSize", vec3i(-1));
      this->brickSize  = getParam1i("brickSize", -1);
      this->blockWidth = getParam1i("blockWidth", -1);
      this->fileName   = getParamString("fileName", "");
      this->format     = getParamString("format", "<not specified>");
      this->validSize  = getParam3i("validSize",vec3i(-1));
      this->validFractionOfRootGrid = 
        vec3f(validSize) / vec3f(gridSize*blockWidth);
      this->sampler = createSampler();
      ispc::BrickTreeVolume_set(getIE(), 
                                (ispc::vec3i &)validSize,
                                (ispc::vec3i &)gridSize,
                                brickSize,
                                blockWidth,
                                this, sampler);

      std::cout << "[cpp]  size of sizeof(BrickTree<4,float>) " << sizeof(BrickTree<4,float>) << std::endl;
      auto& forest = dynamic_cast<BrickTreeForestSampler<float,4>*>(sampler)->forest->tree;
      ispc::BrickTreeVolume_set_BricktreeForest(getIE(),
                                                forest.data(),
                                                forest.size());
      for (int i = 0; i < forest.size(); ++i) {
        std::cout << "cpp " << forest[i].avgValue << std::endl;
      }
      
      if(!finished)
      {
        finish();
        finished=true;
      }
    }

    OSP_REGISTER_VOLUME(BrickTreeVolume, BrickTreeVolume);

  }  // namespace bt
}  // namespace ospray
