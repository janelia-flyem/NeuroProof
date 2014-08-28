/*!
 * This file defines the interface to the main set of algorithms for the
 * StackBase model.  More general functionality that involves manipulating
 * the Stack model is placed here.  There is a derived controller called
 * BioStackController that implements more specialized operations in
 * addition to the ones provided here.  For now, functionality such as
 * agglomeration algorithms (found in BioPriors/StackAgglomAlgs.h) is
 * implemented as separate standalone functions rather than being wrapped
 * into this class.  This was done to logically separate such a specific
 * set of actions into a different file, as well as, reduce the dependencies
 * needed to compile the StackController.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef STACK_H
#define STACK_H

#include "StackBase.h"

// used to represent x,y,z locations
#include <boost/tuple/tuple.hpp>

#include <tr1/unordered_set>
#include <tr1/unordered_map>
#include <map>
#include <string>

// json format is used to represent the serialized rag 
#include <json/json.h>
#include <json/value.h>

namespace NeuroProof {

// forward declare algorithm class for combining rag nodes
class RagNodeCombineAlg;

/*!
 * Class that contains functionality for manipulating and analyzing
 * the Stack model. 
*/
class Stack : public StackBase {
  public:
    /*!
     * Constructor that keeps a pointer to the main segmentation stack
     * \param stack_ Stack
    */
    Stack(VolumeLabelPtr labels_) : StackBase(labels_) {}


    /*!
     * Constructor that initializes from a stack h5 file
     * \param stack_name name of stack h5 file
    */
    Stack(std::string stack_name);

    /*!
     * Constructs a RAG by interating through the 3D label volume and looking
     * at voxels 6 neighbors.  While building a RAG, features are constructed
     * from the probability volumes in the stack.
    */
    virtual void build_rag();

    /*!
     * Constructs a RAG by interating through the 3D label volume and looking
     * at voxels 6 neighbors.  This command is expected to operate on only a subset
     * of a much larger 3D label volume.  Assumes 1 pixel border with larger volume. 
    */
    void build_rag_batch();

    /*!
     * Finds bi-connected components in the RAG (that are not connected to the
     * boundary of the volume) and removes them.  This function modifies the
     * RAG and reassigns labels in the label volume.
     * \return number of inclusions removed
    */
    int remove_inclusions();
    
    /*!
     * Moves label_remove to label_keep by reassigning the label and modifying
     * the rag as appropriate (if ignore_rag is false)
     * \param label_remove label to be removed
     * \param label_keep label to be kept
     * \param combine_alg rag merging algorithm
     * \param ignore_rag determines whether to modify rag
    */
    void merge_labels(Label_t label_remove, Label_t label_keep,
            RagNodeCombineAlg* combine_alg, bool ignore_rag = false);

    /*!
     * Algorithms to absorb small regions by first removing them and then
     * calling a seeded watershed to fill in this empty regions.  The size
     * of bodies removed are determined by a size threshold and a hash of
     * label exclusions can prevent certain regions from being removed.
     * \param boundary_pred probability volume corresponding to boundary
     * \param threshold size below which labels are removed
     * \param exclusions hash of of labels to not be removed
     * \return number of regions absorbed
    */
    int absorb_small_regions(VolumeProbPtr boundary_pred, int threshold,
                    std::tr1::unordered_set<Label_t>& exclusions);
    
    /*!
     * Similar to absorb_small_regions except removed regions are assigned a 0
     * label value and no watershed is performed.
     * \param threshold size below which labels are removed
     * \param exclusions hash of of labels to not be removed
     * \return number of regions removed
    */
    int remove_small_regions(int threshold,
                    std::tr1::unordered_set<Label_t>& exclusions);

    /*!
     * Determines the ground truth label(s) from a list of a set of
     * candidate regions that overlap significantly with a given label
     * from the stack label volume.
     * \param label label to find match in ground truth label volume
     * \param candidate_regions ground truth labels considered for overlap
     * \param gt_rag rag corresponding to ground truth
     * \param labels_matched reference to set of labels that are matched
     * \param gtlabels_matched reference to set of gt labels matching labels
     * \return number of matches found to the given label
    */
    int match_regions_overlap(Label_t label, std::tr1::unordered_set<Label_t>& candidate_regions,
        RagPtr gt_rag, std::tr1::unordered_set<Label_t>& labels_matched,
        std::tr1::unordered_set<Label_t>& gtlabels_matched);
   

