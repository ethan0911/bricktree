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

#include "ChomboReader.h"
#include "hdf5.h"

namespace ospray {
  namespace chombo {

    using namespace ospcommon;

    using std::endl;
    using std::cout;

    void parseBoxes(hid_t file, Level *level)
    {
      herr_t status;
      char dataName[1000];
      sprintf(dataName,"level_%i/boxes",level->levelID);
      hid_t data = H5Dopen(file,dataName,H5P_DEFAULT);
      if (data < 0)
        throw std::runtime_error("does not exist");
      hid_t space = H5Dget_space(data);
      const int nDims = H5Sget_simple_extent_ndims(space);
      hsize_t dims[nDims];
      H5Sget_simple_extent_dims(space,dims,NULL);
      int numBoxes = dims[0];
      // PRINT(numBoxes);

      // create compound data type for a box
      hid_t boxType = H5Tcreate (H5T_COMPOUND, sizeof (box3i));
      status = H5Tinsert (boxType, "lo_i", HOFFSET (box3i, lower.x), H5T_NATIVE_INT);
      status = H5Tinsert (boxType, "lo_j", HOFFSET (box3i, lower.y), H5T_NATIVE_INT);
      status = H5Tinsert (boxType, "lo_k", HOFFSET (box3i, lower.z), H5T_NATIVE_INT);
      status = H5Tinsert (boxType, "hi_i", HOFFSET (box3i, upper.x), H5T_NATIVE_INT);
      status = H5Tinsert (boxType, "hi_j", HOFFSET (box3i, upper.y), H5T_NATIVE_INT);
      status = H5Tinsert (boxType, "hi_k", HOFFSET (box3i, upper.z), H5T_NATIVE_INT);

      level->boxes.resize(numBoxes);
      H5Dread(data,boxType,H5S_ALL,H5S_ALL,H5P_DEFAULT,&level->boxes[0]);
      // for (int i=0;i<level->boxes.size();i++)
      //   level->boxes[i].upper += vec3i(1);
      //   PRINT(boxes[i]);
      H5Dclose(data);
    }

    void parseOffsets(hid_t file, Level *level)
    {
      herr_t status;
      char dataName[1000];
      sprintf(dataName,"level_%i/data:offsets=0",level->levelID);
      hid_t data = H5Dopen(file,dataName,H5P_DEFAULT);
      if (data < 0)
        throw std::runtime_error("does not exist");
      hid_t space = H5Dget_space(data);
      const int nDims = H5Sget_simple_extent_ndims(space);
      hsize_t dims[nDims];
      H5Sget_simple_extent_dims(space,dims,NULL);
      int numOffsets = dims[0];
      // PRINT(numOffsets);

      level->offsets.resize(numOffsets);
      H5Dread(data,H5T_NATIVE_INT,H5S_ALL,H5S_ALL,H5P_DEFAULT,&level->offsets[0]);
      // for (int i=0;i<level->offsets.size();i++)
      //   PRINT(level->offsets[i]);
      H5Dclose(data);
    }

    
    void parseData(hid_t file, Level *level)
    {
      herr_t status;
      char dataName[1000];
      sprintf(dataName,"level_%i/data:datatype=0",level->levelID);
      hid_t data = H5Dopen(file,dataName,H5P_DEFAULT);
      hid_t space = H5Dget_space(data);
      const int nDims = H5Sget_simple_extent_ndims(space);
      hsize_t dims[nDims];
      H5Sget_simple_extent_dims(space,dims,NULL);
      int numData = dims[0];
      // PRINT(numData);

      level->data.resize(numData);
      H5Dread(data,H5T_NATIVE_DOUBLE,H5S_ALL,H5S_ALL,H5P_DEFAULT,&level->data[0]);
      H5Dclose(data);
    }

    void parseLevel(hid_t file, Level *level)
    {
      parseBoxes(file, level);
      parseData(file, level);
      parseOffsets(file, level);

      char levelName[1000];
      sprintf(levelName,"level_%i",level->levelID);
      hid_t attr_dt = H5Aopen_by_name(file, levelName, "dt", H5P_DEFAULT,H5P_DEFAULT);
      H5Aread(attr_dt, H5T_NATIVE_DOUBLE, &level->dt);
      H5Aclose(attr_dt);
    }

    Chombo *Chombo::parse(const std::string &fileName)
    {
      Chombo *cd = new Chombo;

      hid_t file = H5Fopen(fileName.c_str(),H5F_ACC_RDONLY,H5P_DEFAULT);
      if (!file) 
        throw std::runtime_error("could not open Chombo HDF file '"+fileName+"'");

      // read components ...
      int numComponents = -1;
      {
        hid_t attr_num_components
          = H5Aopen_by_name(file, "/", "num_components", H5P_DEFAULT,H5P_DEFAULT);
        H5Aread(attr_num_components, H5T_NATIVE_INT, &numComponents);
        H5Aclose(attr_num_components);
      }
      for (int i=0;i<numComponents;i++) {
        {
          char compName[10000];
          sprintf(compName,"component_%i",i);

          hid_t att = H5Aopen_name(file, compName);
          hid_t ftype = H5Aget_type(att);
          hid_t type_class = H5Tget_class (ftype);   
          assert (type_class == H5T_STRING);

          size_t len = H5Tget_size(ftype);
          char comp[len+1];
          comp[len] = 0;
          
          hid_t type = H5Tget_native_type(ftype, H5T_DIR_ASCEND);
          H5Aread(att, type, comp);
          H5Aclose(att);

          cout << "found component '" << comp << "'" << endl;
          cd->component.push_back(comp);
        }
      }
      
      unsigned long long numObjectsInFile;
      H5Gget_num_objs(file, &numObjectsInFile);
      // PRINT(numObjectsInFile);

      for (int objID=0;objID<numObjectsInFile;objID++) {
        const int MAX_NAME_SIZE=1000;
        char name[MAX_NAME_SIZE];
        ssize_t res = H5Gget_objname_by_idx(file, objID, name, MAX_NAME_SIZE);

        if (objID == 0) {
          if (strcmp(name,"Chombo_global"))
            throw std::runtime_error("missing 'Chombo_global' object - apparently this is not a chombo file!?");
          continue;
        }
        int levelID;
        int rc = sscanf(name,"level_%d",&levelID);
        if (rc != 1)
          throw std::runtime_error("could not parse level ID");
        
        Level *level = new Level(levelID);
        parseLevel(file, level);
        cd->level.push_back(level);
      }

      H5Fclose(file);
      return cd;
    }

  }
}
