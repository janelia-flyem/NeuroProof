/*!
 * The functionality in this interface is dedicated to refining
 * a segmentation described by its underlying graph by pointing
 * the algorithm/human to the edges that most likely are incorrect
 * as determined by different edge priority algorithms.  The general
 * strategy is called 'focused proofreading'.  The graph analysis
 * tools provided in NeuroProof show different ways to use this
 * functionality with a given graph (and ground truth) to estimate
 * how much work is required to fix a segmentation.
 *
 * /author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef EDGEEDITOR_H
#define EDGEEDITOR_H

#include "../Rag/RagNodeCombineAlg.h"
#include "../Rag/Rag.h"
#include "NodeSizeRank.h"
#include "ProbEdgeRank.h"
#include "OrphanRank.h"
#include "SynapseRank.h"

#include <vector>
#include <json/value.h>
#include <set>
#include <map>
#include <boost/tuple/tuple.hpp>

namespace NeuroProof {

/*!
 * Implements strategy for merging two nodes.  For now, the
 * the edge editor does not recompute uncertainties after merging
 * two nodes.  The heuristic used is to take the stronger
 * connection weight if given a choice.  Given the assumption
 * that editing the graph involves a series of merge-only operations
 * this will result in potentially examining more edges for merging.
*/
class LowWeightCombine : public RagNodeCombineAlg {
  public:
    void post_edge_move(RagEdge<unsigned int>* edge_new,
            RagEdge<unsigned int>* edge_remove)
    {
        double weight = edge_remove->get_weight();
        edge_new->set_weight(weight);
    }

    void post_edge_join(RagEdge<unsigned int>* edge_keep,
            RagEdge<unsigned int>* edge_remove)
    {
        double weight = edge_remove->get_weight();
        // take the smaller weight except if it is marked with a value
        // over 1.0 which means that it is definitely an edge
        if ((!(edge_remove->is_false_edge())) && (weight <= edge_keep->get_weight() && (edge_keep->get_weight() <= 1.0))
                || (weight > 1.0) ) { 
            edge_keep->set_weight(weight);
	    Location location = edge_remove->get_property<Location>("location");
	    edge_keep->set_property("location", location);
	}

	if (edge_keep->is_false_edge()) {
		Location location = edge_remove->get_property<Location>("location");
		edge_keep->set_property("location", location);
	}
    }

    void post_node_join(RagNode<unsigned int>* node_keep,
            RagNode<unsigned int>* node_remove) {}
};

class BodyRankList;

/*!
 * Main interface for focused editing.  The main input is a RAG with a set
 * of edges with some confidence weight between them.  Given some algorithm
 * for ordering these edges, this class determines which edge represents the
 * most uncertain edge for examination.  This interface is also meant
 * to work well with a python interface and third-party tools where the list
 * of edges can be written to and from in json and imported by other programs
 * which can call this program to determine which edge to examine.
*/
class EdgeEditor {
  public:
    typedef boost::tuple<unsigned int, unsigned int, unsigned int> Location;

    /*!
     * Only constructor for the editor.  Expects a RAG with edges with
     * confidence weights.  The min, max, and start values are paramters
     * relevant to proofreading using just edge confidence algorith and not a
     * more sophisticated algorithm.  These values will not actually a default
     * and are mostly legacy.  TODO: remove/modify constructor
     * to be more flexible and not require a json interface.
     * \param rag_ RAG with edges and edge weights
     * \param min_val_ Smallest edge confidence weight considered (deprecated)
     * \param max_val_ Largest edge confidence weight considered (deprecated)
     * \param start_val_ Starting edge weight for examination (deprecated)
     * \param json_vals Parameters for running editor (lists which bodies
     * are orphans and which focused mode to use)
    */
    EdgeEditor(Rag_t& rag_, double min_val_, double max_val_,
            double start_val_, Json::Value& json_vals); 

    /*!
     * Write focused proofreading statistics and other configurations
     * to json format.  Assumes that graph will be written (or has been)
     * written) to json.
     * \param json_writer json data
    */
    void export_json(Json::Value& json_writer);

    /*!
     * Set focused algorithm/strategy to consider the size of the body
     * with the uncertainty value.
     * \param ignore_size_ minimum size for an important body
     * \param depth path length considered for node affinity (0=unbounded)
    */ 
    void set_body_mode(double ignore_size_, int depth);
    
    /*!
     * Set focused algorithm/strategy to look at bodies not touching
     * the image boundary.
     * \param ignore_size threshold for an important body
    */
    void set_orphan_mode(double ignore_size_);
   
    /*!
     * Set focused algorithm/strategy to consider edges within a certain
     * range of probabilities.
     * \param lower lower uncertainty bound
     * \param upper upper uncertainty bound
     * \param start starting uncertainty for examining
     * \param ignore_size_ threshold for an important body
    */
    void set_edge_mode(double lower, double upper, double start, double ignore_size_ = 1);

    /*!
     * Set focused algorithm/strategy to consider the number of synapse
     * annotations with the unertainty value.
     * \param ignore_size_ minimum number of synapses in an important body
    */
    void set_synapse_mode(double ignore_size_);

    /*!
     * Find the number of edges that need to be examined (or an estimate).
     * \return number of edges remaining
    */
    unsigned int getNumRemaining();
   
    /*!
     * Get the number of edges currently in the queue.
     * \return number of edges in the queue
    */
    unsigned int getNumRemainingQueue();
 
    /*!
     * Mark an edge true or false.
     * \param node_pair pair of nodes connected by an edge
     * \param remove true for a false edge and false for a true edge
    */
    void removeEdge(NodePair node_pair, bool remove);
    
