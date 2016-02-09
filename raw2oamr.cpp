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

#undef NDEBUG

// ospray
#include "AMR.h"

namespace ospray {
  namespace amr {

    using std::endl;
    using std::cout;

    template<typename T>
    struct OctreeBuilder {
      // constructor
      OctreeBuilder();

      // build one root block (the one with given ID, obviously)
      void makeBlock(const vec3i &blockID);
      typename Octree<T>::Cell recursiveBuild(const vec3i &begin, int blockSize);

      size_t getCellID(const vec3i &cellID) const
      {
        return (size_t)cellID.x + input->dims.x*((size_t)cellID.y + input->dims.y * (size_t)cellID.z);
      }
      // build the entire thing
      void makeAMR();

      // pre-loaded input volume
      Array3D<T> *input;

      // max number of levels we are allowed to create
      int maxLevels;

      // threshold for refining
      float threshold;

      Octree<T> *oct;
    };
    
    // constructor
    template<typename T>
    OctreeBuilder<T>::OctreeBuilder()
      : input(NULL), maxLevels(2), threshold(.1f)
    {
      oct = new Octree<T>;
    }

    template<typename T>
    typename Octree<T>::Cell OctreeBuilder<T>::recursiveBuild(const vec3i &begin, int blockSize)
    {
      Range<T> range = input->getValueRange(begin,begin+vec3i(blockSize));
      // oct->rootCell[getCellID(blockID)].ccValue = range.center();
      typename Octree<T>::Cell  cell;
      cell.ccValue = range.center();
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
        cell.childID = -1;
      }
      return cell;
    }

    // build octree for block i,j,k
    template<typename T>
    void OctreeBuilder<T>::makeBlock(const vec3i &blockID)
    {
      size_t blockSize = (1<<maxLevels);
      vec3i begin = blockID*vec3i(blockSize);

      oct->rootCell[getCellID(blockID)] = recursiveBuild(begin,blockSize);
    }

    template<typename T>
    void OctreeBuilder<T>::makeAMR()
    {
      size_t blockSize = (1<<maxLevels);
      // size_t trueRootBlockSize = rootBlockSize * (1<<maxLevels);
      vec3i numBlocks = divRoundUp(input->dims,vec3i(blockSize));
      if (numBlocks * vec3i(blockSize) != input->dims)
        cout << "warning: input volume not a multiple of true root block size (ie, root block size after " << maxLevels << " binary refinements), some padding will happen" << endl;
      
      oct->rootCell.resize(numBlocks.x*numBlocks.y*numBlocks.z);

      Range<float> mm = input->getValueRange(vec3i(0),input->dims);
      threshold = threshold * mm.size();
      
      for (int iz=0;iz*blockSize<input->dims.z;iz++)
        for (int iy=0;iy*blockSize<input->dims.y;iy++)
          for (int ix=0;ix*blockSize<input->dims.x;ix++) {
            makeBlock(vec3i(ix,iy,iz));
          }
    }

    void error(const std::string &msg = "")
    {
      if (msg != "") {
        cout << "*********************************" << endl;
        cout << "Error: " << msg << endl;
        cout << "*********************************" << endl << endl;
      }
      cout << "Usage" << endl;
      cout << "  ./ospRaw2Octree <inFile.raw> -dims <nx> <ny> <nz> -o <outfilename.xml> <args>" << endl;
      cout << "with args:" << endl;
      cout << " --depth <maxlevels> : use maxlevels octree refinement levels" << endl;
      cout << " --root-block-size <bs> : use root block size of bs^3" << endl;
      exit(msg != "");
    }

    extern "C" int main(int ac, char **av)
    {
      std::string inFileName = "";
      std::string outFileName = "";
      vec3i dims(0);

      OctreeBuilder<float> builder;
      for (int i=1;i<ac;i++) {
        const std::string arg = av[i];
        if (arg == "-o")
          outFileName = av[++i];
        else if (arg == "--depth" || arg == "-depth" || arg == "--max-levels" || arg == "-ml")
          builder.maxLevels = atoi(av[++i]);
        else if (arg == "--threshold" || arg == "-t")
          builder.threshold = atof(av[++i]);
        else if (arg == "-dims" || arg == "--dimensions") {
          dims.x = atoi(av[++i]);
          dims.y = atoi(av[++i]);
          dims.z = atoi(av[++i]);
        }
        else if (arg[0] != '-')
          inFileName = av[i];
        else
          error("unknown arg '"+arg+"'");
      }
      
      if (dims == vec3i(0))
        error("no input dimensions (-dims) specified");
      if (inFileName == "")
        error("no input file speified");
      
      builder.input = new Array3D<float>(dims);
      loadRAW<float>(*builder.input,inFileName,dims);
      builder.makeAMR();
      
      return 0;
    }

  } // ::ospray::amr
} // ::ospary
