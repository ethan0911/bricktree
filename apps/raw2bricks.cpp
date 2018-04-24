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
#include "../bt/BrickTreeBuilder.h"
#ifdef PARALLEL_MULTI_TREE_BUILD
# include <tbb/task_scheduler_init.h>
#endif
// ospcommon
#include "ospcommon/array3D/Array3D.h"

namespace ospray {
  namespace bt {

    using namespace array3D;
    
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
      cout << " --depth|-d <depth>     : num levels per block" << endl;
      cout << " -o <outfilename.osp>   : output file name" << endl;
      cout << " -t <threshold>         : threshold of which nodes to split or not (ABSOLUTE float val)" << endl;
      exit(msg != "");
    }

    template<int N, typename T>
    struct BlockBuilder : public BrickTreeBuilder<N,T> {
      
      BlockBuilder(std::shared_ptr<Array3D<T>> input, 
                   // BrickTreeBuilder<N,T> *builder,
                   int blockWidth,
                   float threshold
                   )
        : // builder(builder),
          valueRange(empty),
          input(input), 
          threshold(threshold),
          blockWidth(blockWidth)
      {
        this->valueRange = buildRec(this->averageValue,vec3i(0),0,blockWidth);
      }
      
      range_t<double> buildRec(double &avg, 
                             const vec3i &begin,
                             int level,
                             int blockWidth);
      
      void save(const std::string &ospFileName, const vec3i &validSize);

      range_t<double> valueRange;
      double        averageValue;
      const std::shared_ptr<Array3D<T>> input;
      const float threshold;
      const int blockWidth;
    };

    template<typename T>
    std::shared_ptr<Array3D<T>> openInput(const std::string &format,
                                                const vec3i &dims,
                                                const std::string &inFileName)
    {
      std::shared_ptr<Array3D<T>> input;
      cout << "mmapping RAW file (" << dims << ":" << typeToString<T>() << "):" 
           << inFileName << endl;
        
      if (format == "") {
        input = mmapRAW<T>(inFileName,dims);
      } else if (format == "uint8") {
        std::shared_ptr<Array3D<uint8_t>> input_uint8 = mmapRAW<uint8_t>(inFileName,dims);
        input = std::make_shared<Array3DAccessor<uint8_t,T>>(input_uint8);
      } else if (format == "float") {
        std::shared_ptr<Array3D<float>> input_float = mmapRAW<float>(inFileName,dims);
        input = std::make_shared<Array3DAccessor<float,T>>(input_float);
      } else if (format == "double") {
        std::shared_ptr<Array3D<double>> input_double = mmapRAW<double>(inFileName,dims);
        input = std::make_shared<Array3DAccessor<double,T>>(input_double);
      } else
        throw std::runtime_error("unsupported format '"+format+"'");
        
      return input;
    }

    template<typename T>
    std::shared_ptr<Array3D<T>> openInput(const std::string &format,
                                                const vec3i &dims,
                                                const std::vector<std::string> &inFileName)
    {
      if (inFileName.size() == 1) 
        return openInput<T>(format,dims,inFileName[0]);
      else if (inFileName.size() == (size_t)dims.z) {
        cout << "=======================================================" << endl;
        cout << "looks like a multi-slice input of " << dims.z << " slices" << endl;
        cout << "=======================================================" << endl;
        std::vector<std::shared_ptr<Array3D<T>>> slices;
        const vec3i sliceDims(dims.x,dims.y,1);
        for (int z=0;z<dims.z;z++) {
          slices.push_back(openInput<T>(format,sliceDims,inFileName[z]));
          assert(slices.back()->size().z == 1);
          assert(slices.back()->size() == slices[0]->size());
        }
        return std::make_shared<MultiSliceArray3D<T>>(slices);
      } else
        throw std::runtime_error("do not understand input - neither a single raw file, now what looks like a multi-slice input!?");
    }
    
