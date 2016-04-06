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

/*! \file sg/Importer.cpp Scene Graph Importer Plugin for HDF5 Chombo files */

#undef NDEBUG

// ospray/sg
#include "sg/volume/Volume.h"
#include "sg/module/Module.h"
#include "sg/importer/Importer.h"
	 
namespace ospray {
  namespace sg {

    using std::endl;
    using std::cout;

    /*! prototype for any scene graph importer function */
    void ChomboHDFImporter(const FileName &fileName,
                           sg::ImportState &importerState);
    
    OSPRAY_SG_DECLARE_MODULE(amr)
    {
      cout << "#osp:sg: loading 'mrbvolume' module (defines sg::MRBVolume type)" << endl;

      /*! declare an importer function for a given file extension */
      declareImporterForFileExtension("hdf5", ChomboHDFImporter);
    }

  }
}

