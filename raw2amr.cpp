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

    void usage(const std::string &err)
    {
      if (err != "")
        cout << "Error: " << err << endl << endl;
      cout << "./ospRAW2AMR inFileName.raw <args>*" << endl << endl;
      cout << endl << "Args:" << endl;
      cout << "-dims sizeX sizeY sizeZ" << endl;
      cout << "-format {float|int8}" << endl;
      cout << "-o outFile.osp" << endl;
      cout << "--threshold t : specify threshold of when to merge" << endl;
      cout << "--brick-size|-bs <numLevels> <brickSize0> <brickSize1> ..." << endl;
      cout << endl;
      exit(0);
    }

    inline vec3i divUp(const vec3i &a, const vec3i &b)
    {
      return vec3i(divRoundUp(a.x,b.x));
    }

    template<typename T> 
    typename AMR<T>::Grid *buildRec(AMR<T> *amr,
                                    Array3D<T> &input,
                                    const vec3i &begin,
                                    const vec3i &end,
                                    const std::vector<int> &brickSize,
                                    float threshold)
    {
      typedef typename AMR<T>::Grid Grid;
      typename AMR<T>::Grid *grid = new typename AMR<T>::Grid;

      size_t skip = 1;
      for (int i=0;i<brickSize.size();i++)
        skip *= brickSize[i];

      grid->cellDims = divUp(end-begin-vec3i(1),vec3i(skip));
      grid->voxelDims = grid->cellDims + vec3i(1);
      grid->numCells  = size_t(grid->cellDims.x) *size_t(grid->cellDims.y) *size_t(grid->cellDims.z);
      grid->numVoxels = size_t(grid->voxelDims.x)*size_t(grid->voxelDims.y)*size_t(grid->voxelDims.z);
      grid->cell  = new typename Grid::Cell[grid->numCells];
      grid->voxel = new T[grid->numVoxels];

      {
        size_t idx = 0;
        for (size_t iz=0;iz<grid->voxelDims.z;iz++)
          for (size_t iy=0;iy<grid->voxelDims.y;iy++)
            for (size_t ix=0;ix<grid->voxelDims.x;ix++) {
              grid->voxel[idx++] = input.getSafe(vec3i(ix,iy,iz)*vec3i(skip));
            }
        assert(idx == grid->numVoxels);
      }

      std::vector<int> childBrickSize;
      for (int j=1;j<brickSize.size();j++)
        childBrickSize.push_back(brickSize[j]);

      {
        size_t idx = 0;
        for (size_t iz=0;iz<grid->cellDims.z;iz++)
          for (size_t iy=0;iy<grid->cellDims.y;iy++)
            for (size_t ix=0;ix<grid->cellDims.x;ix++, idx++) {
              // bool needChild = false;
              // if (childBrickSize.empty()) {
              //   grid->cell[idx].childID = -1;
              // } else
              {
                const vec3i cellBegin = vec3i(ix,iy,iz)*vec3i(skip);
                grid->cell[idx].lo = input.getSafe(cellBegin);
                grid->cell[idx].hi = input.getSafe(cellBegin);
                for (int iiz=0;iiz<=skip;iiz++)
                  for (int iiy=0;iiy<=skip;iiy++)
                    for (int iix=0;iix<=skip;iix++) {
                      const T v = input.getSafe(vec3i(ix,iy,iz)*vec3i(skip)+vec3i(iix,iiy,iiz));
                      grid->cell[idx].lo = std::min(grid->cell[idx].lo,v);
                      grid->cell[idx].hi = std::max(grid->cell[idx].hi,v);
                    }
                if ((grid->cell[idx].hi - grid->cell[idx].lo) >= threshold) {
                  Grid *cellGrid = buildRec(amr,input,
                                            cellBegin,min(cellBegin+vec3i(skip),end),
                                            childBrickSize,threshold);
                  if (cellGrid) {
                    grid->cell[idx].childID = amr->gridVec.size();
                    amr->gridVec.push_back(cellGrid);
                  } else
                    grid->cell[idx].childID = -1;
                } else {
                  grid->cell[idx].childID = -1;
                }
              }
            }
        assert(idx == grid->numCells);
      }

      return grid;
    }

    template<typename T> 
    void raw2amr(const std::string &inFileName,
                 const vec3i &dims,
                 const std::string &outFileName,
                 const std::vector<int> &brickSize,
                 float threshold)
    {
      Array3D<T> input(dims);
      loadRAW<T>(input,inFileName,dims);

      AMR<T> *amr = new AMR<T>;
      amr->rootGrid = buildRec(amr,input,vec3i(0),dims,brickSize,threshold);
      amr->gridVec.push_back(amr->rootGrid);
      PRINT(amr->gridVec.size());
      PRINT(amr->rootGrid->cellDims);
      PRINT(amr->rootGrid->voxelDims);

      amr->writeTo(outFileName);
    }

    void raw2amrMain(int &ac, char **&av)
    {
      std::string inFileName = "";
      std::string outFileName = "";
      std::string format = "int8";
      vec3i dims(0);
      std::vector<int> brickSize;
      float threshold = 8.f;
      
      for (int i=1;i<ac;i++) {
        const std::string arg = av[i];
        if (arg[0] == '-') {
          if (arg == "-format" || arg == "--format") {
            format = av[++i];
          } else if (arg == "-o") {
            outFileName = av[++i];
          } else if (arg == "-t" || arg == "-threshold" || arg == "--threshold") {
            threshold = atof(av[++i]);
          } else if (arg == "-dims" || arg == "--dimensions") {
            dims.x = atoi(av[++i]);
            dims.y = atoi(av[++i]);
            dims.z = atoi(av[++i]);
          } else if (arg == "-bs" || arg == "--brick-size") {
            assert(i+1 < ac);
            int numLevels = atoi(av[++i]);
            for (int j=0;j<numLevels;j++) {
              assert(i+1 < ac);
              brickSize.push_back(atoi(av[++i]));
            }
          } else {
            usage("unknown parameter '"+arg+"'");
          }
        } else {
          if (inFileName != "") usage("input specified more than once!?");
          inFileName = av[i];
        }
      }
      
      if (threshold <= 0.f) 
        usage("no merge threshold specified");
      if (inFileName == "") 
        usage("no input file specified");
      if (outFileName == "") 
        usage("no output file specified");
      if (dims.x == 0) 
        usage("no input size specified");

      // =======================================================
      // do the actual work
      // =======================================================
      if (format == "int8")
        raw2amr<uint8>(inFileName,dims,outFileName,brickSize,threshold);
      else if (format == "float")
        raw2amr<float>(inFileName,dims,outFileName,brickSize,threshold);
      else 
        usage("invalid format '"+format+"' - must be int8 or float");
    }

  }
}

int main(int ac, char **av)
{
  ospray::amr::raw2amrMain(ac,av);
  return 0;
}
