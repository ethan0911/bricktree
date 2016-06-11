The OSPRay "AMR" (Adaptive Mesh Refinement) Module
==================================================


Overview
========

This module defines a "bricktree" data structure that builds a
multi-resolution hierarchy that can represent a MxLxK structured
volume of type T in a hierarchical, multiresolution, and possibly
compressed way.

In particular, the tree consists of a series of "bricks" of NxNxN
cells; each contains one value and *can* refer to a child brick (which
itself will be a NxNxN brick, etc). Bricks therefore form a natural
hierarchy of branching factor NxNxN.

Each cell in a brick corresponds to a region of voxels in the logical
structured volume space that the bricktree corresponds. For leaf
bricks each cell typically corresponds to exactly on logical
structured voluem cell; for inner nodes typically refer to NxNxN input
cells on the first inner bricktree level, to N^2xN^2xN^2 cells on the
second-mode inner level, etc. 

For inner nodes, the value stored in the brick cells should be the
same as the value of corresponding cell of the corresponding
structured volume that our bricktree represents; for inner nodes the
value stored in a cell is the average of all its child cells.

Each cell can - but does not have to - refer to a child brick.  These
child indices are encoded through two indirections: each brick can
(but does not have to) have a index brick (if it doesn't have an index
brick, then none of the brick's cells have children); if it does have
an index brick then this index brick stores one child index for each
brick cell (if a given cell does not have children then its
corresponding child index will be marked as invalid).

The brick tree is specified through the branching factor N, the data
type T, and a tree depth of T. A full bricktree of depth D is called a
"block", and it represents (by definition) a range of N^D x N^D x N^D
cells. If one block is not sufficient to represent the whole input
volume (ie, if M,L,K > N^D) then we generate as many blocks as
required to fully tile the MxLxK blocks.

Data Structure / Data Layout
============================

Each block of a BrickTree<N,T> is internally stored as three linear
arrays:

- one linear array of "value bricks", where each value brick contains
  NxNxN values of type T.
  
- one linear array of int32 "index brick IDs", with exactly one such
  int32 per value brick. If a given value brick's index brick ID is
  negative, it doesn't have any children, otherwise this ID refers to
  an index brick in the index brick array.
  
- one linear array of index bricks, where each index brick contains
  NxNxN int32 indices. Each such index can be -1 (meaning the
  corresponding cell does not have a child); if >= 0 it refers to a
  brick in the value brick array (which in turn can have children
  referred to _it's_ index brick ID, etc).

- Each bricktree block is stored in two files: a ".osp" file that
  given high-level info re N, T, etc in XML form; and one ".ospbin"
  file that contains the three arrays (value brick array, index brick
  ID array, and index brick array) in binary form.

- the whole volume is represented to one .osp file that contains index
  info (N,T, (M,L,K) of input volume, etc)

Building a BrickTree
====================

To build a bricktree, use the "ospRawToBricks" tool. For example, to
build a bricktree with N=4, T=float, and depth=4 (ie, 256^3 blocks)
for the 512^3 float magnetic data set, without any sort of data
conversion or compression, use the following command:

	./ospRaw2Bricks \
		--format float -dims 512 512 512 magnetic.raw \
		-bs 4 --depth 4 -t 0 -o magnetic-bt
	make -f magnetic-bt.mak
	
The first call to 'ospRaw2Bricks' geneartes the index file as well as
a makefile that will then build each brick. If the build process gets
interrupted for any reason, it *should* be sufficient to just re-run
the 'make' command; obviously 'make -j 4' etc should work, but using
too many parallel build processes may overload the file system.

Compression
-----------

The builder automatically supports compression by letting the user
specify a threshold (-t <threshold>) that specifies which input
regions can be safely collapsed. Ie, if an entire input region's that
a cell would refer to all varies by less than this threshold value
then the cell will automatically become a leaf cell, and children will
not be generated for this cell. 

The default value for threshold is 0, meaning that it _does_ eliminate
equal-value regions, but only in a loss-less way.

Other options
-------------

The builder also contains ways of specifying a clip-region,
artificially replicating/repeating the input, converting from an input
format to an interval format, using multiple input slices, etc. Please
refer to the help output of the ospRaw2Bricks tool for more details.
	

Rendering a BrickTree
=====================

Is not yet implemented. SOme of the boilerplate code for loading scne
graph nodes is alreay available, but does not do anything yet.


