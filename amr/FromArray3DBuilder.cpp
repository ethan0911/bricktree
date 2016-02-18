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
#include "FromArray3DBuilder.h"

namespace ospray {
  namespace amr {
    
    using std::cout;
    using std::endl;
    
    // constructor
    template<typename T>
    FromArray3DBuilder<T>::FromArray3DBuilder()
      : input(NULL), maxLevels(2), threshold(.1f)
    {
      oct = new AMR<T>;
    }
    
    template<typename T>
    size_t FromArray3DBuilder<T>::inputCellID(const vec3i &cellID) const
    {
      vec3i dims = input->size();
      return (size_t)cellID.x + dims.x*((size_t)cellID.y + dims.y * (size_t)cellID.z);
    }
    
    template<typename T>
    size_t FromArray3DBuilder<T>::rootCellID(const vec3i &cellID) const
    {
      return (size_t)cellID.x + oct->dimensions.x*((size_t)cellID.y + oct->dimensions.y * (size_t)cellID.z);
    }
    
    template<typename T>
    typename Octree<T>::Cell 
    FromArray3DBuilder<T>::recursiveBuild(const vec3i &begin, int blockSize)
    {
      vec3i end = begin+vec3i(blockSize);
      Range<T> range = input->getValueRange(begin,end);
      // oct->rootCell[getCellID(blockID)].ccValue = range.center();
      typename Octree<T>::Cell  cell;
      if (blockSize > 1 && range.size() > threshold) {
        cell.childID = oct->octCell.size();
        typename Octree<T>::OctCell oc;
        oct->octCell.push_back(oc);
        int halfSize = blockSize/2;
        for (int iz=0;iz<2;iz++)
          for (int iy=0;iy<2;iy++)
            for (int ix=0;ix<2;ix++)
              oc.child[iz][iy][ix] = recursiveBuild(begin+vec3i(ix,iy,iz)*halfSize,halfSize);
        oct->octCell[cell.childID] = oc;
      } else {
        cell.ccValue = range.center();
        cell.childID = -1;
      }
      return cell;
    }
    
    // build octree for block i,j,k (in root grid coordinates)
    template<typename T>
    void FromArray3DBuilder<T>::makeBlock(const vec3i &blockID)
    {
      size_t blockSize = (1<<maxLevels);
      vec3i begin = blockID*vec3i(blockSize);

      oct->rootCell[rootCellID(blockID)] = recursiveBuild(begin,blockSize);
    }

    template<typename T>
    AMR<T> *FromArray3DBuilder<T>::makeAMR(const Array3D<T> *input, 
                                          int maxLevels, 
                                          float threshold)
    {
      this->input = input;
      this->maxLevels = maxLevels;
      this->threshold = threshold;

      size_t blockSize = (1<<maxLevels);
      std::cout << "Building AMR with " << maxLevels << " levels."
                << " this corresponds to blocks of " << blockSize
                << " input cells" << std::endl;
      vec3i dims = input->size();
      vec3i numBlocks = divRoundUp(dims,vec3i(blockSize));
      std::cout << "root grid size at this block size is " << numBlocks << std::endl;
      
      if (numBlocks * vec3i(blockSize) != dims)
        cout << "warning: input volume not a multiple of true root block size (ie, root block size after " << maxLevels << " binary refinements), some padding will happen" << endl;
      
      oct->allocate(numBlocks);//(numBlocks.x*numBlocks.y*numBlocks.z);

      cout << "computing value range for entire volume..." << endl;
      Range<T> mm = input->getValueRange(vec3i(0),dims);
      std::cout << "value range for entire input is " << mm << std::endl;
      threshold = threshold * mm.size();
      std::cout << "at specified rel threshold of " << threshold << " this triggers splitting nodes whose difference exceeds " << threshold << std::endl;
      
      size_t numInputCellsDone = 0;
      size_t oldSize = 0;
      for (int iz=0;iz<numBlocks.z;iz++)
        for (int iy=0;iy<numBlocks.y;iy++)
          for (int ix=0;ix<numBlocks.x;ix++) {
            numInputCellsDone += blockSize*blockSize*blockSize;
            makeBlock(vec3i(ix,iy,iz));
            size_t num = oct->octCell.size() - oldSize;
            size_t totalCellsCreatedSoFar 
              = oct->rootCell.size()
              + 8 * oct->octCell.size();
            float pct = totalCellsCreatedSoFar * 100.f / numInputCellsDone;
            cout << "oct[" << vec3f(ix,iy,iz) << "]: " << (8*num) << " entries"
                 << "\t(" << oct->octCell.size() << " total, "
                 << "out of " << numInputCellsDone
                 << "; that is " << pct << "% of cells"
                 << ")" << endl;
            oldSize = oct->octCell.size();
          }
      return oct;
    }

    template struct FromArray3DBuilder<float>;
    template struct FromArray3DBuilder<uint8>;
  } // ::ospray::amr
} // ::ospray

