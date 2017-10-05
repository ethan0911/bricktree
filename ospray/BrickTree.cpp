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
#include "ospray/common/Model.h"
#include "ospray/common/Data.h"
#include "ospray/transferFunction/TransferFunction.h"
// ospcommon
#include "ospcommon/FileName.h"
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
	template<typename T, int N>
	struct BrickTreeForestSampler : public ScalarVolumeSampler
	{
	    BrickTreeForestSampler(BrickTreeVolume *btv) : btv(btv)
		{
		    PING;
		    forest = std::make_shared<bt::BrickTreeForest<N,T>>
			(btv->gridSize,btv->validSize,
			 FileName(btv->fileName).dropExt());
		    exit(0);
		}
      
	    /*! compute sample at given position */
	    virtual float computeSample(const vec3f &pos) const override
		{ PING; return 0; }
      
	    /*! compute gradient at given position */
	    virtual vec3f computeGradient(const vec3f &pos) const override
		{ PING; return vec3f(1,0,0); }

	    std::shared_ptr<bt::BrickTreeForest<N,T>> forest;
	    BrickTreeVolume *btv;
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

	int BrickTreeVolume::setRegion
	(const void *source, const vec3i &index, const vec3i &count)
	{
	    FATAL("'setRegion()' doesn't make sense for BT volumes; They can only be set from existing data");
	}

	/*! callback function called by ispc sampling code to compute a
	  gradient at given sample pos in this (c++-only) module */
	extern "C" float BrickTree_scalar_computeSample
	(ScalarVolumeSampler *cppSampler,
	 const vec3f &samplePos)
	{
	    return cppSampler->computeSample(samplePos);
	}

	/*! callback function called by ispc sampling code to compute a
	  sample in this (c++-only) module */
	extern "C" vec3f BrickTree_scalar_computeGradient
	(ScalarVolumeSampler *cppSampler,
	 const vec3f &samplePos)
	{
	    return cppSampler->computeGradient(samplePos);
	}

	template<typename T, int N>
	ScalarVolumeSampler *BrickTreeVolume::createSamplerTN()
	{
	    return new BrickTreeForestSampler<T,N>(this);
	}
    
	template<typename T>
	ScalarVolumeSampler *BrickTreeVolume::createSamplerT()
	{
	    PING;
	    PRINT(brickSize);
	    if (brickSize == 2)
		return createSamplerTN<T,2>();
	    if (brickSize == 4)
		return createSamplerTN<T,4>();
	    if (brickSize == 8)
		return createSamplerTN<T,8>();
	    if (brickSize == 16)
		return createSamplerTN<T,16>();
	    throw std::runtime_error("BrickTree: unsupported brick size");
	}
    
	ScalarVolumeSampler *BrickTreeVolume::createSampler()
	{
	    PING;
	    PRINT(format);
	    if (format == "float")
		return createSamplerT<float>();
	    throw std::runtime_error("BrickTree: unsupported format '"+format+"'");
	}
    
	//! Allocate storage and populate the volume.
	void BrickTreeVolume::commit()
	{
	    // right now we only support floats ...
	    this->voxelType = OSP_FLOAT;

	    cout << "#osp:bt: BrickTreeVolume::commit()" << endl;
	    Ref<TransferFunction> xf = 
		(TransferFunction*)getParamObject("transferFunction");
	    if (!xf)
		throw std::runtime_error
		    ("#osp:bt: bricktree volume does not have a transfer function");
      
	    gridSize   = getParam3i("gridSize",vec3i(-1));
	    brickSize  = getParam1i("brickSize",-1);
	    blockWidth = getParam1i("blockWidth",-1);
	    fileName   = getParamString("fileName","");
	    format     = getParamString("format","<not specified>");
	    PRINT(gridSize);
	    validFractionOfRootGrid = vec3f(validSize) / vec3f(gridSize*blockWidth);

	    sampler = createSampler();
      
	    ispc::BrickTreeVolume_set(getIE(),
				      xf->getIE(),
				      (ispc::vec3i &)validSize,
				      this,
				      sampler);
	}

	OSP_REGISTER_VOLUME(BrickTreeVolume,BrickTreeVolume);

    }
} // ::ospray

