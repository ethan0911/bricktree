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
    struct OctAMRBuilder {
      // constructor
      OctAMRBuilder();

      // build one root block (the one with given ID, obviously)
      void makeBlock(const vec3i &blockID);
      typename Octree<T>::Cell recursiveBuild(const vec3i &begin, int blockSize);

      size_t inputCellID(const vec3i &cellID) const
      {
        return (size_t)cellID.x + input->dims.x*((size_t)cellID.y + input->dims.y * (size_t)cellID.z);
      }

      size_t rootCellID(const vec3i &cellID) const
      {
        return (size_t)cellID.x + oct->dimensions.x*((size_t)cellID.y + oct->dimensions.y * (size_t)cellID.z);
      }

      // build the entire thing
      void makeAMR();

      // pre-loaded input volume
      Array3D<T> *input;

      // max number of levels we are allowed to create
      int maxLevels;

      // threshold for refining
      float threshold;

      AMR<T> *oct;
    };
    
    // constructor
    template<typename T>
    OctAMRBuilder<T>::OctAMRBuilder()
      : input(NULL), maxLevels(2), threshold(.1f)
    {
      oct = new AMR<T>;
    }

    template<typename T>
    typename Octree<T>::Cell OctAMRBuilder<T>::recursiveBuild(const vec3i &begin, int blockSize)
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
        // PING;
        // PRINT(begin);
        // PRINT(end);
        // PRINT(blockSize);
        // PRINT(range);
        cell.childID = -1;
      }
      return cell;
    }

    // build octree for block i,j,k (in root grid coordinates)
    template<typename T>
    void OctAMRBuilder<T>::makeBlock(const vec3i &blockID)
    {
      // cout << "-------------------------------------------------------" << endl;
      // cout << "making block " << blockID << endl;
      size_t blockSize = (1<<maxLevels);
      vec3i begin = blockID*vec3i(blockSize);

      oct->rootCell[rootCellID(blockID)] = recursiveBuild(begin,blockSize);
    }

    template<typename T>
    void OctAMRBuilder<T>::makeAMR()
    {
      size_t blockSize = (1<<maxLevels);
      cout << "Building AMR with " << maxLevels << " levels. this corresponds to blocks of " << blockSize << " input cells" << endl;
      // size_t trueRootBlockSize = rootBlockSize * (1<<maxLevels);
      vec3i numBlocks = divRoundUp(input->dims,vec3i(blockSize));
      cout << "root grid size at this block size is " << numBlocks << endl;
      
      if (numBlocks * vec3i(blockSize) != input->dims)
        cout << "warning: input volume not a multiple of true root block size (ie, root block size after " << maxLevels << " binary refinements), some padding will happen" << endl;
      
      oct->allocate(numBlocks);//(numBlocks.x*numBlocks.y*numBlocks.z);

      Range<float> mm = input->getValueRange(vec3i(0),input->dims);
      cout << "value range for entire input is " << mm << endl;
      threshold = threshold * mm.size();
      cout << "at specified rel threshold of " << threshold << " this triggers splitting nodes whose difference exceeds " << threshold << endl;
      
      size_t oldSize = 0;
      for (int iz=0;iz<numBlocks.z;iz++)
        for (int iy=0;iy<numBlocks.y;iy++)
          for (int ix=0;ix<numBlocks.x;ix++) {
            makeBlock(vec3i(ix,iy,iz));
            size_t num = oct->octCell.size() - oldSize;
            cout << "oct[" << vec3f(ix,iy,iz) << "]: " << (8*num) << " entries (" << oct->octCell.size() << " total)" << endl;
            oldSize = oct->octCell.size();
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
      cout << "  ./ospRaw2Octree <inFile.raw> <args>" << endl;
      cout << "with args:" << endl;
      cout << " -dims <nx> <ny> <nz>   : input dimensions" << endl;
      cout << " --format <uint8|float> : input voxel format" << endl;
      cout << " --depth <maxlevels>    : use maxlevels octree refinement levels" << endl;
      cout << " -o <outfilename.xml>   : output file name" << endl;
      cout << " -t <threshold>         : threshold of which nodes to split or not (float val rel to min/max)" << endl;
      exit(msg != "");
    }

    extern "C" int main(int ac, char **av)
    {
      std::string inFileName = "";
      std::string outFileName = "";
      std::string format = "float";
      vec3i dims(0);

      OctAMRBuilder<float> builder;
      for (int i=1;i<ac;i++) {
        const std::string arg = av[i];
        if (arg == "-o")
          outFileName = av[++i];
        else if (arg == "--depth" || arg == "-depth" || arg == "--max-levels" || arg == "-ml")
          builder.maxLevels = atoi(av[++i]);
        else if (arg == "--threshold" || arg == "-t")
          builder.threshold = atof(av[++i]);
        else if (arg == "--format")
          format = av[++i];
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
      cout << "trying to load RAW file from " << inFileName << endl;
      loadRAW<float>(*builder.input,inFileName,dims);
      cout << "read input, now building AMR Octree" << endl;
      builder.makeAMR();
      cout << "done building" << endl;

      return 0;
    }

  } // ::ospray::amr
} // ::ospary
