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

#pragma once

// ospray
#include "Array3D.h"
// std
#include <iostream>

namespace ospray {
  namespace amr {

    template<typename T>
    const char *typeToString();

    struct BrickTreeBase {
      virtual std::string voxelType() const = 0;
      virtual int         brickSize() const = 0;
      virtual const void *brickInfoPtr()  const = 0;
      virtual const void *valueBrickPtr()  const = 0;
      virtual const void *indexBrickPtr() const = 0;
      virtual vec3i getRootGridDims() const = 0;
      virtual const int *firstIndexBrickOfTreePtr() const = 0;
      virtual const int *firstValueBrickOfTreePtr() const = 0;

      static BrickTreeBase *mapFrom(const void *ptr, 
                                    const std::string &format, 
                                    int BS, 
                                    size_t superBlockOfs);
      // virtual void saveTo(FILE *bin, size_t &superBlockOfs) = 0;

      size_t numValueBricks;
      size_t numIndexBricks;
      size_t numBrickInfos;
      vec3i rootGridDims;
      vec3f validFractionOfRootGrid;
    };

    /*! the superblock we'll store at the end of the binary
        memory/file region for a binary bricktree */
    struct BinaryFileSuperBlock {
      size_t valueBricksOfs;
      size_t indexBricksOfs;
      size_t childBricksOfs;
    };

    /* hierarchical tree of bricks. if N is the template parameter,
       this tree will encode bricks of NxNxN cells (a so-called value
       brick). child pointers are encoded in 'index bricks', with each
       value brick possibly having one associated index brick. For each
       value brick, we store as 'indexBrickOf' value (in a separate
       array); if this value is 'invalidID' the given value brick is a
       complete leaf brick that doesn't ahve an index brick (and thus,
       no children at all); otherwise this ID refers to a index brick
       in the index brick array, with the index brick having exactly
       one child index value (referring to value brick IDs) for each
       cell (though it is possible that some (or even, many) cells do
       not have a child at all, in which the corresponding child index
       in the index brick is 'invalidID'  */
    template<int N, typename T=float>
    struct BrickTree : public BrickTreeBase {
      static inline int invalidID() { return -1; }

      virtual std::string voxelType() const override { return typeToString<T>(); };
      virtual int         brickSize() const override { return N; };
      
      /*! 4x4x4 brick/brick of value. */
      struct ValueBrick {
        void clear();
        // Range<float> getValueRange() const;
        double computeWeightedAverage(// coordinates of lower-left-front
                                      // voxel, in resp level
                                      const vec3i &brickCoord,
                                      // size of bricks in current level
                                      const int brickSize,
                                      // maximum size in finest level
                                      const vec3i &maxSize) const;

        T value[N][N][N];
      };
      
      /*! gives the _value_ brick ID of NxNxN children. if a
        nodes does NOT have a child, it will use ID "invalidID". if NO
        childID is valid, the brick shouldn't exist in the first place
        ... (see BrickInfo description) */
      struct IndexBrick {
        void clear(); 
        int32 childID[N][N][N];
      };
      struct BrickInfo {
        BrickInfo(int32 ID=invalidID()) : indexBrickID(ID) {};
        
        /*! gives the ID of the index brick that encodes the children
          for the current brick. if the current brick doesn't HAVE any
          children, this will be (int32)-1 */
        int32 indexBrickID; 
      }; 

      struct Builder {
        virtual void set(const vec3i &coord, int level, float v) = 0;

        /*! be done with the build, and save all value to the xml/bin
            file of 'fileName' and 'filename+"bin"'. clipboxsize
            specifies which fraction of the encoded domain si actually
            useful. (1,1,1) means that everything is value (as in a
            native AMR value set), but encoding a 302^3 RAW volume into
            something that can, by definiion, only encode powers of N,
            woudl result i na clip box size of ~vec3f(0.3) (=302/1024)
            : the tree itself encodes voxels up to 1024^3, but only
            that fraction of them iscatually valid. */
        virtual void save(const std::string &ospFileName, const vec3f &clipBoxSize) = 0;
      };

      void mapFrom(const unsigned char *ptr, 
                   const vec3i &rootGrid,
                   const vec3f &fracOfRootGrid,
                   std::vector<int32_t> numValueBricksPerTree,
                   std::vector<int32_t> numIndexBricksPerTree);

      virtual const void *brickInfoPtr()  const override { return brickInfo; }
      virtual const void *valueBrickPtr()  const override { return valueBrick; }
      virtual const void *indexBrickPtr() const override { return indexBrick; }

      const ValueBrick *valueBrick;
      const IndexBrick *indexBrick;
      const BrickInfo *brickInfo;

      // virtual void saveTo(FILE *bin, size_t &superBlockOfs) override;

      virtual vec3i getRootGridDims() const override { return rootGridDims; }

      /* gives, for each root cell / tree in the root grid, the ID of
         the first index brick in the (shared) value brick array */
      const int32_t *firstIndexBrickOfTree;

      virtual const int *firstIndexBrickOfTreePtr() const override { return firstIndexBrickOfTree; } ;
      virtual const int *firstValueBrickOfTreePtr() const override { return firstValueBrickOfTree; } ;

      /* gives, for each root cell / tree in the root grid, the ID of
         the first value brick in the (shared) value brick array */
      const int32_t *firstValueBrickOfTree;
    };
    
    // =======================================================
    // INLINE IMPLEMENTATION SECTION 
    // =======================================================
    
    /*! print a value brick */
    template<int N, typename T>
    inline std::ostream &operator<<(std::ostream &o, 
                                    const typename BrickTree<N,T>::ValueBrick &db)
    {      
      for (int iz=0;iz<N;iz++) {
        for (int iy=0;iy<N;iy++) {
          if (N > 2) std::cout << std::endl;
          for (int ix=0;ix<N;ix++)
            o << db.value[iz][iy][ix] << " ";
          o << "| ";
        }
        if (iz < 3) o << std::endl;
      }
        
      return o;
    }
    
    /*! print an index brick */
    template<int N, typename T>
    inline std::ostream &operator<<(std::ostream &o, 
                                    const typename BrickTree<N,T>::IndexBrick &db)
    {      
      for (int iz=0;iz<N;iz++) {
        for (int iy=0;iy<N;iy++) {
          if (N > 2) std::cout << std::endl;
          for (int ix=0;ix<N;ix++)
            o << db.childID[iz][iy][ix] << " ";
          o << "| ";
        }
        if (iz < 3) o << std::endl;
      }
        
      return o;
    }

  } // ::ospray::amr
} // ::ospray



