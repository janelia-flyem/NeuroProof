/*!
 * This file implements the core algorithm described in
 *u http://www.plosone.org/article/info%3Adoi%2F10.1371%2Fjournal.pone.0044448
 * [Plaza '12].  In particular, the implementation concerns
 * only examining a single path determine affinity between two
 * nodes.  The paper also generalizes and explains assesing affinity
 * by considering multiple paths.  In general, we don't find this
 * more computationally intensive calculation to be beneficial even
 * if it is presumably more accurate.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef GPR_H
#define GPR_H

#include "../Rag/Rag.h"
#include "../Utilities/AffinityPair.h"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <queue>
#include <iostream>

namespace NeuroProof {

/*!
 * Calculate the uncertainty of a given segmentation graph
 * (Generalized Probabilistic Rand -- GPR)  by computating
 * the node affinities between each pair of nodes in the
 * graph (that are reasonably close) using the edge weights.
 * The algorithm requires running a dijkstra's search algorithm
 * over each node in the graph.  In practice this is not require
 * an all paths algorithm since some nodes will effectively not
 * connect (will be below some threshold beyond which we ignore).
*/
class GPR {
  public:
    /*!
     * Constructor that requires a RAG with edges that have
     * weights that give an estimate of how like the edge is
     * true.  1 is a prediction for a true edge.  0 is a 
     * prediction for a false edge.
     * \param rag_ reference to RAG
     * \debug_ enables some debug output
    */
    GPR(Rag_uit& rag_, bool debug_ = false); 
    
    /*!
     * Calculate the GPR for the RAG.
     * \param num_paths num paths to compute GPR [values not 1 unsupported]
     * \param num_threads num threads to compute GPR
     * \return value of GPR
    */
    double calculateGPR(int num_paths, int num_threads); 

  private:
    /*!
     * Calculate the GPR for the RAG.
     * \param num_paths num paths to compute GPR [values not 1 unsupported]
     * \param num_threads num threads to compute GPR 
     * \param node_list nodes to be examined
     * \return value of GPR
    */
    double calculateGPR(int num_paths, int num_threads, std::vector<RagNode_uit* >& node_list); 

    /*!
     * Calculates the maximum expected rand index using
     * the affinities computed between the nodes.
     * \return maximum rand index
    */
    double calculateMaxExpectedRand();
    
    /*!
     * Normalize GPR value using an normalization scheme
     * similar to the one used in adjusting rand index.
     * \return value of GPR
    */
    double calculateNormalizedGPR();

    //! Reference to rag analyzed for uncertainty 
    Rag_uit& rag;

    //! The minimum rand correspondence independent of edge decisions 
    unsigned long long max_rand_base;

    //! Total number of pairs of points over whole rag
    unsigned long long total_num_voxelpairs;
    
    //! Debug mode flag
    bool debug;

    //! Structure for holding pairs of nodes and the affinity weight
    AffinityPair::Hash affinity_pairs;

    /*!
     * Struct for assign the GPR calculation work to different worker
     * threads.  The GPR calculation cost is dominated by analyzing
     * the paths for each node.  The algorithm can be trivially
     * parallelized by assigning nodes to different threads.
    */
    struct ThreadCompute {
        /*!
         * Constructor for an individual thread of computation
         * that assigns a thread id, the number of paths used (1 is only
         * supported), and a shared affinity_pairs structure where
         * results are written back.
         * \param id_ thread id
         * \param num_threads_ number of threads to use
         * \param num_paths_ number of paths to find affinty (1 supported)
         * \param rag_ reference to RAG
         * \param affinity_pair_ set of node pairs with affinities
         * \param node_list_ set of nodes that will be analyzed
        */ 
        ThreadCompute(int id_, int num_threads_, int num_paths_, Rag_uit& rag_, 
                AffinityPair::Hash& affinity_pairs_, 
                std::vector<RagNode_uit* >& node_list_, bool debug_) : 
            id(id_), num_threads(num_threads_), num_paths(num_paths_), 
            rag(rag_), affinity_pairs(affinity_pairs_), node_list(node_list_), 
            debug(debug_), EPSILON(0.000001), CONNECTION_THRESHOLD(0.01) {}

        /*!
         * Function overloaded operation to be called by the boost
         * threading library and actually perform the path examination
         * algorithms to determine the node affinities.
        */
        void operator()()
        {
            int num_regions = rag.get_num_regions() / num_threads;
            int increment = num_regions/100;
            int curr_num = 0;

            for (int i = id; i < node_list.size();) {
                if (num_paths == 1) {
                    if (debug && (id == 0)) {
                        if (increment && !(curr_num++ % increment)) {
                            std::cout << curr_num/double(num_regions) * 100 << 
                                "% done" << std::endl;
                        }
                    }
                    findBestPath(node_list[i]);
                } else {
                    throw ErrMsg("Multi-path prob calculation not re-implemented");
                }
                i += num_threads;
            }
            boost::mutex::scoped_lock scoped_lock(mutex);
            affinity_pairs.insert(affinity_pairs_local.begin(), affinity_pairs_local.end());
        }

        //! Thread id
        int id;

        //! Number of threads
        int num_threads;

        //! Number of paths to calculate affinity (should be 1 for now)
        int num_paths;

        //! Reference to RAG
        Rag_uit& rag;

        //! Affinity between node pairs (not necessarily directly connected)
        AffinityPair::Hash& affinity_pairs;

        //! List of nodes to be considered for affinity
        std::vector<RagNode_uit* >& node_list;

        //! Enables debug mode
        bool debug;

        //! If below this, treat edge as a true edge 
        const double EPSILON;

        //! Minimum affinity allowed between nodes before being ignored 
        const double CONNECTION_THRESHOLD;

        //! Mutex for protecting affinity nodes structures
        static boost::mutex mutex;

        //! Thread memory for affinity pairs
        AffinityPair::Hash affinity_pairs_local;
        
        /*!
         *  Element to rank highest affinity nodes.
        */
        struct BestNode {
            //! Current node in path being examined
            RagNode_uit* rag_node_curr;
        
            //! Current edge traversed to get to node
            RagEdge_uit* rag_edge_curr;
            
            //! Current connection of weight
            double weight;
        };
        
        /*!
         * Structure to rank nodes by connection weight.
        */
        struct BestNodeCmp {
            bool operator()(const BestNode& q1, const BestNode& q2) const
            {
                return (q1.weight < q2.weight);
            }
        };

        //! Defines priority queue for running Dijkstra's algorithm
        typedef std::priority_queue<BestNode, std::vector<BestNode>, BestNodeCmp> BestNodeQueue;

        //! Starting node in path
        BestNode best_node_head;

        //! Queue of nodes being examined
        BestNodeQueue best_node_queue;

        //! Temporary affinity pairs stored
        AffinityPair::Hash temp_affinity_pairs;

        /*
         * Runs a version of Dijkstra's algorithm with multiplication
         * where edges range in values from 0 to 1.  The result
         * is a set of affinity pairs with the starting head node.
         * \param rag_node_head Starting node for path search
        */
        void findBestPath(RagNode_uit* rag_node_head);
    };
};

}

#endif

