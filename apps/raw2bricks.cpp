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

// own
#include "../amr/BrickTreeBuilder.h"

namespace ospray {
  namespace amr {

    using std::endl;
    using std::cout;

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
      cout << " --format|-f <uint8|float|double> : desired voxel type" << endl;
      cout << " --input-format|-if <uint8|float|double>: format of input raw file (if different from '--format')" << endl;
      cout << " --brick-size|-bs <N>   : use bricks of NxNxN voxels" << endl;
      cout << " --depth <maxlevels>    : use maxlevels octree refinement levels" << endl;
      cout << " -o <outfilename.xml>   : output file name" << endl;
      cout << " -t <threshold>         : threshold of which nodes to split or not (ABSOLUTE float val)" << endl;
      exit(msg != "");
    }

#if 1

    template<int N, typename T>
    void progress(MultiBrickTreeBuilder<N,T> *builder,
                  const Array3D<T> *input,
                  const vec3i &begin=vec3i(-1), 
                  const vec3i &end=vec3i(-1))
    {
      static std::mutex progressMutex;
      std::lock_guard<std::mutex> lock(progressMutex);

      static size_t numDone = 0;

      size_t numIndexBricks = 0;
      size_t numDataBricks = 0;

      assert(builder);
      cout << "done brick " << begin << " - " << end
           << " num = " << reduce_mul(end-begin) << endl;
      numDone += reduce_mul((end-begin));
      double pctgDone = 100.f * numDone / double(input->numElements());
      cout << "build update (done w/ " << pctgDone << "% of volume)" << endl;

      for (int iz=0;iz<builder->getRootGridSize().z;iz++)
        for (int iy=0;iy<builder->getRootGridSize().y;iy++)
          for (int ix=0;ix<builder->getRootGridSize().x;ix++) {
            BrickTreeBuilder<N,T> *msb = builder->getRootCell(vec3i(ix,iy,iz));
            if (msb) {
              numIndexBricks += msb->indexBrick.size();
              numDataBricks += msb->dataBrick.size();
            }
          }

      cout << "- root grid size " << builder->getRootGridSize() << endl;
      cout << "- total num index bricks " << prettyNumber(numIndexBricks)
           << " (estd " << prettyNumber(long(numIndexBricks * 100.f / pctgDone)) << ")" << endl;
      cout << "- total num data bricks " << prettyNumber(numDataBricks)
           << " (estd " << prettyNumber(long(numDataBricks * 100.f / pctgDone)) << ")" << endl;

      size_t totalSize
        = numIndexBricks * sizeof(typename BrickTree<N,T>::IndexBrick)
        + numDataBricks * (sizeof(typename BrickTree<N,T>::DataBrick) + sizeof(int));
      
      cout << "- total size (bytes) " << prettyNumber(totalSize)
           << " (estd " << prettyNumber(long(totalSize * 100.f / pctgDone)) << ")" << endl;
      size_t sizeExpected = long(totalSize * 100.f / pctgDone);
      size_t sizeOriginal = input->numElements()*sizeof(T);
      cout << "[that's a compression rate (ASSUMING INPUT WAS TS) of " << (sizeExpected *100.f / sizeOriginal) << "%]" << endl << endl;;
    }
#endif

    template<int N, typename T>
    struct SumFromArrayBuilder {
      
