#include "GPR.h"

using namespace NeuroProof;

boost::mutex GPR::ThreadCompute::mutex;

GPR::GPR(Rag_uit& rag_, bool debug_) : rag(rag_), debug(debug_),
    total_num_voxelpairs(0), max_rand_base(0)
{
    // establish number of pixel pairs and baseline for a given RAG
    for (Rag_uit::nodes_iterator iter = rag.nodes_begin(); 
            iter != rag.nodes_end(); ++iter) {
        unsigned long long val = (*iter)->get_size();
        max_rand_base += (val * (val-1)/2);
        total_num_voxelpairs += val;
    }
    total_num_voxelpairs = total_num_voxelpairs * (total_num_voxelpairs - 1) / 2;
    
}

double GPR::calculateMaxExpectedRand()
{
    double max_rand = double(max_rand_base);
    // examing maximum possible correspondence given uncertainty -- similar
    // to the adjusted rand max index
    for (AffinityPair::Hash::iterator iter = affinity_pairs.begin(); 
            iter != affinity_pairs.end(); ++iter) {
        max_rand += ( (iter->size * (iter->weight) * (iter->weight)) + 
                (iter->size * (1-iter->weight) * iter->weight));
    }
    return max_rand;
}


double GPR::calculateGPR(int num_paths, int num_threads)
{
    std::vector<RagNode_uit* > node_list;
    for (Rag_uit::nodes_iterator iter = rag.nodes_begin(); 
            iter != rag.nodes_end(); ++iter) {
        node_list.push_back(*iter);
    }
    return calculateGPR(num_paths, num_threads, node_list);
}


double GPR::calculateGPR(int num_paths, int num_threads, 
        std::vector<RagNode_uit* >& node_list)
{
    boost::thread_group threads;

    // launch path finding algorithms over all nodes for different threads
    for (int i = 0; i < num_threads; ++i) {
        threads.create_thread(ThreadCompute(i, num_threads, num_paths, 
                    rag, affinity_pairs, node_list, debug));
    } 

    threads.join_all();

    double gpr_index = calculateNormalizedGPR();
    return gpr_index;
}

double GPR::calculateNormalizedGPR()
{
    double total_diffs = 0.0;
    for (AffinityPair::Hash::iterator iter = affinity_pairs.begin(); 
            iter != affinity_pairs.end(); ++iter) {
        double prob = (1 - iter->weight) * iter->weight;
        total_diffs += (prob * iter->size);
    }
  
    // adjust the rand index using a maximum and expected value 
    double max_rand = calculateMaxExpectedRand();
    double expected_rand = max_rand * max_rand / (total_num_voxelpairs);
    double adjusted_index = ((max_rand - total_diffs) - expected_rand) / 
        (max_rand - expected_rand) * 100;
    return adjusted_index;
}

void GPR::ThreadCompute::findBestPath(RagNode_uit* rag_node_head)
{
    best_node_head.rag_node_curr = rag_node_head;
    best_node_head.rag_edge_curr = 0;
    best_node_head.weight= 1.0;
    Node_uit node_head = rag_node_head->get_node_id();
    
    best_node_queue.push(best_node_head);
    AffinityPair affinity_pair_head(node_head, node_head);
    affinity_pair_head.weight = 1.0;

    // examine the shortest node paths first
    while (!best_node_queue.empty()) {
        BestNode best_node_curr = best_node_queue.top();
        AffinityPair affinity_pair_curr(node_head, 
                best_node_curr.rag_node_curr->get_node_id());
  
        if (temp_affinity_pairs.find(affinity_pair_curr) == 
                temp_affinity_pairs.end()) { 
            for (RagNode_uit::edge_iterator edge_iter = best_node_curr.rag_node_curr->edge_begin();
                    edge_iter != best_node_curr.rag_node_curr->edge_end(); ++edge_iter) {
                // avoid simple cycles
                if (*edge_iter == best_node_curr.rag_edge_curr) {
                    continue;
                }

                // grab other node 
                RagNode_uit* other_node = (*edge_iter)->get_other_node(best_node_curr.rag_node_curr);

                // avoid duplicates
                AffinityPair temp_pair(node_head, other_node->get_node_id());
                if (temp_affinity_pairs.find(temp_pair) != temp_affinity_pairs.end()) {
                    continue;
                }

                // don't examine paths below certain threshold values
                double edge_prob = (*edge_iter)->get_weight();
                if (edge_prob < EPSILON) {
                    continue;
                }
                edge_prob = best_node_curr.weight * edge_prob;
                if (edge_prob < CONNECTION_THRESHOLD) {
                    continue;
                }

                BestNode best_node_new;
                best_node_new.rag_node_curr = other_node;
                best_node_new.rag_edge_curr = *edge_iter;
                best_node_new.weight = edge_prob;
                best_node_queue.push(best_node_new);
            }
            affinity_pair_curr.weight = best_node_curr.weight; 
            affinity_pair_curr.size = rag_node_head->get_size() * 
                best_node_curr.rag_node_curr->get_size();
            temp_affinity_pairs.insert(affinity_pair_curr);
        }

        best_node_queue.pop();
    }
    
    temp_affinity_pairs.erase(affinity_pair_head);
    affinity_pairs_local.insert(temp_affinity_pairs.begin(), temp_affinity_pairs.end());
    temp_affinity_pairs.clear();
}

