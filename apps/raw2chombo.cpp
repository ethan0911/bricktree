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
#include "../ospray/ChomboVolume.h"
// tbb
#include "tbb/tbb.h"

namespace ospray {
  namespace amr {

    using std::endl;
    using std::cout;

    bool doParallel = false;
    bool includeBoundary = false;

    std::string toString(long l)
    {
      char c[1000];
      sprintf(c,"%ld",l);
      return c;
    }

    std::string prettyTime(double t)
    {
      assert(t >= 0.f);
      std::string ret = "";
      if (t >= 1.f && t < 1e20f) {
        ret = toString((long)t % 60)+"s"+ret;
        t /= 60.f;

        if (t >= 1.f) {
          ret = toString((long)t % 60)+"m"+ret;
          t /= 60.f;

          if (t >= 1.f) {
            ret = toString((long)t % 24)+"h"+ret;
            t /= 24.f;

            if (t >= 1.f) {
              ret = toString((long)t % 24)+"d"+ret;
            } 
          } 
        } 
        return ret;
      }
      else {
        char c[1000];
        sprintf(c,"%lf",t);
        return c;
      }
    }
    
    struct SafeRange : public Range<float> {
      std::mutex mutex;
      SafeRange() : Range<float>(empty) {};

      void extend(float f) { std::unique_lock<std::mutex> lock(mutex); Range<float>::extend(f); }
      void extend(Range<float> &r) { std::unique_lock<std::mutex> lock(mutex); Range<float>::extend(r); }
    };
    

    struct Raw2Chombo {
      static float threshold;

      Raw2Chombo(int brickSize, 
                 int numLevels, 
                 const Array3D<float> *input, 
                 const std::string &fileName);
      ~Raw2Chombo();

      struct Level {
        std::mutex mutex;
        FILE *file;
      };

      size_t blockSizeOf(int level=0);
      void buildAll();
      vec2f buildBlock(const vec3i &coord, 
                       const int level,
                       SafeRange &range);
      
      const Array3D<float> *input; 
      FILE *osp;
      int brickSize;
      Level *level;
      int numLevels;
      int maxLevel; // for convenience - this is always 'numLevels-1',
                    // but less prone to forget the '-1'
      int cellLevel;

      SafeRange valueRange;

      std::mutex progressMutex;
      std::atomic<size_t> numDone;
      std::atomic<size_t> numBytesWritten;
      int lastPercentagePinged;
      double startTime;
      
      void progress() 
      {
        size_t myNumDone = ++numDone;
        int myPercentage = size_t(myNumDone * 100.f / input->numElements());
        // PRINT(myNumDone);
        if (myPercentage > lastPercentagePinged) {
          std::unique_lock<std::mutex> lock(progressMutex);
          if (myPercentage > lastPercentagePinged) {
            size_t numWritten = numBytesWritten;
            lastPercentagePinged = myPercentage;
            double myTime = getSysTime();
            double timeElapsed = myTime - startTime;
            double timeExpected = timeElapsed / (myPercentage * .01f);
            double timeRemaining = timeExpected - timeElapsed;

            size_t szExpected = numWritten * 100.f / (myPercentage+1e-20f);
            size_t szOriginal = input->numElements()*sizeof(float);
            std::string pt = prettyTime(timeRemaining);
            printf("progress: %03d%% (time remaining %s).",myPercentage,pt.c_str());
            cout << " num written " << prettyNumber(numWritten) << "b";
            cout << " expt size " << prettyNumber(szExpected) << "b";
            cout << " (that's " << (100.f*szExpected/szOriginal) << "% of input)";
            cout << endl;
          }
        }
      }
    };

    float Raw2Chombo::threshold = 0.f;

    size_t Raw2Chombo::blockSizeOf(int level)
    {
      size_t bs = 1;
      for (int i=maxLevel;i>=level;--i)
        bs *= brickSize;
      return bs;
    }

// #if 0
     template<typename TASK_T>
    inline void scalar_for_all(int num, const TASK_T &t)
    {
      for (int i=0;i<num;i++)
        t(i);
    }
// #else
    template<typename TASK_T>
    inline void tbb_for_all(int num, const TASK_T &t)
    {
      tbb::parallel_for(0,num,1,t);
    }

    template<typename TASK_T>
    inline void my_for_all(int num, const TASK_T &t)
    {
      if (doParallel)
        tbb_for_all(num,t);
      else
        scalar_for_all(num,t);
    }
// #endif