    /*!
     * Find labels that belong to ground truth body.
    */
    void get_gt2segs_map(RagPtr gt_rag,
            std::tr1::unordered_map<Label_t, std::vector<Label_t> >& gt2segs);

    /*!
     * Creates a set of labels from a json file and zeros out these labels
     * in the label volume.
     * \param exclusions_json name of json file with exclusion labels
    */
    void set_body_exclusions(std::string exclusions_json);
    
    /*!
     * Dilates the borders between labels using the given disc size.
     * A disc size of 1 will just create a 0 border using each voxels
     * 6-voxel neighborhood.  A larger disc size will result in calling
     * an edge dilation algorithm from Vigra.
     * \param disc_size determines size of the border edge
    */
    void dilate_labelvol(int disc_size);
    
    /*!
     * Same as 'dilate_labelvol' but on the ground truth label volume.
     * \param disc_size determines size of the border edge
    */
    void dilate_gt_labelvol(int disc_size);

    /*!
     * Compute the variation of information (VI) similarity metric between
     * the ground truth and label volume.  This computation requires
     * the linear traversal of the label volume to find the region overlap
     * between the volumes.
     * \param merge the false merge VI result
     * \param split the false split VI result
    */
    void compute_vi(double& merge, double& split);    
    
    /*!
     * Same as 'compute_vi' but it also returns a VI result for each label.
     * \param merge the false merge VI result
     * \param split the false split VI result
     * \param label_ranked labels from the label volume that are falsely merged
     * \param gt_ranked labels from the gt label volume that are falsely split
    */
    void compute_vi(double& merge, double& split, std::multimap<double, Label_t>& label_ranked,
            std::multimap<double, Label_t>& gt_ranked);


    /*!
     * Return the number of distinct labels in the label volume.  This assumes
     * that a RAG exists for this data volume.  Otherwise, a linear traversal
     * of the label volume would need to be performed.
     * \return number of labels
    */
    unsigned int get_num_labels() const
    {
        if (!rag) {
            throw ErrMsg("No rag defined for stack");
        }

        return rag->get_num_regions();
    }

    /*!
     * Return the x dimension size of the label volume.  This assumes
     * that a label volum exists.
     * \return size of dimension
    */
    unsigned int get_xsize() const
    {
        if (!labelvol) {
            throw ErrMsg("No label volume defined for stack"); 
        }
    
        return labelvol->shape(0);
    }

    /*!
     * Return the y dimension size of the label volume.  This assumes
     * that a label volum exists.
     * \return size of dimension
    */
    unsigned int get_ysize() const
    {
        if (!labelvol) {
            throw ErrMsg("No label volume defined for stack"); 
        }
    
        return labelvol->shape(1);
    }

    /*!
     * Return the z dimension size of the label volume.  This assumes
     * that a label volum exists.
     * \return size of dimension
    */
    unsigned int get_zsize() const
    {
        if (!labelvol) {
            throw ErrMsg("No label volume defined for stack"); 
        }

        return labelvol->shape(2);
    }

    /*!
     * Write the stack to disk.  The graph is written to the json
     * file format.  The label volume is rebased and written
     * to an h5 file.  There is an option to determine the best location
     * for observing the edge using the information in the probability volumes.
     * \param h5_name name of h5 file
     * \param graph_name name of graph json file
     * \param optimal_prob_edge_loc determine strategy to select edge location
     * \param disable_prob_comp determines whether saved prob values are used
    */
    void serialize_stack(const char* h5_name, const char* graph_name,
            bool optimal_prob_edge_loc, bool disable_prob_comp = false);
    
    /*!
     * Virtual function to allow derived controllers to add their own
     * information to the graph being written.
     * \param json_write json data to be written to disk
    */
    virtual void serialize_graph_info(Json::Value& json_write) {}
    
    /*!
     * Determined whether an edge exists between label1 and label2
     * given the ground truth label volume.
     * \param label1 volume label
     * \param label2 volume label
     * \return -1 if no edge and 1 if edge
    */
    int find_edge_label(Label_t label1, Label_t label2);
    
