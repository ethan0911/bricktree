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
#ifdef PARALLEL_MULTI_TREE_BUILD
# include <tbb/task_scheduler_init.h>
#endif


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
      cout << " --depth <depth>        : num levels per block" << endl;
      cout << " -o <outfilename.xml>   : output file name" << endl;
      cout << " -t <threshold>         : threshold of which nodes to split or not (ABSOLUTE float val)" << endl;
      exit(msg != "");
    }

// #if 1

//     template<int N, typename T>
//     void progress(MultiBrickTreeBuilder<N,T> *builder,
//                   const Array3D<T> *input,
//                   const vec3i &begin=vec3i(-1), 
//                   const vec3i &end=vec3i(-1))
//     {
//       static std::mutex progressMutex;
//       std::lock_guard<std::mutex> lock(progressMutex);

//       static size_t numDone = 0;

//       size_t numIndexBricks = 0;
//       size_t numDataBricks = 0;

//       assert(builder);
//       cout << "done brick " << begin << " - " << end
//            << " num = " << reduce_mul(end-begin) << endl;
//       numDone += reduce_mul((end-begin));
//       double pctgDone = 100.f * numDone / double(input->numElements());
//       cout << "build update (done w/ " << pctgDone << "% of volume)" << endl;

//       for (int iz=0;iz<builder->getRootGridSize().z;iz++)
//         for (int iy=0;iy<builder->getRootGridSize().y;iy++)
//           for (int ix=0;ix<builder->getRootGridSize().x;ix++) {
//             BrickTreeBuilder<N,T> *msb = builder->getRootCell(vec3i(ix,iy,iz));
//             if (msb) {
//               numIndexBricks += msb->indexBrick.size();
//               numDataBricks += msb->dataBrick.size();
//             }
//           }

// 	cout << "- format " 
// 		<< typeToString<T>() << " BS " << N 
// 		<< " dataBrickSz = " << sizeof(typename BrickTree<N,T>::DataBrick) 
// 		<< " idxBrickSz = " << sizeof(typename BrickTree<N,T>::IndexBrick) 
// 		<< endl;
//       cout << "- root grid size " << builder->getRootGridSize() << endl;
//       cout << "- total num index bricks " << prettyNumber(numIndexBricks)
//            << " (estd " << prettyNumber(long(numIndexBricks * 100.f / pctgDone)) << ")" << endl;
//       cout << "- total num data bricks " << prettyNumber(numDataBricks)
//            << " (estd " << prettyNumber(long(numDataBricks * 100.f / pctgDone)) << ")" << endl;

//       size_t totalSize
//         = numIndexBricks * sizeof(typename BrickTree<N,T>::IndexBrick)
//         + numDataBricks * (sizeof(typename BrickTree<N,T>::DataBrick) + sizeof(int));
      