    vec2f Raw2Chombo::buildBlock(const vec3i &coord, 
                                 const int level,
                                 SafeRange &range)
    {
      // cout << "Building " << coord << ":" << level << endl;
      if (level == cellLevel /* yes, ">", not "==" - because the individaul cell level is NOT on a brick-level itself */) {
        bool valid = (coord.x < input->size().x && 
                      coord.y < input->size().y && 
                      coord.z < input->size().z) ;
        const float weight = valid ? 1.f : 0.f;
        const float val = input->get(coord);
        range.extend(val);

        if (includeBoundary) {
          for (int iz=-1;iz<=+1;iz++)
            for (int iy=-1;iy<=+1;iy++)
              for (int ix=-1;ix<=+1;ix++)
                range.extend(input->get(coord+vec3i(ix,iy,iz)));
        }

        if (valid) progress();
        return vec2f(val,weight);
      } else {
        std::mutex mutex;
        int bs = brickSize;
        vec2f *brick = new vec2f[bs*bs*bs];

        SafeRange blockRange;
        // for (int cellID=0;cellID<bs*bs*bs;cellID++) {
        scalar_for_all(bs*bs*bs, [&](const int cellID) {
        // tbb::parallel_for(0, bs*bs*bs, 1, [&](const int cellID) {
            const vec3i cellIdx(cellID % bs, (cellID/bs)%bs, cellID/(bs*bs));
            brick[cellID] = buildBlock(bs * coord + cellIdx, level+1, blockRange);
          });
        
        float sum_weighted_vals = 0.f, sum_weights = 0.f;
        for (int i=0;i<bs*bs*bs;i++) {
          sum_weighted_vals += brick[i].y * brick[i].x;
          sum_weights += brick[i].y;
        }
        
        range.extend(blockRange);
        
        if (level == 0 || (blockRange.hi - blockRange.lo) > threshold) {
          // block is important - return it ...
          float block[bs*bs*bs];
          for (int i=0;i<bs*bs*bs;i++)
            block[i] = brick[i].x;

          vec3i brickBegin = coord * vec3i(bs);
          std::unique_lock<std::mutex> lock(this->level[level].mutex);
          fwrite(&brickBegin,sizeof(brickBegin),1,this->level[level].file);
          fwrite(block,sizeof(float),bs*bs*bs,this->level[level].file);
          numBytesWritten += (sizeof(float)*bs*bs*bs+sizeof(brickBegin));
        } else {
          // block isn't important - collapse and don't emit
        }
        
        delete[] brick;

        return vec2f(sum_weighted_vals / (sum_weights+1e-10f), sum_weights);
      }
    }
    
    void Raw2Chombo::buildAll()
    {
      startTime = getSysTime();
      size_t rootBS = blockSizeOf(0);
      vec3i rootDims = divRoundUp(input->size(),vec3i(rootBS));
      size_t numRootBlocks = rootDims.product();
      cout << "building chombo tree, with root grid dims of " << rootDims << endl;
      my_for_all((int)numRootBlocks, [&](int brickID) {
          vec3i rootID;
          rootID.x = brickID % rootDims.x;
          rootID.y = (brickID / rootDims.x) % rootDims.y;
          rootID.z = (brickID / rootDims.x / rootDims.y);
          //          buildOneRootBlock(rootID,0);
          buildBlock(rootID,0,valueRange);
        });
    }
    
    Raw2Chombo::Raw2Chombo(int blockSize, 
                           int numLevels, 
                           const Array3D<float> *input, 
                           const std::string &fileName)
      : input(input),
        brickSize(blockSize),
        numLevels(numLevels),
        maxLevel(numLevels-1),
        cellLevel(numLevels),
        numDone(0),
        numBytesWritten(0),
        lastPercentagePinged(-1)
    {
      level = new Level[numLevels];
      for (int i=0;i<numLevels;i++) {
        char levelFileName[10000];
        sprintf(levelFileName,"%s_level%02d.bin",fileName.c_str(),i);
        level[i].file = fopen(levelFileName,"wb");
      }

      buildAll();

      const std::string ospFileName = fileName+".osp";
      osp = fopen(ospFileName.c_str(),"w");
      fprintf(osp,"<?xml?>\n");
      fprintf(osp,"<ospray>\n");
      fprintf(osp,"  <ChomboVolume\n");
      fprintf(osp,"     numLevels=\"%i\"\n",numLevels);
      fprintf(osp,"     brickSize=\"%i\"\n",blockSize);
      fprintf(osp,"     valueRange=\"%f %f\"\n",valueRange.lo,valueRange.hi);
      fprintf(osp,"     format=\"float\"\n");
      fprintf(osp,"  />\n");
      fprintf(osp,"</ospray>\n");

    }

