#include "sgBrickTree.h"
#include "ospray/common/OSPCommon.h"

namespace ospray{
  /*! constructor */
  BrickTree::BrickTree()
    : ospVolume(NULL),
    brickSize(-1),
    blockWidth(-1),
    validSize(-1),
    gridSize(-1),
    format("<none>"),
    fileName("<none>"),
    valueRange(one)
  {}; 

  BrickTree::~BrickTree(){
    if (ospVolume != nullptr) {
      ospRelease(ospVolume);
    }
  }

  //! return bounding box of all primitives
  box3f BrickTree::getBounds() 
  {
    const box3f bounds = box3f(vec3f(0.f),vec3f(validSize));
    return bounds;
  }

  void BrickTree::setFromXML(const std::string &fileName)
  {
    std::shared_ptr<xml::XMLDoc> doc = xml::readXML(fileName);
    assert(doc);
    if (!doc) {
      throw std::runtime_error("could not parse " + fileName);
    }
    if (doc->child.empty()) {
      throw std::runtime_error(
          "ospray xml input file does not contain any nodes!?");
    }
    if (doc->child.size() != 1) {
      throw std::runtime_error(
          "not an ospray xml file (no 'ospray' child node)'");
    }
    if ((doc->child[0].name != "ospray" && doc->child[0].name != "OSPRay")) {
      throw std::runtime_error(
          "not an ospray xml file (document root node is '" +
          doc->child[0].name + "', should be 'ospray'");
    }
    auto &root = doc->child[0];
    for (auto &child : root.child) {
      // parse AMR volume
      if (child.name == "MultiBrickTree") {
        this->loadOSP(child);
      } else {
        std::cout << "#osp:amr: skip node " + child.name << std::endl;
      }
    }
  }


  void BrickTree::loadOSP(const ospcommon::xml::Node &node){
    // -------------------------------------------------------
    // parameter parsing
    // -------------------------------------------------------
    format       = node.getProp("format");
    samplingRate = toFloat(node.getProp("samplingRate","1.f").c_str());
    brickSize    = toInt(node.getProp("brickSize").c_str());
    blockWidth   = toInt(node.getProp("blockWidth").c_str());
    gridSize     = toVec3i(node.getProp("gridSize").c_str());
    validSize    = toVec3i(node.getProp("validSize").c_str());
    fileName     = node.doc->fileName;

    assert(gridSize.x > 0);
    assert(gridSize.y > 0);
    assert(gridSize.z > 0);
    PRINT(gridSize);

    // -------------------------------------------------------
    // parameter sanity checking
    // -------------------------------------------------------
    if (format != "float")
      throw std::runtime_error("can only do float BrickTree right now");
    this->voxelType = typeForString(format.c_str());
  }

  void BrickTree::createBtVolume(OSPTransferFunction& tfn){
    if(ospVolume)
      ospVolume = nullptr;

    ospVolume = ospNewVolume("BrickTreeVolume");
    if(!ospVolume)
      throw std::runtime_error("Could not create ospray BrickTreeVolume");
    // -----------------=--------------------------------------
    ospSet1i(ospVolume,"blockWidth",blockWidth);
    ospSet1i(ospVolume,"brickSize",brickSize);
    ospSet3i(ospVolume,"gridSize",gridSize.x,gridSize.y,gridSize.z);
    ospSetString(ospVolume,"fileName",fileName.c_str());
    ospSetString(ospVolume,"format",format.c_str());
    ospSet3i(ospVolume, "validSize",validSize.x,validSize.y,validSize.z);
    ospSetObject(ospVolume,"transferFunction",tfn);
    ospCommit(ospVolume);
  }
}
