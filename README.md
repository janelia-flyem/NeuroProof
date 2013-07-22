# NeuroProof 1.1 -- Toolkit for Graph-based Image Segmentation and Analysis

The NeuroProof software is an image segmentation tool currently being used
in [the FlyEM project at Janelia Farm Research Campus](http://janelia.org/team-project/fly-em)
to help reconstruct neuronal structure in the fly brain.  This tool
provides routines for efficiently agglomerating an initial volume that
is over-segmented.  While NeuroProof has been tested in the domain
of EM reconstruction, we believe it to be widely applicable to other
application domains.  In addition, to graph agglomeration tools, this
package also provides routines for estimating the uncertainty of a segmentation
[Plaza, et al '12](http://www.plosone.org/article/info%3Adoi%2F10.1371%2Fjournal.pone.0044448)
and mechanisms to comparing with ground truth.  This package shares many
ideas from [Gala](https://github.com/janelia-flyem/gala).

###Features

* Algorithm for efficiently learning an agglomeration classifier
* Algorithm for efficiently agglomerating an oversegmented volume
* Implementation of variation of information metric for comparing two labeled volumes
* Algorithm to estimate uncertainty of graph and is predicted edge confidences
* Tools to assess the amount of work required to edit/revise a segmentation
* Simple and access-efficient graph-library implementation and straightforward conversion
to powerful boost graph library
* Data stack implementation that allows one leverage image processing
algorithms in [Vigra](http://hci.iwr.uni-heidelberg.de/vigra)
* Python bindings to enable accessing the Rag, segmentation routines, and
editing operations in various tool environments like in Gala and [Raveler](https://openwiki.janelia.org/wiki/display/flyem/Raveler)


## Installation Instructions

NeuroProof is currently only supported on linux operating systems.  It probably
would be relatively straightforward to build on a MacOS.  NeuroProof
supports two mechanisms for installation.

### Buildem

The most-supported and most push-button approach involves using the build system
called [buildem](https://github.com/janelia-flyem/buildem).  The advantage
of using buildem is that it will automatically build all of the dependencies
and put it in a separate environment so as not to conflict with any other
builds.  These automatic builds contain versions of the dependencies that
are know to work with NeuroProof.  The disadvantage of buildem is that it will
install a lot of package dependencies.  The initial build of all of the dependencies
could take on the order of an hour to complete.

To build NeuroProof using buildem

    % mkdir build; cd build;
    % cmake .. -DBUILDEM_DIR=/user-defined/path/to/build/directory
    % make -j num_processors

This will automatically load the binaries into BUILDEM_DIR/bin.  To run
the test regressions, add this directory into your PATH environment and
run <i>make test</i> in the <i>build</i> directory.

### NeuroProof Only

NeuroProof can be built without NeuroProof with the following:

    % mkdir build; cd build;
    % cmake ..
    % make

To successfully compile this code, you will need to install the following
dependencies and make sure their headers and libraries can be found in
the common linux search paths:

* jsoncpp
* boost
* vigra
* hdf5
* opencv

For more information on the supported versions of these dependencies, please
consult buildem.

To run <i>make test</i>, the installed libraries will need to be in the default
library search path or set in LD_LIBRARY_PATH.  The PYTHONPATH environment
variable will need to be set to the lib directory create within NeuroProof.  The
binaries produced will be placed in the NeuroProof bin directory.

## NeuroProof Examples

The top-level programs that are built in NeuroProof are defined
in the src/ directory.  NeuroProof's capabilities are mainly 
accessed via command-line executables.  A subset of these capabilities
are exposed in a python interface.  For some examples on how
to run the tool, please consult the 'examples' directory.  If you
have just cloned the NeuroProof repository, you will need to 
add the example submodule.  (The examples are not included by default
due to the size of the directory.)

    % git submodule init
    % git submodule update


## To Be Implemented

* A simple visualization client for analyzing segmentation
* Add capability to call NeuroProof as a service in clustered environments
* Direct support for 2D datasets (2D data is generally allowable in the current implementation)
* Improved algorithms for handling EM reconstruction specific goals


