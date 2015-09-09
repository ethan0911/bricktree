The OSPRay "AMR" (Adaptive Mesh Refinement) Module
==================================================


How to use the AMR Module
=========================

Converting a RAW file to OSPRay AMR format
------------------------------------------

Assuming you have properly built the AMR module, you should have a
tool called 'ospRawToAmr' that you can use as follows:

    ./ospRawToAmr myRawVolume.raw [options]

with the following options:

    -dims <sizeX> <sizeY> <sizeZ>

specifies the dimensions of the input volume (required)

    -format {float|int8}

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

--brick-size <numLevels> <brickSize0> <brickSize1> ... <brickSize(N-1)>

specifies the resolution of the respective AMR levels. NumLevels
specifies how many levels there will be; then each following value
specifies the resolution of the respective level as follows.

RAW to AMR conversion example
-----------------------------

For example, consider you have a volume test.raw of 1024^3 floating
point voxels, and you want to convert this to an AMR representation
using 8^3 blocks. Further assume that all voxel values are in the
[0..1] range, and you want to use a threshold of 0.1. In that case,
you'd be using the command

    ospRawToAmr test.raw -dims 1024 1024 1024 -format float \
         -o test_8.osp --threshold .1 --brick-size 1 8

Given that the fine grid has 1024^3 voxels (that is 1023^3 cells),
using bricks of 8^3 cells (that is 9^3 voxels) the output grid will
have 129^3 voxels (128^3 cells, because 128=roundup(1023/8)), and each
of these cells will be either a leaf (if its corresponding 9^3 voxels
in the fine grid didn't vary by more than 0.1, or point to a brick of
9^3 voxels that contain the original cells. The voxel values at the
coarse grid will correspond to the voxel values at the corners of the
fine grid's cells, i.e., at (0,0,0), (0,0,8), (0,0,16), ... etc.

Since the number of input _cells_ (1023) is not an exact multiple of
the brick size the tool will automatically pad the fine grid by the
voxel values from the last valid cell. I.e., for the voxel (0,0,1024)
required for the right-most bricks and coarse-grid voxel values, it
will simply use the closest valid voxel value, at (0,0,1023).


Rendering an AMR Data Set
=========================

Given an ospray AMR data set as produced in the 'RAW to AMR
conversion' section, you should be able to render this via

    ./ospQTViewer --module amr test_8.osp --renderer raycast_volume_renderer
       
NOTE: SO FAR THIS IS AMR MODULE IS NOT FULLY IMPLEMENTED, SO THIS WILL
NOT RENDER ANYTHING AT ALL RIGHT NOW.
