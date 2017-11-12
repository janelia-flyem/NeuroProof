# NeuroProof [![Picture](https://raw.github.com/janelia-flyem/janelia-flyem.github.com/master/images/HHMI_Janelia_Color_Alternate_180x40.png)](http://www.janelia.org)
##Toolkit for Graph-based Image Segmentation and Analysis

[![Picture](https://anaconda.org/flyem/neuroproof/badges/version.svg)](https://anaconda.org/flyem/neuroproof)
[![Build Status](https://circleci.com/gh/janelia-flyem/NeuroProof.svg?style=shield)](https://circleci.com/gh/janelia-flyem/NeuroProof)

The NeuroProof software is an image segmentation tool currently being used
in [the FlyEM project at Janelia Farm Research Campus](http://janelia.org/team-project/fly-em)
to help reconstruct neuronal structure in the fly brain.  This tool
provides routines for efficiently agglomerating an initial volume that
is over-segmented.  It provides several advances over Fly EM's previous, but actively maintained tool, [Gala](https://github.com/janelia-flyem/gala):

* Faster implementation of agglomeration (written in C++ instead of Python)
* Incorporation of biological priors (like mitochondria) to improve the quality of the segmentation
(see [Parag, et al '15](http://journals.plos.org/plosone/article?id=10.1371/journal.pone.0125825))
* Interactive training of superpixel boundaries without exhaustive groundtruth ([Parag, et.al. 14](http://www.researchgate.net/publication/265683774_Small_Sample_Learning_of_Superpixel_Classifiers_for_EM_Segmentation))
* Introduction of new metrics for analyzing segmentation quality

While NeuroProof has been tested in the domain
of EM reconstruction, we believe it to be widely applicable to other
application domains.  In addition to graph agglomeration tools, this
package also provides routines for estimating the uncertainty of a segmentation
[Plaza, et al '12](http://www.plosone.org/article/info%3Adoi%2F10.1371%2Fjournal.pone.0044448),
algorithms to compare segmentation with ground truth, and focused proofreading algorithms
to accelerate semi-manual EM tracing [Plaza '14](http://arxiv.org/abs/1409.1199).

###Features

* Algorithm for efficiently learning an agglomeration classifier
* Algorithm for efficiently agglomerating an oversegmented volume
* Implementation of variation of information metric for comparing two labeled volumes
* Algorithm to estimate uncertainty in the segmentation graph
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

NeuroProof has been tested on several different linux environments and compiles on Mac.

### Documentation

In NeuroProof, follow Doxygen comment conventions; an html view can be created
by running the following command:

    % doxygen doxygenconfig.file

Neuroproof has several dependencies.  In principle, all of these dependencies
can be built by hand and then the following commands issued:

    % mkdir build; cd build
    % cmake ..
    % make; make install

To simplify the build we now use the [conda-build](http://conda.pydata.org/docs/build.html) tool.
The resulting binary is uploaded to the [flyem binstar channel](https://binstar.org/flyem),
and can be installed using the [conda](http://conda.pydata.org/) package manager.  The installation
will install all of the neuroproof binaries (including the interactive tool)
and the python libraries.

The NeuroProof dependencies can be found in [Fly EM's conda recipes](https://github.com/janelia-flyem/flyem-build-conda.git).

[buildem](https://github.com/janelia-flyem/buildem) is no longer supported.  Instructions on how to compile/develop NeuroProof with minimal interaction with conda are provided below.

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
and libraries, set your PATH to the location of `PREFIX/CHOOSE_ENV_NAME/bin`.

Note: This should work on many distributions of linux and on Mac OSX 10.10+.  For Mac,
you will need to set `DYLD_FALLBACK_LIBRARY_PATH` to `PREFIX/CHOOSE_ENV_NAME/lib`.

Note: For now, the NeuroProof conda package expects certain files to live in `/usr/lib64/`.
On Ubuntu, you may need to run `sudo ln -s /usr/lib/x86_64-linux-gnu /usr/lib64` to use NeuroProof.
(This is currently tracked as issue [#6](https://github.com/janelia-flyem/NeuroProof/issues/6).)

### Developers' Builder Guide
Developing has never been easier using conda.  If you plan to actively modify
the code, first install neuroproof as discussed above.  Then clone
this repository into the directory of your choosing.  The package cmake
can still be used but the environment variables must be set to point to
the dependencies and libraries stored in `PREFIX/CHOOSE_ENV_NAME`.  NeuroProof
includes a simple wrapper script `configure-for-conda.sh` that simply
calls the [build script][build.sh] from NeuroProof's recipe with the appropriate environment variables.

    % ./configure-for-conda.sh /path/to/your/environment-prefix

[build.sh]: https://github.com/janelia-flyem/flyem-build-conda/blob/master/neuroproof/build.sh

That will create a directory named `build` (if necessary) configured to install to your environment prefix.
(If necessary, it will install gcc first.)  To actually run the build, try these commands:

    % cd build 
    % make
    % make install

To use (or test) your custom build of NeuroProof, you'll need to set the `LD_LIBRARY_PATH` and `PYTHONPATH` environment variables:

    % export LD_LIBRARY_PATH=/path/to/your/environment-prefix/lib
    % export PYTHONPATH=/path/to/NeuroProof/build/python
    % make test

For coding that requires adding new dependencies please consult documentation for
building conda builds and consult Fly EM's conda recipes.

Contributors should verify regressions using 'make test' and submit pull requests
so that the authors can properly update the binstar repository and build system.

So, the full build process, from scratch, is this:

```bash
# Set up a conda environment with all dependencies
conda create -n myenv -c flyem neuroproof
source activate myenv
PREFIX=$(conda info --root)/envs/myenv
export LD_LIBRARY_PATH=${PREFIX}/lib # Linux
export DYLD_FALLBACK_LIBRARY_PATH=${PREFIX}/lib # Mac

# Discard the neuroproof downloaded binary; we'll build our own.
conda remove neuroproof

# Clone and build
git clone https://github.com/janelia-flyem/neuroproof
cd neuroproof
./configure-for-conda.sh ${PREFIX}
cd build
make -j4
make install
make test
```

## Focused Proofreading
If one generates edge probability between segments, focused proofreading can be used.
The original focused proofreading approach is implemented in NeuroProof and available as a plugin
within [Raveler](https://openwiki.janelia.org/wiki/display/flyem/Raveler).

Edge probability can be determined using the agglomeration executable neuroproof_graph_predict.  After
agglomerating the segmentation, a final graph is produced which can be used for focused proofreading.
(Other techniques to generate uncertainty will be compatible with neuroproof as long as the format
matches the graph output of agglomeration).

### Raveler

Install Raveler and launch the focused proofreading workflow providing a body importance threshold
and a graph json file produced by neuroproof_graph_predict.  In the current workflow,
Raveler will look for all paths connecting bodies whose risk (impact * uncertainty) is high
enough to warrant examination.  For a given important path, it will navigate to the first edge
in the path, and recompute uncertainty for each merge/no merge decision.  Merge decisions currently
do not result in re-running the edge classifier, rather the highest affinity probability is used
when merging edges.

### neuroproof_graph_analyze_gt

To use focused proofreading outside of Raveler, examine the functions used to evaluate GT in
neuroproof_graph_analyze_gt.  In particular, run_body and run_synapse functions show how to use neuroproof to choose the most important edge and how the thresholds are set.

### static path computation

The easiest way to use focused proofreading is probably the following:

1.  Generate a graph file by running something like neuroproof_graph_predict (or the workflow in DVIDSparkServices)
2.  Choose a set of important bodies in a segmentation (e.g., big bodies or bodies that contain synapse points)
3.  For each important body, use Disjkstra's algorithm to find the closest bodies and consider paths whose endpoints are within the important body set and whose affinity is reasonably high.

Neuroproof has a simple implementation of Dijsktra's algorithm that will work over the neuroproof output.  See example below

    % from neuroproof import FocusedProofreading
    % graph = FocusedProofreading.Graph("graph.json")
    % BODYID = 1
    % PATHCUTOFF = 0
    % THRES = 0.1
    % close_bodies = graph.find_close_bodies(BODYID, PATHCUTOFF, THRES)
    % #This will return something like
    % [2,6,3,0.4]
    
Where this gives a path 1-3-6-2 with an affinity between 1 and 2 of 0.40.  PATHCUTOFF specifies the length of path that will be considered (0 is infinite to within the constraint).  THRES is the minimum acceptable affinity (0.1 or 10% here).

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