    /*!
     * Compute the number of regions in the RAG (above a threshold
     * or with a synapse annotation) that do not touch an image boundary
     * TODO: this can probably be made into a Rag utility.
     * \param threshold threshold below which violators are ignored
     * \return array of node ids
    */
    std::vector<Node_t> getQAViolators(unsigned int threshold);
    void estimateWork();

    /*!
     * Produce the most uncertain edge and its location.
     * \param location 3D location estimate for the edge
     * \return pair of nodes connected by an edge
    */
    NodePair getTopEdge(Location& location);
    void set_edge_label(int plabel);
    
    /*!
     * Undo the last decision made by calling removeEdge.
     * \return true if successful
    */
    bool undo();
  
    /*!
     * Determines if the current focused algorithm is finished.
     * \return true if no more edges to examine
    */
    bool isFinished();

    /*!
     * Set a custom focused algorithm/strategy for use in
     * focused editing.
     * \param edge_mode_ edge ranking algorithm
    */
    void set_custom_mode(EdgeRank* edge_mode_);

    void save_labeled_edges(std::string& save_fname);
    
    /*!
     * Editor destructor.
    */
    ~EdgeEditor();

  private:
    //! defines type for holding node/edge properties
    typedef std::tr1::unordered_map<std::string, boost::shared_ptr<Property> >
        NodePropertyMap; 

    /*!
     * Set the value of an edge to the desired weight (typically >1 to
     * indicate that this is a true edge. 
     * \param node_pair pair of nodes connected by an edge
     * \param weight value to set on edge
    */
    void setEdge(NodePair node_pair, double weight);

    /*!
     * Internal function for actually removing edge from Rag.
     * \param node_pair pair of nodes connected by an edge
     * \param remove set to true [deprecated]
     * \param node_properties properties that should be saved
    */ 
    void removeEdge2(NodePair node_pair, bool remove,
            std::vector<std::string>& node_properties);

    /*!
     * Internal function to be called whenever resetting algorithm.
    */
    void reinitialize_scheduler();

    /*!
     * Internal function called by undo to restore node and edge history.
     * \return true if successful
    */
    bool undo2();
    
    //! Rag that will be examined with a focused strategy
    Rag_t& rag;

    /*!
     * Internal struct for maintaining node/edge information of
     * edges that are removed or modified during editing.
    */
    struct EdgeHistory {
        //! node1 id
        Node_t node1;
        //! node1 id
        Node_t node2;
        
        //! node1 size
        unsigned long long size1;
        //! node2 size
        unsigned long long size2;
        
        //! node1 boundary size
        unsigned long long bound_size1;
        //! node2 boundary size
        unsigned long long bound_size2;
        
        //! nodes associated with node1
        std::vector<Node_t> node_list1;
        //! nodes associated with node2
        std::vector<Node_t> node_list2;

        //! property list for node1
        NodePropertyMap node_properties1;
        //! property list for node2
        NodePropertyMap node_properties2;
        
        //! weight of all connecting edges to node1  
        std::vector<double> edge_weight1;
        //! weight of all connecting edges to node2  
        std::vector<double> edge_weight2;

        //! preserve status of all connecting edges to node1
        std::vector<bool> preserve_edge1;
        //! preserve status of all connecting edges to node2
        std::vector<bool> preserve_edge2;

        //! false edge status of all connecting edges to node1
        std::vector<bool> false_edge1;
        //! false edge status of all connecting edges to node2
        std::vector<bool> false_edge2;

        //! edge properties for node1's edges
        std::vector<std::vector<boost::shared_ptr<Property> > > property_list1;
        //! edge properties for node2's edges
        std::vector<std::vector<boost::shared_ptr<Property> > > property_list2;
        
        //! whether the edge was removed
        bool remove;

        //! edge weight
        double weight;

        //! false status of edge
        bool false_edge;

        //! preserve status of edge
        bool preserve_edge;

        //! edge properties for current edge
        std::vector<PropertyPtr> property_list_curr;
    };

    //! array of history elements to allow undoing of operations
    std::vector<EdgeHistory> history_queue;

    //! algorithm used for combining nodes
    LowWeightCombine join_alg;

    //! minimum, maximum, ans starting value 
    double min_val, max_val, start_val; 
    
    //! number of edges examined
    unsigned int num_processed;

    //! number of synapse focused edges examined   
    unsigned int num_syn_processed;

    //! number of body size focused edges examined   
    unsigned int num_body_processed;    
    
    //! number of edges examined using connection weights 
    unsigned int num_edge_processed;

    //! number of orphan focused edges examined 
    unsigned int num_orphan_processed;

    //! number of slices in volume
    unsigned int num_slices;

    //! maximum depth allowed between nodes for analysis
    unsigned int current_depth;

    //! estimate of number of remaining edges to examine
    unsigned int num_est_remaining;    
    
    //! threshold used in different focused algorithsm
    double ignore_size;

    //! properties that need to be saved when modifying the RAG 
    std::vector<std::string> node_properties;

    //! constant string for accessing synapse property
    const std::string SynapseStr;

    // ordering algorithms supported by builtin
    // import utility -- this can be extended easily
    // other rank algorithms can always be used
    // generically but its stats and mode will
    // not be exportable by the edge editor
    
    //! edge rank algorithm using edge probability
    ProbEdgeRank* prob_edge_mode;

    //! orphan rank algorithm using orphan status of body
    OrphanRank* orphan_edge_mode;

    //! synapse rank algorithm using synapse uncertainty
    SynapseRank* synapse_edge_mode;

    //! body size rank algorithm using body size uncertainty
    NodeSizeRank* body_edge_mode;

    //! current rank algorithm used by the editor
    EdgeRank* edge_mode;
    
    
};

}

#endif