      SumFromArrayBuilder(const Array3D<T> *input, 
                          MultiBrickTreeBuilder<N,T> *builder,
                          float threshold = 0.f,
                          int rootGridLevel = 0)
        : input(input), 
          threshold(threshold), 
          valueRange(empty), 
          builder(builder),
          rootGridLevel(rootGridLevel)
      {
        int rootSize = N;
        while (rootSize < reduce_max(input->size())) 
          rootSize *= N;
        
#ifdef PARALLEL_MULTI_TREE_BUILD
        int rootLevelBrickSize = rootSize;
        for (int i=0;i<rootGridLevel;i++)
          rootLevelBrickSize /= N;
        PRINT(rootLevelBrickSize);
        PRINT(rootGridLevel);
        vec3i rootGridDims = divRoundUp(input->size(),vec3i(rootLevelBrickSize));
        PRINT(rootGridDims);
        int numRootGridBricks = rootGridDims.product();
        std::mutex mutex;
        tbb::parallel_for(0, numRootGridBricks, 1, [&](int brickID)
                          {
                            vec3i rootID;
                            rootID.x = brickID % rootGridDims.x;
                            rootID.y = (brickID / rootGridDims.x) % rootGridDims.y;
                            rootID.z = (brickID / rootGridDims.x / rootGridDims.y);
                            double avg;
                            Range<double> myRange = buildRec(avg,rootID,
                                                             //0,
                                                             rootGridLevel,
                                                             rootLevelBrickSize);
                            
                            mutex.lock();
                            valueRange.extend(myRange);
                            mutex.unlock();
                          }
                          );
        
#else
        double avg;
        
        valueRange = buildRec(avg,vec3i(0),0,rootSize);
#endif
      }

      std::vector<typename BrickTree<N,T>::IndexBrick> indexBrick;
      std::vector<typename BrickTree<N,T>::DataBrick>  dataBrick;
      std::vector<typename BrickTree<N,T>::BrickInfo>  brickInfo;
      
      Range<double> buildRec(double &avg, 
                             const vec3i &begin,
                             int level,
                             int brickSize);
      
      Range<double> valueRange;
      const Array3D<T> *input;
      const float threshold;
      MultiBrickTreeBuilder<N,T> *builder;
      int rootGridLevel;
    };

    template<typename T>
    const Array3D<T> *openInput(const std::string &format,
                                const vec3i &dims,
                                const std::string &inFileName)
    {
      const Array3D<T> *input = NULL;
      cout << "mmapping RAW file (" << dims << ":" << typeToString<T>() << "):" << inFileName << endl;
      // cout << "  input file is   " << inFileName[0] << endl;
      // cout << "  expected format " << typeToString<T>() << endl;
      // cout << "  expected dims   " << dims << endl;
        
      if (format == "") {
        input = mmapRAW<T>(inFileName,dims);
      } else if (format == "uint8") {
        const Array3D<uint8> *input_uint8 = mmapRAW<uint8>(inFileName,dims);
        input = new Array3DAccessor<uint8,T>(input_uint8);
      } else if (format == "float") {
        const Array3D<float> *input_float = mmapRAW<float>(inFileName,dims);
        input = new Array3DAccessor<float,T>(input_float);
      } else if (format == "double") {
        const Array3D<double> *input_double = mmapRAW<double>(inFileName,dims);
        input = new Array3DAccessor<double,T>(input_double);
      } else
        throw std::runtime_error("unsupported format '"+format+"'");
        
      // cout << "loading complete." << endl;
      return input;
    }

    template<typename T>
    const Array3D<T> *openInput(const std::string &format,
                                const vec3i &dims,
                                const std::vector<std::string> &inFileName)
    {
      if (inFileName.size() == 1) 
        return openInput<T>(format,dims,inFileName[0]);
      else if (inFileName.size() == dims.z) {
        cout << "=======================================================" << endl;
        cout << "looks like a multi-slice input of " << dims.z << " slices" << endl;
        cout << "=======================================================" << endl;
        std::vector<const Array3D<T> *> slices;
        const vec3i sliceDims(dims.x,dims.y,1);
        for (int z=0;z<dims.z;z++) {
          slices.push_back(openInput<T>(format,sliceDims,inFileName[z]));
          assert(slices.back()->size().z == 1);
          assert(slices.back()->size() == slices[0]->size());
        }
        return new MultiSliceArray3D<T>(slices);
      } else
        throw std::runtime_error("do not understand input - neither a single raw file, now what looks like a multi-slice input!?");
    }
    