//       cout << "- total size (bytes) " << prettyNumber(totalSize)
//            << " (estd " << prettyNumber(long(totalSize * 100.f / pctgDone)) << ")" << endl;
//       size_t sizeExpected = long(totalSize * 100.f / pctgDone);
//       size_t sizeOriginal = input->numElements()*sizeof(T);
//       cout << "[that's a compression rate (ASSUMING INPUT WAS TS) of " << (sizeExpected *100.f / sizeOriginal) << "%]" << endl << endl;;
//     }
// #endif

    template<int N, typename T>
    struct BlockBuilder : public BrickTreeBuilder<N,T> {
      
      BlockBuilder(const Array3D<T> *input, 
                   // BrickTreeBuilder<N,T> *builder,
                   int blockWidth,
                   float threshold
                   )
        : input(input), 
          threshold(threshold), 
          // builder(builder),
          valueRange(empty),
          blockWidth(blockWidth)
      {
        this->valueRange = buildRec(this->averageValue,vec3i(0),0,blockWidth);
      }
      
      // std::vector<typename BrickTree<N,T>::IndexBrick> indexBrick;
      // std::vector<typename BrickTree<N,T>::DataBrick>  dataBrick;
      // std::vector<typename BrickTree<N,T>::BrickInfo>  brickInfo;
      
      Range<double> buildRec(double &avg, 
                             const vec3i &begin,
                             int level,
                             int blockWidth);
      
      void save(const std::string &ospFileName, const vec3i &validSize);

      Range<double> valueRange;
      double        averageValue;
      const Array3D<T> *input;
      const float threshold;
      // BrickTreeBuilder<N,T> *builder;
      const int blockWidth;
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
    Range<double> BlockBuilder<N,T>::buildRec(double &avg, 
                                              const vec3i &begin,
                                              int level,
                                              int levelWidth)
    {
      // PING;
      // PRINT(begin);
      // PRINT(level);
      // PRINT(levelWidth);

      vec3i lo = min(input->size(),begin*levelWidth);
      vec3i hi = min(input->size(),lo + vec3i(levelWidth));
      // PRINT(lo);
      // PRINT(hi);

      if ((hi-lo).product() == 0) { avg = T(0); return Range<double>(T(0),T(0)); }

      // bool output = level == rootGridLevel; //(levelWidth == 64); //N*N*N);
      // if (output) 
      //   cout << "building brick " << lo << " - " << hi << " bs " << levelWidth << endl;

      Range<double> range = empty;
      typename BrickTree<N,T>::DataBrick db;
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

      if ((range.hi - range.lo) <= threshold)
          // && 
          // level > rootGridLevel) 
        {
          // do not set any fields - kill that brick
        } 
      else {
        // need to actually create this brick:
        // if (output)
        //   cout << "inserting brick " << begin << ":" << level << endl;
        for (int iz=0;iz<N;iz++)
          for (int iy=0;iy<N;iy++)
            for (int ix=0;ix<N;ix++) {
              const vec3i cellID = N*begin + vec3i(ix,iy,iz);
              if (cellID.x*cellSize < input->size().x &&
                  cellID.y*cellSize < input->size().y &&
                  cellID.z*cellSize < input->size().z) {
                // if (level >= rootGridLevel) {
                this->set(cellID,level,db.value[iz][iy][ix]);
                // builder->set(cellID,level,db.value[iz][iy][ix]);
                // }
              }
            }
      }
      // if (output) {
      //   progress(builder,input,lo,hi);
      // }


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
      const Array3D<T> *org_input = openInput<T>(inputFormat,dims,inFileName);
      const Array3D<T> *input = new SubBoxArray3D<T>(org_input,clipBox);
      // threshold = 0.f;

      std::string inputFilesString = "";
      for (int i=0;i<inFileName.size();i++)
        inputFilesString += (inFileName[i]+" ");
      int blockWidth = 1;
      for (int i=0;i<blockDepth;i++)
        blockWidth *= N;
      PRINT(blockWidth);
      // int rootGridLevel = 0;
      // compute num skiplevels based on num levels specified:
      // int levelWidth = reduce_max(input->size());
      // for (int i=0;i<blockDepth;i++)
      //   levelWidth = divRoundUp(levelWidth,N);
      // while (levelWidth > N) {
      //   rootGridLevel++;
      //   levelWidth = divRoundUp(levelWidth,N);
      // }
      // cout << "building tree with " << rootGridLevel << " skip levels" << endl;
      // PRINT(levelWidth);
      const vec3i rootGridSize = divRoundUp(input->size(),vec3i((int)blockWidth));
      const std::string format = typeToString<T>();
      if (blockID == -1) {
        // =======================================================
        // brickID == -1: create the makefile, nothing else
        // =======================================================
        // this is the "create makefile" case
        const std::string makeFileName = outFileName+".mak";
        FILE *out = fopen(makeFileName.c_str(),"w");
        assert(out);

        size_t numBlocks = rootGridSize.product();
        if (numBlocks >= 1000000)
          throw std::runtime_error("too many blocks ... consider changing bricksisze or block depth");

        fprintf(out,"# makefile generated by raw2bricks tool\n\n");
        fprintf(out,"all:",outFileName.c_str());
        for (int i=0;i<numBlocks;i++)
          fprintf(out," \\\n\t%s-brick%06i.osp",outFileName.c_str(),i);
        fprintf(out,"\n");

        fprintf(out,"RAW2BRICKS=./ospRaw2Bricks\n\n");

        for (int i=0;i<numBlocks;i++) {
          fprintf(out,"%s-brick%06i.osp: %s.mak\n",outFileName.c_str(),i,outFileName.c_str());
          fprintf(out,"\t${RAW2BRICKS}"
                  " -o %s"
                  " --blockID %i"
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
        // fprintf(out,"\t${RAW2BRICKS}"
        //         " -o %s"
        //         " --blockID -2"
        //         " --format %s"
        //         " --input-format %s"
        //         " --dimensions %i %i %i"
        //         " --brick-size %i"
        //         " --clip-box %i %i %i %i %i %i"
        //           " --depth %i"
        //         " --threshold %f"
        //         " %s"
        //         ,
        //         outFileName.c_str(),
        //         format.c_str(),
        //         (inputFormat!=""?inputFormat.c_str():format.c_str()),
        //         dims.x,dims.y,dims.z,
        //         N,
        //         clipBox.lower.x,clipBox.lower.y,clipBox.lower.z,
        //         clipBox.size().x,clipBox.size().y,clipBox.size().z,
        //         blockDepth,
        //         threshold,
        //         inputFilesString.c_str()
        //         );
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
        cout << "done writing multibrick osp file name..." << endl;
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
        cout << "building block " << blockIdx << " / " << rootGridSize << ", coords = " << blockDims << endl;
        cout << "reading in actual block data" << endl;
        ActualArray3D<T> *blockInput = new ActualArray3D<T>(blockDims.size());
        tbb::parallel_for(0, (int)blockDims.size().z, 1, [&](int iz){
            // for (int iz=0;iz<blockWidth;iz++)
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
// #if 0
//         BrickTreeBuilder<N,T> *builder = new MultiBrickTreeBuilder<N,T>;
//         // MultiBrickTreeBuilder<N,T> *builder = new MultiBrickTreeBuilder<N,T>;
//         PRINT(builder);
//         vec3i encodedSize = builder->getRootGridSize();
//         PRINT(encodedSize);
//         while (encodedSize.x < input->size().x) encodedSize.x *= N;
//         while (encodedSize.y < input->size().y) encodedSize.y *= N;
//         while (encodedSize.z < input->size().z) encodedSize.z *= N;

//         vec3f clipBoxSize = vec3f(input->size()) / vec3f(encodedSize);
//         PRINT(clipBoxSize);

//         // SumFromArrayBuilder<N,T>(input,builder,threshold,rootGridLevel);
//         cout << "done building!" << endl;
//         // progress(builder,input);

//         cout << "saving to output file " << outFileName << endl;

//         builder->save(outFileName,clipBoxSize);
//         cout << "done writing multi-sum tree" << endl;
// #endif
      }
    }


#if 1
    template<int N, typename T>
    void BlockBuilder<N,T>::save(const std::string &ospFileName, const vec3i &validSize)
    {

      const std::string binFileName = ospFileName+"bin";
      FILE *bin = fopen(binFileName.c_str(),"wb");
      
      size_t indexOfs = ftell(bin);
      for (int i=0;i<this->indexBrick.size();i++) {
        const typename BrickTree<N,T>::IndexBrick *brick = this->indexBrick[i];
        if (!fwrite(brick,sizeof(*brick),1,bin))
          throw std::runtime_error("could not write ... disk full!?");
      }
      
      size_t dataOfs = ftell(bin);      
      for (int i=0;i<this->dataBrick.size();i++) {
        const typename BrickTree<N,T>::DataBrick *brick = this->dataBrick[i];
        if (!fwrite(brick,sizeof(*brick),1,bin))
          throw std::runtime_error("could not write ... disk full!?");
      }
      
      size_t indexBrickOfOfs = ftell(bin);      
      for (int i=0;i<this->indexBrickOf.size();i++)
        if (!fwrite(&this->indexBrickOf[i],sizeof(this->indexBrickOf[i]),1,bin))
          throw std::runtime_error("could not write ... disk full!?");
      
      fclose(bin);

      FILE *osp = fopen(ospFileName.c_str(),"w");
      fprintf(osp,"<?xml?>\n");
      fprintf(osp,"<ospray>\n");
      {
        fprintf(osp,"  <BrickTree>\n");
        {
          fprintf(osp,"    averageValue=\"%f\"\n",averageValue);
          fprintf(osp,"    valueRange=\"%f %f\"\n",valueRange.lo,valueRange.hi);
          fprintf(osp,"    format=\"%s\"\n",typeToString<T>());
          fprintf(osp,"    brickSize=\"%i\"\n",N);
          fprintf(osp,"    validSize=\"%i %i %i\"\n",validSize.x,validSize.y,validSize.z);
          fprintf(osp,"    <index num=%li ofs=%li/>\n",
                  this->indexBrick.size(),indexOfs);
          fprintf(osp,"    <data num=%li ofs=%li/>\n",
                  this->dataBrick.size(),dataOfs);
          fprintf(osp,"    <indexBrickOf num=%li ofs=%li/>\n",
                  this->indexBrickOf.size(),indexBrickOfOfs);
        }
        fprintf(osp,"  </BrickTree>\n");
      }
      fprintf(osp,"</ospray>\n");
      fclose(osp);
      // throw std::runtime_error("saving an individual sumerian not implemented yet (use multisum instead)");
    }
#endif


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
        buildIt<N,uint8>(blockID,inputFormat,dims,inFileName,outFileName,clipBox,threshold,blockDepth);
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
        else if (arg == "--depth" || arg == "-depth" || 
                 arg == "--max-levels" || arg == "-ml")
          blockDepth = atoi(av[++i]);
        else if (arg == "--threshold" || arg == "-t")
          threshold = atof(av[++i]);
        else if (arg == "--brick-size" || arg == "-bs")
          brickSize = atof(av[++i]);
        else if (arg == "-bid" || arg == "--blockID")
          blockID = atoi(av[++i]);
        else if (arg == "--format")
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