    template<int N, typename T>
    range_t<double> BlockBuilder<N,T>::buildRec(double &avg, 
                                              const vec3i &begin,
                                              int level,
                                              int levelWidth)
    {
      vec3i lo = min(input->size(),begin*levelWidth);
      vec3i hi = min(input->size(),lo + vec3i(levelWidth));

      if ((hi-lo).product() == 0) { avg = T(0); return range_t<double>(T(0),T(0)); }

      range_t<double> range = empty;
      typename BrickTree<N,T>::ValueBrick db;
      db.clear();

      const int cellSize = levelWidth / N;
        
      if (levelWidth == N) {
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
      avg = db.computeWeightedAverage(begin,levelWidth,input->size());

      if ((range.upper - range.lower) <= threshold) {
        // float r = range.upper - range.lower;
        // std::cout<<"t: " <<threshold << " r: "<<r<<std::endl;
        // do not set any fields - kill that brick
      } else {
        // need to actually create this brick:
        for (int iz = 0; iz < N; iz++)
          for (int iy = 0; iy < N; iy++)
            for (int ix = 0; ix < N; ix++) {
              const vec3i cellID = N * begin + vec3i(ix, iy, iz);
              if (cellID.x * cellSize < input->size().x &&
                  cellID.y * cellSize < input->size().y &&
                  cellID.z * cellSize < input->size().z) {
                this->set(cellID, level, db.value[iz][iy][ix]);
              }
            }
      }

      return range;
    }

    template<int N, typename T>
    void buildIt(int blockID,
                 const std::string &inputFormat,
                 const vec3i &dims,
                 const std::vector<std::string> &inFileName,
                 const std::string &outFileName,
                 const box3i &clipBox,
                 const float threshold,
                 const int blockDepth)
    {
      std::shared_ptr<Array3D<T>> org_input = openInput<T>(inputFormat,dims,inFileName);
      std::shared_ptr<Array3D<T>> input = std::make_shared<SubBoxArray3D<T>>(org_input,clipBox);
      // threshold = 0.f;

      std::string inputFilesString = "";
      for (auto fn : inFileName)
        inputFilesString += (fn+" ");
      int blockWidth = 1;
      for (int i=0;i<blockDepth;i++)
        blockWidth *= N;
      PRINT(blockWidth);
      const vec3i rootGridSize = divRoundUp(input->size(),vec3i((int)blockWidth));
      PRINT(rootGridSize);
      const size_t numBlocks = rootGridSize.product();
      if (numBlocks >= 1000000)
        throw std::runtime_error("too many blocks ... consider changing bricksisze or block depth");


      const std::string format = typeToString<T>();
      if (blockID == -1) {
        // =======================================================
        // brickID == -1: create the makefile, nothing else
        // =======================================================
        // this is the "create makefile" case
        const std::string makeFileName = outFileName+".mak";
        FILE *out = fopen(makeFileName.c_str(),"w");
        assert(out);

        fprintf(out,"# makefile generated by raw2bricks tool\n\n");
        fprintf(out,"all:");
        for (size_t i=0;i<numBlocks;i++)
          fprintf(out," \\\n\t%s-brick%06li.osp",outFileName.c_str(),i);
        fprintf(out,"\n");

        fprintf(out,"RAW2BRICKS=./ospRaw2Bricks\n\n");

        for (size_t i=0;i<numBlocks;i++) {
          fprintf(out,"%s-brick%06li.osp: %s.mak\n",outFileName.c_str(),i,outFileName.c_str());
          fprintf(out,"\t${RAW2BRICKS}"
                  " -o %s"
                  " --blockID %li"
                  " --format %s"
                  " --input-format %s"
                  " --dimensions %i %i %i"
                  " --brick-size %i"
                  " --clip-box %i %i %i %i %i %i"
                  " --depth %i"
                  " --threshold %f"
                  " %s"
                  "\n",
                  outFileName.c_str(),
                  i,
                  format.c_str(),
                  (inputFormat!=""?inputFormat.c_str():format.c_str()),
                  dims.x,dims.y,dims.z,
                  N,
                  clipBox.lower.x,clipBox.lower.y,clipBox.lower.z,
                  clipBox.size().x,clipBox.size().y,clipBox.size().z,
                  blockDepth,
                  threshold,
                  inputFilesString.c_str()
                  );
          fprintf(out,"\n");
        }
        cout << "done writing makefile." << endl;
        fclose(out);

        const std::string ospFileName = outFileName+".osp";
        FILE *osp = fopen(ospFileName.c_str(),"w");
        fprintf(osp,"<?xml?>\n");
        fprintf(osp,"<ospray>\n");
        {
          fprintf(osp,"<MultiBrickTree\n");
          fprintf(osp,"   gridSize=\"%i %i %i\"\n",rootGridSize.x,rootGridSize.y,rootGridSize.z);
          vec3i inputSize = input->size();
          fprintf(osp,"   format=\"%s\"\n",typeToString<T>());
          fprintf(osp,"   brickSize=\"%i\"\n",N);
          fprintf(osp,"   blockWidth=\"%i\"\n",blockWidth);
          fprintf(osp,"   validSize=\"%i %i %i\"\n",inputSize.x,inputSize.y,inputSize.z);
          fprintf(osp,"\t/>\n");
        }
        fprintf(osp,"</ospray>\n");
        fclose(osp);
        cout << "done writing multibrick scene graph '.osp' file name..." << endl;
        exit(0);
      } else {
        // =======================================================
        // actual block ID >= 0: build that bricktree
        // =======================================================
        vec3i blockIdx;
        blockIdx.z = blockID / (rootGridSize.x*rootGridSize.y);
        blockIdx.y = (blockID / rootGridSize.x) % rootGridSize.y;
        blockIdx.x = blockID % rootGridSize.x;
        cout << "----------------------------" << endl;
        box3i blockDims;
        blockDims.lower = blockIdx*blockWidth;
        blockDims.upper = min(blockDims.lower+vec3i(blockWidth),input->size());
        cout << "building block " << blockID << "/" << numBlocks << " (" << (int)(100.f*blockID/float(numBlocks)) << "%) " << blockIdx << " / " << rootGridSize << ", coords = " << blockDims << endl;
        cout << "reading in actual block data" << endl;
        std::shared_ptr<ActualArray3D<T>> blockInput = std::make_shared<ActualArray3D<T>>(blockDims.size());
        tbb::parallel_for(0, (int)blockDims.size().z, 1, [&](int iz){
            for (int iy=0;iy<blockDims.size().y;iy++)
              for (int ix=0;ix<blockDims.size().x;ix++)
                blockInput->set(vec3i(ix,iy,iz),input->get(blockDims.lower+vec3i(ix,iy,iz)));
          });
        cout << "done; now building bricktree..." << endl;
        cout << "----------------------------" << endl;
        // BrickTreeBuilder<N,T> *builder = new BrickTreeBuilder<N,T>;
        BlockBuilder<N,T> block(blockInput,blockWidth,threshold);
        cout << "done building block's bricktree ... " << endl;
        PRINT(block.averageValue);
        PRINT(block.valueRange);
        char blockOutName[outFileName.size()+100];
        sprintf(blockOutName,"%s-brick%06i.osp",outFileName.c_str(),blockID);
        cout << "saving tree to " << blockOutName << endl;
        block.save(blockOutName,blockInput->size());
        cout << "done saving block's bricktree... exiting!" << endl;
        cout << "=========================================" << endl;
        exit(0);
      }
    }

    template<int N, typename T>
    void BlockBuilder<N,T>::save(const std::string &ospFileName, const vec3i &validSize)
    {
      const std::string binFileName = ospFileName+"bin";
      FILE *bin = fopen(binFileName.c_str(),"wb");
      
      size_t indexOfs = ftell(bin);
      for (size_t i=0;i<this->indexBrick.size();i++) {
        const typename BrickTree<N,T>::IndexBrick *brick = this->indexBrick[i];
        if (!fwrite(brick,sizeof(*brick),1,bin))
          throw std::runtime_error("could not write ... disk full!?");
      }
      
      size_t dataOfs = ftell(bin);      
      for (size_t i=0;i<this->valueBrick.size();i++) {
        const typename BrickTree<N,T>::ValueBrick *brick = this->valueBrick[i];
        if (!fwrite(brick,sizeof(*brick),1,bin))
          throw std::runtime_error("could not write ... disk full!?");
      }
      
      size_t indexBrickOfOfs = ftell(bin);      
      for (size_t i=0;i<this->indexBrickOf.size();i++)
        if (!fwrite(&this->indexBrickOf[i],sizeof(this->indexBrickOf[i]),1,bin))
          throw std::runtime_error("could not write ... disk full!?");
      
      fclose(bin);

      FILE *osp = fopen(ospFileName.c_str(),"w");
      fprintf(osp,"<?xml?>\n");
      fprintf(osp,"<ospray>\n");
      {
        fprintf(osp,"  <BrickTree\n");
        {
          fprintf(osp,"    averageValue=\"%f\"\n",averageValue);
          fprintf(osp,"    valueRange=\"%f %f\"\n",valueRange.lower,valueRange.upper);
          fprintf(osp,"    format=\"%s\"\n",typeToString<T>());
          fprintf(osp,"    brickSize=\"%i\"\n",N);
          fprintf(osp,"    validSize=\"%i %i %i\"\n",validSize.x,validSize.y,validSize.z);
          fprintf(osp,"    >\n");
          fprintf(osp,"    <indexBricks num=\"%li\" ofs=\"%li\"/>\n",
                  this->indexBrick.size(),indexOfs);
          fprintf(osp,"    <valueBricks num=\"%li\" ofs=\"%li\"/>\n",
                  this->valueBrick.size(),dataOfs);
          fprintf(osp,"    <indexBrickOf num=\"%li\" ofs=\"%li\"/>\n",
                  this->indexBrickOf.size(),indexBrickOfOfs);
        }
        fprintf(osp,"  </BrickTree>\n");
      }
      fprintf(osp,"</ospray>\n");
      fclose(osp);
    }

    template<int N>
    void buildIt(int blockID,
                 const std::string &inputFormat,
                 const std::string &treeFormat,
                 const vec3i &dims,
                 const std::vector<std::string> &inFileName,
                 const std::string &outFileName,
                 const box3i &clipBox,
                 const float threshold,
                 const int blockDepth)
    {
      if (treeFormat == "uint8")
        buildIt<N,uint8_t>(blockID,inputFormat,dims,inFileName,outFileName,clipBox,threshold,blockDepth);
      else if (treeFormat == "float")
        buildIt<N,float>(blockID,inputFormat,dims,inFileName,outFileName,clipBox,threshold,blockDepth);
      else if (treeFormat == "double")
        buildIt<N,double>(blockID,inputFormat,dims,inFileName,outFileName,clipBox,threshold,blockDepth);
      else 
        error("unsupported format");
    }

    extern "C" int main(int ac, char **av)
    {
      int blockID = -1;
      std::vector<std::string> inFileName;
      std::string outFileName = "";
      std::string inputFormat = "";
      std::string treeFormat  = "float";
      int         blockDepth   = -1;
      float       threshold   = 0.f;
      vec3i       dims        = vec3i(0);
      int         brickSize   = 4;
      box3i       clipBox(vec3i(-1),vec3i(-1));

      for (int i=1;i<ac;i++) {
        const std::string arg = av[i];
        if (arg == "-o")
          outFileName = av[++i];
        else if (arg == "--depth" ||
                 arg == "-depth" ||
                 arg == "-d" ||
                 arg == "--max-levels" ||
                 arg == "-ml")
          blockDepth = atoi(av[++i]);
        else if (arg == "--threshold" || arg == "-t")
          threshold = atof(av[++i]);
        else if (arg == "--brick-size" || arg == "-bs")
          brickSize = atof(av[++i]);
        else if (arg == "-bid" || arg == "--blockID")
          blockID = atoi(av[++i]);
        else if (arg == "--format" || arg == "-f")
          treeFormat = av[++i];
        else if (arg == "--input-format" || arg == "-if")
          inputFormat = av[++i];
        else if (arg == "-dims" || arg == "--dimensions") {
          dims.x = atoi(av[++i]);
          dims.y = atoi(av[++i]);
          dims.z = atoi(av[++i]);
        }
        else if (arg == "--num-threads" || arg == "-nt") {
#ifdef PARALLEL_MULTI_TREE_BUILD
          int numThreads = atoi(av[++i]);
          static tbb::task_scheduler_init tbb_init(numThreads);
#else
          ++i;
          cout << "tbb support not compiled in ... " << endl;
#endif
        }
        else if (arg == "--clip" || arg == "--sub-box" || arg == "--clip-box") {
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
      
      if (blockDepth < 0) {
        blockDepth = (int)(logf(256.f)/logf(brickSize)+.5f);
        cout << "automatically set block depth to " << blockDepth << endl;
      }
      switch (brickSize) {
      case 2:
        buildIt<2>(blockID,inputFormat,treeFormat,dims,inFileName,outFileName,clipBox,threshold,blockDepth);
        break;
      case 4:
        buildIt<4>(blockID,inputFormat,treeFormat,dims,inFileName,outFileName,clipBox,threshold,blockDepth);
        break;
      case 8:
        buildIt<8>(blockID,inputFormat,treeFormat,dims,inFileName,outFileName,clipBox,threshold,blockDepth);
        break;
      case 16:
        buildIt<16>(blockID,inputFormat,treeFormat,dims,inFileName,outFileName,clipBox,threshold,blockDepth);
        break;
      case 32:
        buildIt<32>(blockID,inputFormat,treeFormat,dims,inFileName,outFileName,clipBox,threshold,blockDepth);
        break;
      case 64:
        buildIt<64>(blockID,inputFormat,treeFormat,dims,inFileName,outFileName,clipBox,threshold,blockDepth);
        break;
      default:
        error("unsupported brick size ...");
      };
      return 0;
    }

  }
}