    template<int N, typename T>
    Range<double> SumFromArrayBuilder<N,T>::buildRec(double &avg, 
                                                     const vec3i &begin,
                                                     int level,
                                                     int brickSize)
    {
      vec3i lo = min(input->size(),begin*brickSize);
      vec3i hi = min(input->size(),lo + vec3i(brickSize));
      if ((hi-lo).product() == 0) { avg = T(0); return Range<double>(T(0),T(0)); }

      bool output = level == rootGridLevel; //(brickSize == 64); //N*N*N);
      if (output) 
        cout << "building brick " << lo << " - " << hi << " bs " << brickSize << endl;

      Range<double> range = empty;
      typename BrickTree<N,T>::DataBrick db;
      db.clear();

      const int cellSize = brickSize / N;
        
      if (brickSize == N) {
        // -------------------------------------------------------
        // LEAF
        // -------------------------------------------------------
        for (int iz=0;iz<N;iz++)
          for (int iy=0;iy<N;iy++)
            for (int ix=0;ix<N;ix++) {
              db.value[iz][iy][ix] = input->get(N*begin+vec3i(ix,iy,iz));
              range.extend(db.value[iz][iy][ix]);
            }
      }
      else {
        // -------------------------------------------------------
        // INNER
        // -------------------------------------------------------
        for (int iz=0;iz<N;iz++)
          for (int iy=0;iy<N;iy++)
            for (int ix=0;ix<N;ix++) {
              double avg;
              range.extend(buildRec(avg,
                                    N*begin+vec3i(ix,iy,iz),
                                    level+1,
                                    cellSize
                                    ));
              db.value[iz][iy][ix] = (T)avg;
            }
      }
      // now, compute average of this node - we need this even if the node gets killed...
      avg = db.computeWeightedAverage(begin,brickSize,input->size());        

      if (((range.hi - range.lo) <= threshold)
          && 
          level > rootGridLevel) 
        {
          // do not set any fields - kill that brick
        } 
      else {
        // need to actually create this brick:
        if (output)
          cout << "inserting brick " << begin << ":" << level << endl;
        for (int iz=0;iz<N;iz++)
          for (int iy=0;iy<N;iy++)
            for (int ix=0;ix<N;ix++) {
              const vec3i cellID = N*begin + vec3i(ix,iy,iz);
              if (cellID.x*cellSize < input->size().x &&
                  cellID.y*cellSize < input->size().y &&
                  cellID.z*cellSize < input->size().z) {
                if (level >= rootGridLevel) {
                  builder->set(cellID,level-rootGridLevel,db.value[iz][iy][ix]);
                }
              }
            }
      }
      if (output) {
        progress(builder,input,lo,hi);
      }


      return range;
    }

    template<int N, typename T>
    void buildIt(const std::string &inputFormat,
                 const vec3i &dims,
                 const std::vector<std::string> &inFileName,
                 const std::string &outFileName,
                 const box3i &clipBox,
                 const float threshold,
                 const int maxLevels)
    {
      const Array3D<T> *org_input = openInput<T>(inputFormat,dims,inFileName);
      const Array3D<T> *input = new SubBoxArray3D<T>(org_input,clipBox);
      // threshold = 0.f;
      int rootGridLevel = 0;
      // compute num skiplevels based on num levels specified:
      int levelWidth = reduce_max(input->size());
      for (int i=0;i<maxLevels;i++)
        levelWidth = divRoundUp(levelWidth,N);
      while (levelWidth > N) {
        rootGridLevel++;
        levelWidth = divRoundUp(levelWidth,N);
      }
      cout << "building tree with " << rootGridLevel << " skip levels" << endl;

      MultiBrickTreeBuilder<N,T> *builder = new MultiBrickTreeBuilder<N,T>;
      SumFromArrayBuilder<N,T>(input,builder,threshold,rootGridLevel);
      cout << "done building!" << endl;
      progress(builder,input);

      cout << "saving to output file " << outFileName << endl;

      vec3i encodedSize = builder->getRootGridSize();
      while (encodedSize.x < input->size().x) encodedSize.x *= N;
      while (encodedSize.y < input->size().y) encodedSize.y *= N;
      while (encodedSize.z < input->size().z) encodedSize.z *= N;

      vec3f clipBoxSize = vec3f(input->size()) / vec3f(encodedSize);
      
      builder->save(outFileName,clipBoxSize);
      cout << "done writing multi-sum tree" << endl;
    }


