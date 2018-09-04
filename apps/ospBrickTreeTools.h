#include "ospray/ospray.h"
#include "ospcommon/xml/XML.h"
#include "../bt/BrickTree.h"
#include "ospcommon/AffineSpace.h"
#include "ospcommon/box.h"
#include "ospcommon/math.h"
#include "ospcommon/memory/malloc.h"
#include "ospcommon/range.h"
#include "ospcommon/vec.h"


namespace ospray{

  using namespace ospcommon;

  struct BrickTree{
    BrickTree();
    ~BrickTree();
    box3f getBounds();
    void setFromXML(const std::string &fileName);
    void loadOSP(const ospcommon::xml::Node &node);
    void createBtVolume(OSPCamera& camera,OSPTransferFunction& tfn);

      //! handle to the ospray volume object
    OSPVolume   ospVolume;
    /*! resolutoin (in BLOCKs) of input grid */
    vec3i gridSize;
    OSPDataType voxelType;
    /*! user-desired sampling rate */
    float samplingRate;
    /*! valid number of voxels in entire data structure
      (gridSize*BLOCKWIDTH may be larger than this! */
    vec3i validSize;
    /*! voxel format; 'float', 'int', etc */
    std::string format;
    /*! the filename for the actual data */
    std::string fileName;
    /*! size (in voxels^3) of each brick */
    int brickSize;
    /*! width (in voxels) of one block in the root grid */
    int blockWidth;
    /*! total range of all values in the tree, so we can set the
      proper xfer fct */
    range_t<float> valueRange;
    /*! whether to use adaptive sampling */
    bool adaptiveSampling;
  };
};//::ospray