    Raw2Chombo::~Raw2Chombo()
    {
      for (int i=0;i<numLevels;i++)
        fclose(level[i].file);
      fclose(osp);
    }

    void error(const std::string &msg = "")
    {
      if (msg != "") {
        cout << "*********************************" << endl;
        cout << "Error: " << msg << endl;
        cout << "*********************************" << endl << endl;
      }
      cout << "Usage" << endl;
      cout << "  ./ospRaw2Chombo <inFile.raw> <args>" << endl;
      cout << "with args:" << endl;
      cout << " -dims <nx> <ny> <nz>        : input dimensions" << endl;
      cout << " --format <uint8|float|double> : input voxel format" << endl;
      cout << " --depth|-ml <maxlevels>     : use maxlevels octree refinement levels" << endl;
      cout << " --blockSize|-bs <blockSize> : specify 'refinement' blocksize" << endl;
      cout << " -o <outfilename.xml>        : output file name" << endl;
      cout << " --threshold|-t <threshold>  : threshold of which nodes to split or not (ABSOLUTE float val)" << endl;
      exit(msg != "");
    }

    extern "C" int main(int ac, char **av)
    {
      std::string inFileName  = "";
      std::string outFileName = "";
      std::string format      = "float";
      int         numLevels   = 3;
      int         brickSize   = 4;
      // float       threshold   = 0.f; //.01f;
      vec3i       dims        = vec3i(0);
      vec3i       repeat      = vec3i(0);
      vec3i       shift       = vec3i(0);

      tbb::task_scheduler_init tbb_init(2);

      for (int i=1;i<ac;i++) {
        const std::string arg = av[i];
        if (arg == "-o")
          outFileName = av[++i];
        else if (arg == "--depth" || arg == "-depth" || 
                 arg == "--levels" || arg == "-levels" ||
                 arg == "--num-levels" || arg == "-ml" || arg == "-nl")
          numLevels = atoi(av[++i]);
        else if (arg == "--bs" || arg == "-bs" || 
                 arg == "--brick-size")
          brickSize = atoi(av[++i]);
        else if (arg == "--threshold" || arg == "-t")
          Raw2Chombo::threshold = atof(av[++i]);
        else if (arg == "--repeat" || arg == "-r") {
          repeat.x = atof(av[++i]);
          repeat.y = atof(av[++i]);
          repeat.z = atof(av[++i]);
        } else if (arg == "--format")
          format = av[++i];
        else if (arg == "-dims" || arg == "--dimensions") {
          dims.x = atoi(av[++i]);
          dims.y = atoi(av[++i]);
          dims.z = atoi(av[++i]);
        }
        else if (arg == "-shift" || arg == "--shift") {
          shift.x = atoi(av[++i]);
          shift.y = atoi(av[++i]);
          shift.z = atoi(av[++i]);
        } 
        else if (arg == "-p" || arg == "--parallel") {
          doParallel = true;
        }
        else if (arg == "-b" || arg == "--boundary") {
          includeBoundary = true;
        }
        else if (arg[0] != '-')
          inFileName = av[i];
        else
          error("unknown arg '"+arg+"'");
      }
      
      if (dims == vec3i(0))
        error("no input dimensions (-dims) specified");
      if (inFileName == "")
        error("no input file specified");
      if (outFileName == "")
        error("no output file specified");
      
      
      const Array3D<float> *input = NULL;
      cout << "going to mmap RAW file:" << endl;
      cout << "  input file is   " << inFileName << endl;
      cout << "  expected format " << format << endl;
      cout << "  expected dims   " << dims << endl;
      if (format == "float") {
        input = mmapRAW<float>(inFileName,dims);
      } else if (format == "uint8") {
        const Array3D<uint8> *input_uint8 = mmapRAW<uint8>(inFileName,dims);
        input = new Array3DAccessor<uint8,float>(input_uint8);
      } else if (format == "double") {
        const Array3D<double> *input_double = mmapRAW<double>(inFileName,dims);
        input = new Array3DAccessor<double,float>(input_double);
      } else
        throw std::runtime_error("unsupported format '"+format+"'");
      
      cout << "loading complete." << endl;
      
      if (shift != vec3i(0)) {
        cout << "artifically shifting array by " << shift << endl;
        input = new IndexShiftedArray3D<float>(input,shift);
      }
      if (repeat.x != 0) {
        cout << "artificially blowing up to size " << repeat << endl;
        input = new Array3DRepeater<float>(input,repeat);
      }

      Raw2Chombo builder(brickSize,numLevels,input,outFileName);
      cout << "done building!" << endl;
      return 0;
    }

  }
}
