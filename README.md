# NeuroProof 1.1 [![Picture](https://raw.github.com/janelia-flyem/janelia-flyem.github.com/master/images/gray_janelia_logo.png)](http://janelia.org/)
##Toolkit for Graph-based Image Segmentation and Analysis

The NeuroProof software is an image segmentation tool currently being used
in [the FlyEM project at Janelia Farm Research Campus](http://janelia.org/team-project/fly-em)
to help reconstruct neuronal structure in the fly brain.  This tool
provides routines for efficiently agglomerating an initial volume that
is over-segmented.  It provides several advances over Fly EM previous, but actively maintained tool, [Gala](https://github.com/janelia-flyem/gala):

* Faster implementation of agglomeration written in C++ (instead of Python)
* Incorporation of biological priors (like mitochondria) to improve the quality of the segmentation
(see [PLOS ONE paper](http://journals.plos.org/plosone/article?id=10.1371/journal.pone.0125825))
* Introduction of new metrics for analyzing segmentation quality

While NeuroProof has been tested in the domain
of EM reconstruction, we believe it to be widely applicable to other
application domains.  In addition, to graph agglomeration tools, this
package also provides routines for estimating the uncertainty of a segmentation
[Plaza, et al '12](http://www.plosone.org/article/info%3Adoi%2F10.1371%2Fjournal.pone.0044448)
and algorithms to compare with ground truth.

###Features

* Algorithm for efficiently learning an agglomeration classifier
* Algorithm for efficiently agglomerating an oversegmented volume
* Implementation of variation of information metric for comparing two labeled volumes
* Algorithm to estimate uncertainty of graph and is predicted edge confidences
* Tools to assess the amount of work required to edit/revise a segmentation
* GUI front-end for visualizing segmentation results; interface for merging segments together
* Algorithm for performing focused training (active learning based supervoxel merging classification) using simple GUI
* Simple and access-efficient graph-library implementation and straightforward conversion
to powerful boost graph library
* Data stack implementation that allows one to leverage image processing
algorithms in [Vigra](http://hci.iwr.uni-heidelberg.de/vigra)
* Python bindings to enable accessing the Rag, segmentation routines, and
editing operations in various tool environments like in
[Gala](https://github.com/janelia-flyem/gala) and [Raveler](https://openwiki.janelia.org/wiki/display/flyem/Raveler)


## Installation Instructions

NeuroProof has been on several different linux environments.  Tests on MacOS are forthcoming.
Documentation
in NeuroProof follow Doxygen comment conventions; an html view can be created
by running the following command:

    % doxygen doxygenconfig.file

Neuroproof has several dependencies.  In principle, all of these dependencies
can be built by hand and then the following commands issued:

    % mkdir build; cd build
    % cmake ..
    % make; make install

To simplify the build we now use the [conda-build][2] tool.
The resulting binary is uploaded to the [ilastik binstar channel][3],
and can be installed using the [conda][1] package manager.  The installation
will install all of the neuroproof binaries (including the interactive tool)
and the python libraries.

    [1]: http://conda.pydata.org/
    [2]: http://conda.pydata.org/docs/build.html
    [3]: https://binstar.org/flyem

The NeuroProof dependencies can be found in [Fly EM's conda recipes](https://github.com/janelia-flyem/flyem-build-conda.git).

[buildem](https://github.com/janelia-flyem/buildem) is no longer supported.  Instructions on how to compile/develop NeuroProof without having to worry
about conda are provided below.

### CONDA
The [Miniconda](http://conda.pydata.org/miniconda.html) tool first needs to installed:

```
# Install miniconda to the prefix of your choice, e.g. /my/miniconda

# LINUX:
wget https://repo.continuum.io/miniconda/Miniconda-latest-Linux-x86_64.sh
bash Miniconda-latest-Linux-x86_64.sh

# MAC:
wget https://repo.continuum.io/miniconda/Miniconda-latest-MacOSX-x86_64.sh
bash Miniconda-latest-MacOSX-x86_64.sh

# Activate conda
CONDA_ROOT=`conda info --root`
source ${CONDA_ROOT}/bin/activate root
```
Once conda is in your system path, call the following to install neuroproof:

    % conda create -n CHOOSE_ENV_NAME -c flyem neuroproof

Conda allows builder to create multiple environments.  To use the executables
and libraries, set your PATH to the location of PREFIX/CHOOSE_ENV_NAME/bin.

### Developers' Builder Guide
Developing has never been easier using conda.  If you plan to actively modify
the code, first install neuroproof as discussed above.  Then clone
this repository into the directory of your choosing.  The package cmake
can still be used but the environment variables must be set to point to
the dependencies and libraries stored in PREFIX/CHOOSE_ENV_NAME.  NeuroProof
includes a simple wrapper script 'compile_against_conda.sh' that simply
call cmake with the correct environment variables.  To build NeuroProof:

    % mkdir build; cd build
    % export CONDA_ENV_PATH=PREFIX/CHOOSE_ENV_NAME
    % make -j NUM_PROCESSORS
    % make install

Calling make install will over-write the binaries and libraries in your CHOOSE_ENV_NAME.
Currently, LD_LIBRARY_PATH needs to be set to PREFIX/CHOOSE_ENV_NAME/lib to use
libraries installed this way.

For coding that requires adding new dependencies please consult documentation for
building conda builds and consult Fly EM's conda recipes.

Contributors should verify regressions using 'make test' and submit pull requests
so that the authors can properly update the binstar repository and build system.

## NeuroProof Examples

The top-level programs that are built in NeuroProof are defined
in the <i>src/</i> directory.  NeuroProof's capabilities are mainly 
accessed via command-line executables.  A subset of these capabilities
are exposed in a python interface.  For some examples on how
to run the tool, please consult the 'examples' directory.  If you
have just cloned the NeuroProof repository, you will need to 
add the example submodule.  (The examples are not included by default
due to the size of the directory.)

    % git submodule init
    % git submodule update


## To Be Done

* Update algorithms based on latest Fly EM research
* Add more library/service capabilities (especially for generating metrics)
* Direct support for 2D datasets (2D data is generally allowable in the current implementation)

