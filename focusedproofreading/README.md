# focused proofreading scripts 

This directory contains a few scripts for analyzing uncertain paths in a segmentation.
More information on focused proofreading can be found at the top-level README.
These scripts are not for general use.  The conda NeuroProof build will install the focused proofreading
Python module.  The appropriate conda Python should be in the executable path.

### Inputs needed

* Graph with uncertainty infomration between adjacent segments (from neuroproof_graph_predict or DVIDSparkServices)
* A list of "important" bodies to be proofread (SIZE should be as a percentage of the entire volume)
    
    format: [[BODY1, SIZE], [BODY2, SIZE2], ...] 
* (optional) segmentation to groundtruth map for assessing focused proofreading quality
    
    format: [[SEGBODY, GT], [SEGBODY2, GT2], ....]

## extract_paths.py

This script finds all paths between important bodies within a given certain threshold or path length and
returns a list of these paths.  The uncertainty graph and list of important bodies are required.

Input:

    python extract_paths <uncertainty graph> <important body list>

Output:

    [ [body1, body2, uncertainty] ]

TODO: Print entire path along with locations to examine.


## proofreading_work.py

This script estimates the amount of work to go from an initial segmentation
to ground truth given uncertainty paths determined with the previous script.
This script shows how efficient and effective focused proofreading.

Input:

    python proofreading_work.py <important bodies1> <important bodies2> <paths> <threshold1> <threshold2> <ground truth map>
(currently assumes that there are two classes of 'important bodies')

* paths: refers to the output of the previous script
* threshold1: is a cutoff for what bodies to examine in the important body list 1 (set it high to examine everything)
* threshold2: is a cutoff for "important bodies2"

Output:

    # yes merge, # no merge, # redundant decisions, # test seg, # important GT seg


## smallseg.py

Initial experiments with using proofreading to aid another pass of segmentation.

