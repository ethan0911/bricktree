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
#include "BrickTree.h"
// this - actual bricktree data structure
#include "../bt/BrickTree.h"
// ospray
#include "ospray/common/Data.h"
#include "ospray/common/Model.h"
#include "ospray/transferFunction/TransferFunction.h"
// ospcommon
#include "ospcommon/FileName.h"
// ispc exports
#include "BrickTree_ispc.h"
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

    static bool flag = true;



    BrickTreeVolume::BrickTreeVolume()
        : Volume(),
          sampler(NULL),
          gridSize(-1),
          validSize(-1),
          brickSize(-1),
          fileName("<none>")
    {
      ispcEquivalent = ispc::BrickTreeVolume_create(this);
    }

    int BrickTreeVolume::setRegion(const void *source,
                                   const vec3i &index,
                                   const vec3i &count)
    {
      FATAL(
          "'setRegion()' doesn't make sense for BT volumes; They can only be "
          "set from existing data");
    }

    size_t BrickTreeVolume::getBlockID(const vec3f &pos)
    {
      // vec3i blockIdx = (pos * validSize) / blockWidth;
      // return blockIdx.x + blockIdx.y * gridSize.x +
      //        blockIdx.z * gridSize.y * gridSize.x;

      // PRINT(blockWidth);
      size_t xDim = pos.x / blockWidth;  //(pos.x <= blockWidth) ? 0 : 1;
      size_t yDim = pos.y / blockWidth;  //(pos.y <= blockWidth) ? 0 : 1;
      size_t zDim = pos.z / blockWidth;  //(pos.z <= blockWidth) ? 0 : 1;

      xDim = (xDim >= gridSize.x) ? gridSize.x - 1 : xDim;
      yDim = (yDim >= gridSize.y) ? gridSize.y - 1 : yDim;
      zDim = (zDim >= gridSize.z) ? gridSize.z - 1 : zDim;
      // std::cout<<xDim <<" "<<yDim<<" "<<zDim<<std::endl;
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
      //PING;
      return new BrickTreeForestSampler<T, N>(this);
    }

    template <typename T>
    ScalarVolumeSampler *BrickTreeVolume::createSamplerT()
    {
      //PING;
      //PRINT(brickSize);
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
      //PING;
      //PRINT(format);
      if (format == "float")
        return createSamplerT<float>();
      throw std::runtime_error("BrickTree: unsupported format '" + format +"'");
    }

    //! Allocate storage and populate the volume.
    void BrickTreeVolume::commit()
    {
      //Volume::updateEditableParameters();
      // right now we only support floats ...
      this->voxelType = OSP_FLOAT;

      cout << "#osp:bt: BrickTreeVolume::commit()" << endl;
      Ref<TransferFunction> xf =
          (TransferFunction *)getParamObject("transferFunction");
      if (!xf)
        throw std::runtime_error(
            "#osp:bt: bricktree volume does not have a transfer function");

      gridSize   = getParam3i("gridSize", vec3i(-1));
      brickSize  = getParam1i("brickSize", -1);
      blockWidth = getParam1i("blockWidth", -1);
      fileName   = getParamString("fileName", "");
      format     = getParamString("format", "<not specified>");
      validSize  = getParam3i("validSize",vec3i(-1));
      PRINT(gridSize);
      validFractionOfRootGrid = vec3f(validSize) / vec3f(gridSize * blockWidth);

      sampler = createSampler();

      // vec3i halfSize = validSize * 0.5f;

      ispc::BrickTreeVolume_set(
          getIE(), xf->getIE(), (ispc::vec3i &)validSize, this, sampler);

      if(!finished)
      {
        finish();
        finished=true;
      }

    }

    OSP_REGISTER_VOLUME(BrickTreeVolume, BrickTreeVolume);

  }  // namespace bt
}  // namespace ospray
