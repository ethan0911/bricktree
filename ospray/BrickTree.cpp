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

#include "BrickTree.h"
// ospray
#include "ospray/common/Model.h"
#include "ospray/common/Data.h"
#include "ospray/transferFunction/TransferFunction.h"
// ispc exports
#include "BrickTree_ispc.h"
// stl
#include <set>
#include <map>
#include <stack>

namespace ospray {
  namespace bt {
    using std::endl;
    using std::cout;
    using std::ostream;
    using std::flush;

    /*! the actual sampler code for a bricktree; to be specialized for
      bricksize, voxel type, etcpp */
    struct BrickTreeSampler : public ScalarVolumeSampler
    {
      /*! compute sample at given position */
      virtual float computeSample(const vec3f &pos) const override
      { PING; return 0; }
      
      /*! compute gradient at given position */
      virtual vec3f computeGradient(const vec3f &pos) const override
      { PING; return vec3f(1,0,0); }
    };
    
    BrickTreeVolume::BrickTreeVolume()
      : Volume(),
        gridSize(-1),
        validSize(-1),
        brickSize(-1),
        fileName("<none>"),
        sampler(NULL)
    {
      ispcEquivalent = ispc::BrickTreeVolume_create(this);
    }

    int BrickTreeVolume::setRegion(const void *source, const vec3i &index, const vec3i &count)
    {
      FATAL("'setRegion()' doesn't make sense for BT volumes; they can only be set from existing data");
    }

    /*! callback function called by ispc sampling code to compute a
        gradient at given sample pos in this (c++-only) module */
    extern "C" float BrickTree_scalar_computeSample(void *_cppSampler,
                                                    const vec3f &samplePos)
    {
      BrickTreeSampler *cppSampler = (BrickTreeSampler *)_cppSampler;
      return cppSampler->computeSample(samplePos);
    }

    /*! callback function called by ispc sampling code to compute a
      sample in this (c++-only) module */
    extern "C" vec3f BrickTree_scalar_computeGradient(void *_cppSampler,
                                                      const vec3f &samplePos)
    {
      BrickTreeSampler *cppSampler = (BrickTreeSampler *)_cppSampler;
      return cppSampler->computeGradient(samplePos);
    }
    
    //! Allocate storage and populate the volume.
    void BrickTreeVolume::commit()
    {
      // right now we only support floats ...
      this->voxelType = OSP_FLOAT;

      cout << "#osp:bt: BrickTreeVolume::commit()" << endl;
      Ref<TransferFunction> xf = (TransferFunction*)getParamObject("transferFunction");
      if (!xf)
        throw std::runtime_error("#osp:bt: bricktree volume does not have a transfer function");
      
      brickSize  = getParam1i("brickSize",-1);
      blockWidth = getParam1i("blockWidth",-1);
      fileName   = getParamString("fileName","");
      validFractionOfRootGrid = vec3f(validSize) / vec3f(gridSize*blockWidth);

      sampler = new BrickTreeSampler;
      
      ispc::BrickTreeVolume_set(getIE(),
                                xf->getIE(),
                                (ispc::vec3i &)validSize,
                                this,
                                sampler);
    }

    OSP_REGISTER_VOLUME(BrickTreeVolume,BrickTreeVolume);

  }
} // ::ospray

