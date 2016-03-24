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
#include "../amr/MultiSumBuilder.h"

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
      cout << " --format <uint8|float|double> : input voxel format" << endl;
      cout << " --depth <maxlevels>    : use maxlevels octree refinement levels" << endl;
      cout << " -o <outfilename.xml>   : output file name" << endl;
      cout << " -t <threshold>         : threshold of which nodes to split or not (ABSOLUTE float val)" << endl;
      exit(msg != "");
    }

    void progress(MultiSumBuilder *builder,
                  const Array3D<float> *input,
                  const vec3i &begin=vec3i(-1), 
                  const vec3i &end=vec3i(-1))
    {
      static size_t numDone = 0;

      size_t numIndexBlocks = 0;
      size_t numDataBlocks = 0;

      assert(builder);
      cout << "done block " << begin << " - " << end
           << " num = " << reduce_mul(end-begin) << endl;
      numDone += reduce_mul((end-begin));
      double pctgDone = 100.f * numDone / double(input->numElements());
      cout << "build update (done w/ " << pctgDone << "% of volume)" << endl;

      if (!builder->getRootGrid()) {
        cout << "(no nodes created, yet)" << endl;
        return;
      }

      for (int iz=0;iz<builder->getRootGrid()->size().z;iz++)
        for (int iy=0;iy<builder->getRootGrid()->size().y;iy++)
          for (int ix=0;ix<builder->getRootGrid()->size().x;ix++) {
            SingleSumBuilder *msb = builder->getRootGrid()->get(vec3i(ix,iy,iz));
            if (msb) {
              numIndexBlocks += msb->indexBlock.size();
              numDataBlocks += msb->dataBlock.size();
            }
          }

      cout << "- root grid size " << builder->getRootGrid()->size() << endl;
      cout << "- total num index blocks " << prettyNumber(numIndexBlocks)
           << " (estd " << prettyNumber(long(numIndexBlocks * 100.f / pctgDone)) << ")" << endl;
      cout << "- total num data blocks " << prettyNumber(numDataBlocks)
           << " (estd " << prettyNumber(long(numDataBlocks * 100.f / pctgDone)) << ")" << endl;

      size_t totalSize
        = numIndexBlocks * sizeof(Sumerian::IndexBlock)
        + numDataBlocks * (sizeof(Sumerian::DataBlock) + sizeof(int));
      
      cout << "- total size (bytes) " << prettyNumber(totalSize)
           << " (estd " << prettyNumber(long(totalSize * 100.f / pctgDone)) << ")" << endl;
      size_t sizeExpected = long(totalSize * 100.f / pctgDone);
      size_t sizeOriginal = input->numElements()*sizeof(float);
      cout << "[that's a compression rate (ASSUMING INPUT WAS FLOATS) of " << (sizeExpected *100.f / sizeOriginal) << "%]" << endl << endl;;
    }

    struct SumFromArrayBuilder {
      
      SumFromArrayBuilder(const Array3D<float> *input, 
                          MultiSumBuilder *builder,
                          float threshold = 0.f,
                          int skipLevels = 0)
        : input(input), 
          threshold(threshold), 
          valueRange(empty), 
          builder(builder),
          skipLevels(skipLevels)
      {
        int rootSize = 4;
        while (rootSize < reduce_max(input->size())) 
          rootSize *= 4;
        PING;
        PRINT(rootSize);
        float avg;
        valueRange = buildRec(avg,vec3i(0),0,rootSize);
      }

      std::vector<Sumerian::IndexBlock> indexBlock;
      std::vector<Sumerian::DataBlock>  dataBlock;
      std::vector<Sumerian::BlockInfo>  blockInfo;

      Range<float> buildRec(float &avg, 
                            const vec3i &begin,
                            int level,
                            int blockSize);

      Range<float> valueRange;
      const Array3D<float> *input;
      const float threshold;
      MultiSumBuilder *builder;
      int skipLevels;
    };

    Range<float> SumFromArrayBuilder::buildRec(float &avg, 
                                               const vec3i &begin,
                                               int level,
                                               int blockSize)
    {
      // static bool alive = 0;
      // static bool debug = 0;
      // if (blockSize == 256 && (begin * blockSize) == vec3i(1024,2560,2816)) 
      //   debug = alive = true; 

      // if (!alive && blockSize <= 256) {
      //   avg = 0.f;
      //   return Range<float>(0.f,0.f);
      // }

      vec3i lo = min(input->size(),begin*blockSize);
      vec3i hi = min(input->size(),lo + vec3i(blockSize));
      if ((hi-lo).product() == 0) { avg = 0.f; return Range<float>(0.f,0.f); }

      bool output = (blockSize == 64)
        // || 
        // (blockSize == 256)
        ;
      if (output) 
        cout << "building block " << lo << " - " << hi << " bs " << blockSize << endl;

      Range<float> range = empty;
      Sumerian::DataBlock db;
      db.clear();

      const int cellSize = blockSize / 4;
        
      if (blockSize == 4) {
        // -------------------------------------------------------
        // LEAF
        // -------------------------------------------------------
        for (int iz=0;iz<4;iz++)
          for (int iy=0;iy<4;iy++)
            for (int ix=0;ix<4;ix++) {
              db.value[iz][iy][ix] = input->get(4*begin+vec3i(ix,iy,iz));
              range.extend(db.value[iz][iy][ix]);
            }
      }
      else {
        // -------------------------------------------------------
        // INNER
        // -------------------------------------------------------
        for (int iz=0;iz<4;iz++)
          for (int iy=0;iy<4;iy++)
            for (int ix=0;ix<4;ix++) {
              range.extend(buildRec(db.value[iz][iy][ix],
                                    4*begin+vec3i(ix,iy,iz),
                                    level+1,
                                    cellSize
                                    ));
            }
      }
      // now, compute average of this node - we need this even if the node gets killed...
      avg = db.computeWeightedAverage(begin,blockSize,input->size());        

      if (((range.hi - range.lo) <= threshold)
          && 
          level > skipLevels) 
        {
          // do not set any fields - kill that block
        } 
      else {
        // need to actually create this block:
        // cout << "inserting block " << begin << ":" << level << endl;
        for (int iz=0;iz<4;iz++)
          for (int iy=0;iy<4;iy++)
            for (int ix=0;ix<4;ix++) {
              const vec3i cellID = 4*begin + vec3i(ix,iy,iz);
              if (cellID.x*cellSize < input->size().x &&
                  cellID.y*cellSize < input->size().y &&
                  cellID.z*cellSize < input->size().z) {
                if (level >= skipLevels) {
                  builder->set(cellID,level-skipLevels,db.value[iz][iy][ix]);
                }
              }
            }
      }
      if (output) {
        progress(builder,input,lo,hi);
      }


      return range;
    }
    
    extern "C" int main(int ac, char **av)
    {
      std::string inFileName  = "";
      std::string outFileName = "";
      std::string format      = "float";
      int         maxLevels   = 3;
      float       threshold   = 0.f; //.01f;
      vec3i       dims        = vec3i(0);
      vec3i       repeat      = vec3i(0);
      vec3i       shift       = vec3i(0);

      for (int i=1;i<ac;i++) {
        const std::string arg = av[i];
        if (arg == "-o")
          outFileName = av[++i];
        else if (arg == "--depth" || arg == "-depth" || 
                 arg == "--max-levels" || arg == "-ml")
          maxLevels = atoi(av[++i]);
        else if (arg == "--threshold" || arg == "-t")
          threshold = atof(av[++i]);
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


      // input = new DummyArray3D<float>(dims);

      // for (int i=0;i<302;i++) {
      //   // PRINT(i);
      //   Range<float> vr = input->getValueRange(vec3i(0),vec3i(i));
      //   // PRINT(vr);
      //   if (vr.lo < vr.hi) {
      //     cout << "first non-empty region at " << vec3i(0) << " - " << vec3i(i) << endl;
      //     break;
      //   }
      // }
      // Range<float> vr = input->getValueRange();
      // PRINT(vr);
      // exit(0);

      cout << "loading complete." << endl;

      if (shift != vec3i(0)) {
        cout << "artifically shifting array by " << shift << endl;
        input = new IndexShiftedArray3D<float>(input,shift);
      }
      if (repeat.x != 0) {
        cout << "artificially blowing up to size " << repeat << endl;
        input = new Array3DRepeater<float>(input,repeat);
      }

      // threshold = 0.f;
      int skipLevels = 0;
      // compute num skiplevels based on num levels specified:
      int levelWidth = reduce_max(input->size());
      for (int i=0;i<maxLevels;i++)
        levelWidth = divRoundUp(levelWidth,4);
      while (levelWidth > 4) {
        skipLevels++;
        levelWidth = divRoundUp(levelWidth,4);
      }
      cout << "building tree with " << skipLevels << " skip levels" << endl;

      MultiSumBuilder *builder = new MultiSumBuilder;
      SumFromArrayBuilder(input,builder,threshold,skipLevels);
      cout << "done building!" << endl;
      progress(builder,input);

      cout << "saving to output file " << outFileName << endl;

      vec3i encodedSize = builder->getRootGrid()->size();
      while (encodedSize.x < input->size().x) encodedSize.x *= 4;
      while (encodedSize.y < input->size().y) encodedSize.y *= 4;
      while (encodedSize.z < input->size().z) encodedSize.z *= 4;

      vec3f clipBoxSize = vec3f(input->size()) / vec3f(encodedSize);
      
      builder->save(outFileName,clipBoxSize);
      cout << "done writing multi-sum tree" << endl;
      return 0;
    }

  }
}