    template<int N>
    void buildIt(const std::string &inputFormat,
                 const std::string &treeFormat,
                 const vec3i &dims,
                 const std::vector<std::string> &inFileName,
                 const std::string &outFileName,
                 const box3i &clipBox,
                 const float threshold,
                 const int maxLevels)
    {
      if (treeFormat == "uint8")
        buildIt<N,uint8>(inputFormat,dims,inFileName,outFileName,clipBox,threshold,maxLevels);
      else if (treeFormat == "float")
        buildIt<N,float>(inputFormat,dims,inFileName,outFileName,clipBox,threshold,maxLevels);
      else if (treeFormat == "double")
        buildIt<N,double>(inputFormat,dims,inFileName,outFileName,clipBox,threshold,maxLevels);
      else 
        error("unsupported format");
    }

    extern "C" int main(int ac, char **av)
    {
      std::vector<std::string> inFileName;
      std::string outFileName = "";
      std::string inputFormat = "";
      std::string treeFormat  = "float";
      int         maxLevels   = 3;
      float       threshold   = 0.f;
      vec3i       dims        = vec3i(0);
      int         brickSize   = 4;
      box3i       clipBox(vec3i(-1),vec3i(-1));

      for (int i=1;i<ac;i++) {
        const std::string arg = av[i];
        if (arg == "-o")
          outFileName = av[++i];
        else if (arg == "--depth" || arg == "-depth" || 
                 arg == "--max-levels" || arg == "-ml")
          maxLevels = atoi(av[++i]);
        else if (arg == "--threshold" || arg == "-t")
          threshold = atof(av[++i]);
        else if (arg == "--brick-size" || arg == "-bs")
          brickSize = atof(av[++i]);
        else if (arg == "--format")
          inputFormat = av[++i];
        else if (arg == "--input-format" || arg == "-if")
          treeFormat = av[++i];
        else if (arg == "-dims" || arg == "--dimensions") {
          dims.x = atoi(av[++i]);
          dims.y = atoi(av[++i]);
          dims.z = atoi(av[++i]);
        }
        else if (arg == "--clip" || arg == "--sub-box") {
          clipBox.lower.x = atof(av[++i]);
          clipBox.lower.y = atof(av[++i]);
          clipBox.lower.z = atof(av[++i]);
          clipBox.upper = clipBox.lower;
          clipBox.upper.x += atof(av[++i]);
          clipBox.upper.y += atof(av[++i]);
          clipBox.upper.z += atof(av[++i]);
        }
        else if (arg[0] != '-')
          inFileName.push_back(av[i]);
        else
          error("unknown arg '"+arg+"'");
      }
      if (clipBox.lower == vec3i(-1))
        clipBox.lower = vec3i(0);
      if (clipBox.upper == vec3i(-1))
        clipBox.upper = dims;
      else
        clipBox.upper = min(clipBox.upper,dims);
      assert(!clipBox.empty());
      
      
      if (dims == vec3i(0))
        error("no input dimensions (-dims) specified");
      if (inFileName.empty())
        error("no input file(s) specified");
      if (outFileName == "")
        error("no output file specified");
      
      switch (brickSize) {
      case 2:
        buildIt<2>(inputFormat,treeFormat,dims,inFileName,outFileName,clipBox,threshold,maxLevels);
        break;
      case 4:
        buildIt<4>(inputFormat,treeFormat,dims,inFileName,outFileName,clipBox,threshold,maxLevels);
        break;
      case 8:
        buildIt<8>(inputFormat,treeFormat,dims,inFileName,outFileName,clipBox,threshold,maxLevels);
        break;
      case 16:
        buildIt<16>(inputFormat,treeFormat,dims,inFileName,outFileName,clipBox,threshold,maxLevels);
        break;
      case 32:
        buildIt<32>(inputFormat,treeFormat,dims,inFileName,outFileName,clipBox,threshold,maxLevels);
        break;
      case 64:
        buildIt<64>(inputFormat,treeFormat,dims,inFileName,outFileName,clipBox,threshold,maxLevels);
        break;
      default:
        error("unsupported brick size ...");
      };
      return 0;
    }

  }
}
