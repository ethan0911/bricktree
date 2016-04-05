The OSPRay "AMR" (Adaptive Mesh Refinement) Module
==================================================


Overview
========

This module defines a series of OSPRay volume types and scene graph
plugins to render various AMR type data formats. In particular, it implements

- a class of "BrickTree" data structures that seem particularly
  interesting for representing structured volumes in a compressed format; and

- a "ChomboVolume" class that maintains a list of Chombo-style
  "boxes", where each box is essentially a mini-structured volume that
  lives, with a specified offset, on a specified level



How to use the "ChomboVolume" AMR Type
======================================

Loading and Parameterizing a "ChomboVolume"'s data
--------------------------------------------------

The ChomboVolume is specified through a sequence of "blocks" (or
"boxes"), each of which is a miniature structured grid. Each such
block consists of two parts: a "BrickInfo" struct that specifies its coordinates,
level, etc; and a array of NxMxK floats for that block's data values.

In particular, each BrickInfo looks as follows:

		struct BrickInfo {
      //! bounding box of integer coordinates 
      //! (eg, a 8x8x8 block at origin would have
      //! box3i((0,0,0),(7,7,7)) 
      box3i box;
      //! level this brick is at
      int   level;
      // width of each cell in this level
      float cellWidth;
    }


After creating the chombovolume (through
'ospNewVolume("ChomboVolume")') all this information is passed through
two data arrays:

- the "brickInfo" data array is a OSPData<BrickInfo> (of N entries),
  where N is the number of blocks

- the "brickData" data array is a OSPData<OSPData> (of N
  entires). Each entry in this data array is itself a data array of
  NxMxK floats, where N,M,K are the dimensions of the corresponding
  brick in the brickInfo array.

In addition, this being a volume, there should be a "transferFunction"
specified with the volume.

Using with the QtViewer
-----------------------

The ChomboVolume comes with a scene graph plugin that can load a
Chombo hdf5 file (well, it's only tested for the "black hole
collision" files), and that then renders this as a "ChomboVolume"
ospray object. 

In addition to the scene graph node, this scene graph plugin also
registers this Chombo reader as a default reader for anything that has
a "hdf5" extension. As such, to load a chombo file with the qtviewer,
simply do

			 ./ospQtViewer --module amr MyChomboFile.hdf5 --renderer dvr

Note the "--module amr" (without which the qtviewer will not know what
to do with a hdf5 file).




How to use the BrickTree AMR Types
==================================

Converting a RAW file to OSPRay AMR format
------------------------------------------

Assuming you have properly built the AMR module, you should have a
tool called 'ospRawToAmr' that you can use as follows:

    ./ospRaw2Sum myRawVolume.raw [options]

with the following options:

    -dims <sizeX> <sizeY> <sizeZ>

specifies the dimensions of the input volume (required)

    --format {float|int8|double}

specifies the data format of the input raw file (required)

    -o <filename>

specifies the name of the output ospray AMR file to be generated (required)

    --threshold <t>

specifies the threshold to be define when a group of voxels is
"homogeneous" enough to be represented by a coarser grid. E.g., if we
are using a AMR block size of 8^3 voxels, and the absolute difference
of all voxels with coordinates (i,j,k), i,j,k=(0..8) are less than
this threshold, then the fine grid will NOT contain these voxels, and
the coarse grid will refer to a leaf; if the range of these voxel
values exceeds this threshold, then the coarse grid will refer to a
refined 8^3 gridlet that contains the respective voxel values.

--depth <d>

specifies the depth of the octree's in the multi-octree data
structure. For example., a depth of 0 means the data set is actually
_only_ a root grid (each cell is a leaf); a depth of 3 means octrees
are 3 levels deep, and cover 8x8x8 input cells. I.e., a 1k^3 RAW data
set built with a depth of d=6 would have a root grid of 1k / 2^d =
16^3 root cells of (up to) 6-deep octrees; with a depth of d=10 it
would be a single octree of up to 10 levels.

RAW to AMR conversion example
-----------------------------

As an example, consider the 'magnetic' data set, which is
512^3. Here's how to convert it to a 15^5 root grid with up to 5
levels, and a refinement threshold of 5%:

  ./ospRaw2Sum ~/models/magnetic-512-volume/magnetic-512-volume.raw -dims 512 512 512 --format float -o /tmp/magnetic.osp -t .05 --depth 5


Rendering an AMR Data Set
=========================

Given an ospray AMR data set as produced in the 'RAW to AMR
conversion' section, you should be able to render this via

    ./ospQTViewer --module amr /tmp/magnetic.osp --renderer 
       