    /*!
     * Generates an assignment of labels to ground truth using overlap.
    */
    void compute_groundtruth_assignment();

     
  protected:
    /*!
     * Add edge to rag and update feature manager.
     * \param id1 region1 label id
     * \param id2 region2 label id
     * \param preds array of features
     * \param increment increment edge count
    */
    void rag_add_edge(unsigned int id1, unsigned int id2, std::vector<double>& preds,
            bool increment=true);

    //! declaration of typedef for x,y,z location representation
    typedef boost::tuple<unsigned int, unsigned int, unsigned int> Location;
    
    //! declaration of typedef for mapping of rag edges to doubles
    typedef std::tr1::unordered_map<RagEdge_t*, double> EdgeCount;

    //! declaration of typedef for mapping of rag edges to location 
    typedef std::tr1::unordered_map<RagEdge_t*, Location> EdgeLoc; 
   
    /*!
     * Support function called by 'serialize_graph_info' to find the
     * ideal point on the edge between two labels for examination.
     * Currently the strategy is to find an XY plane with the maximum
     * amount of 'edgyness'.
     * \param best_edge_z count associated with each edge
     * \param best_edge_loc location associated with each edge
     * \param optimal_prob_edge_loc determine strategy to select edge location
    */
    void determine_edge_locations(EdgeCount& best_edge_z,
        EdgeLoc& best_edge_loc, bool use_probs);

    // TODO: remove labelcount in favor of an unordered_map
    /*!
     * Structure to associate a count with each label used
     * in the construction of the contigency table.
    */
    struct LabelCount{
        Label_t lbl;
        size_t count;
        LabelCount(): lbl(0), count(0) {};	 	 		
        LabelCount(Label_t plbl, size_t pcount): lbl(plbl), count(pcount) {};	 	 		
    };
    
  private:
    /*!
     * Updates the assignment of labels to ground truth labels when
     * two labels have been merged together.  This update should be
     * unnecessary if the two labels being merged together were already
     * mapped to the same ground truth label.
     * \param label_remove label being removed
     * \param label_keep label beig kept
    */
    void update_assignment(Label_t label_remove, Label_t label_keep);
    
    /*!
     * Support function for the 'dilate_labelvol' and 'dilate_gt_labelvol'.
     * \param ptr pointer to a label volume
     * \param size of the dilation to be performed
     * \return new label volume created
    */
    VolumeLabelPtr dilate_label_edges(VolumeLabelPtr ptr, int disc_size);
    
    /*!
     * Support function for the 'dilate_labelvol' and 'dilate_gt_labelvol'
     * to create borders between each distinct label.
     * \param ptr pointer to a label volume
     * \return new label volume created
    */
    VolumeLabelPtr generate_boundary(VolumeLabelPtr ptr);   
    
    /*!
     * Support function called by 'compute_vi' and
     * 'compute_groundtruth_assignment' to produce the contingency table
     * of correspondence between the label volume and the ground truth.
    */
    void compute_contingency_table();
    
    /*!
     * Support function called by 'serialize_stack' that actually writes
     * the graph to json on disk.
     * \param graph_name name of graph json file
     * \param optimal_prob_edge_loc determine strategy to select edge location
     * \param disable_prob_comp determines whether saved prob values are used
    */
    void serialize_graph(const char* graph_name, bool optimal_prob_edge_loc, bool disable_prob_comp = false);
    
    /*!
     * Support function called by 'serialize_stack' that actually writes
     * the volume labels to h5 on disk.
     * \param h5_name name of h5 file
    */
    void serialize_labels(const char* h5_name);

    /*!
     * Contains information on the correspondence between the gt label
     * volume and the original label volume.  This could probably be
     * moved the Stack.
    */
    std::tr1::unordered_map<Label_t, std::vector<LabelCount> > contingency;	
    
    /*!
     * Contains a map that shows which ground truth label each original
     * label is mapped to (based on overlap).  This could probably be
     * moved to the Stack.
    */
    std::tr1::unordered_map<Label_t, Label_t> assignment;

};

}

#endif
