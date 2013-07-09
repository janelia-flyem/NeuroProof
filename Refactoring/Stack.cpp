#include "Stack.h"

#include "../BioPriors/MitoTypeProperty.h"
#include "../Algorithms/FeatureJoinAlgs.h"

using namespace NeuroProof;
using namespace std;
using std::string;

Label find_max(Label* data, const size_t* dims)
{
    Label max=0;  	
    size_t plane_size = dims[1]*dims[2];
    size_t width = dims[2];	 	
    for(size_t i=0;i<dims[0];i++){
        for (size_t j=0;j<dims[1];j++){
            for (size_t k=0;k<dims[2];k++){
                size_t curr_spot = i*plane_size + j * width + k;
                if (max<data[curr_spot])
                    max = data[curr_spot];	
            }
        }
    }	
    return max;	
}


void Stack2::build_rag()
{
    if (feature_mgr && (feature_mgr->get_num_features() == 0)) {
        feature_mgr->add_median_feature();
        median_mode = true; 
    } 

    unsigned int plane_size = width * height;
    std::vector<double> predictions(prediction_array.size(), 0.0);
    std::tr1::unordered_set<Label> labels;
    
    std::tr1::unordered_map<Label, MitoTypeProperty> mito_probs;

    for (unsigned int z = 1; z < (depth-1); ++z) {
        int z_spot = z * plane_size;
        for (unsigned int y = 1; y < (height-1); ++y) {
            int y_spot = y * width;
            for (unsigned int x = 1; x < (width-1); ++x) {
                unsigned long long curr_spot = x + y_spot + z_spot;
                unsigned int spot0 = watershed[curr_spot];
                unsigned int spot1 = watershed[curr_spot-1];
                unsigned int spot2 = watershed[curr_spot+1];
                unsigned int spot3 = watershed[curr_spot-width];
                unsigned int spot4 = watershed[curr_spot+width];
                unsigned int spot5 = watershed[curr_spot-plane_size];
                unsigned int spot6 = watershed[curr_spot+plane_size];

                for (unsigned int i = 0; i < prediction_array.size(); ++i) {
                    predictions[i] = prediction_array[i][curr_spot];
                }

                if (!spot0) {
                    continue;
                }

                RagNode_uit * node = rag->find_rag_node(spot0);
                if (!node) {
                    node = rag->insert_rag_node(spot0);
                }

                if (feature_mgr && !median_mode) {
                    feature_mgr->add_val(predictions, node);
                }
                node->incr_size();

                if (!predictions.empty() && merge_mito) {
                    mito_probs[spot0].update(predictions); 
                    /*try { 
                        MitoTypeProperty& mtype = node->get_property<MitoTypeProperty>("mito-type");
                        mtype.update(predictions);
                    } catch (ErrMsg& msg) {
                        MitoTypeProperty mtype;
                        mtype.update(predictions);
                        node->set_property("mito-type", mtype);
                    } */
                }

                if (spot1 && (spot0 != spot1)) {
                    rag_add_edge(rag, spot0, spot1, predictions, feature_mgr);
                    labels.insert(spot1);
                }
                if (spot2 && (spot0 != spot2) && (labels.find(spot2) == labels.end())) {
                    rag_add_edge(rag, spot0, spot2, predictions, feature_mgr);
                    labels.insert(spot2);
                }
                if (spot3 && (spot0 != spot3) && (labels.find(spot3) == labels.end())) {
                    rag_add_edge(rag, spot0, spot3, predictions, feature_mgr);
                    labels.insert(spot3);
                }
                if (spot4 && (spot0 != spot4) && (labels.find(spot4) == labels.end())) {
                    rag_add_edge(rag, spot0, spot4, predictions, feature_mgr);
                    labels.insert(spot4);
                }
                if (spot5 && (spot0 != spot5) && (labels.find(spot5) == labels.end())) {
                    rag_add_edge(rag, spot0, spot5, predictions, feature_mgr);
                    labels.insert(spot5);
                }
                if (spot6 && (spot0 != spot6) && (labels.find(spot6) == labels.end())) {
                    rag_add_edge(rag, spot0, spot6, predictions, feature_mgr);
                }

                if (!spot1 || !spot2 || !spot3 || !spot4 || !spot5 || !spot6) {
                    node->incr_boundary_size();
                }
                labels.clear();

            }
        }
    } 

    watershed_to_body[0] = 0;
    for (Rag_uit::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
        Label id = (*iter)->get_node_id();
        watershed_to_body[id] = id;
        if (!predictions.empty() && merge_mito) {
            MitoTypeProperty mtype = mito_probs[id];
            mtype.set_type(); 
            (*iter)->set_property("mito-type", mtype);
        }
    }
}

void Stack2::init_mappings()
{
    unsigned long long total_size = width*height*depth;
    for (unsigned long long i = 0; i < total_size; ++i) {
        watershed_to_body[(watershed[i])] = watershed[i];
    }
}

int Stack2::remove_inclusions()
{
    int num_removed = 0;

    visited.clear();
    node_depth.clear();
    low_count.clear();
    prev_id.clear();
    biconnected_components.clear();
    stack.clear();

    RagNode_uit* rag_node = 0;
    for (Rag_uit::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
        bool border = (*iter)->is_boundary();

        if (border) {
            rag_node = *iter;
            break;
        }
    }
    assert(rag_node);

    DFSStack temp;
    temp.previous = 0;
    temp.rag_node = rag_node;
    temp.count = 1;
    temp.start_pos = 0;

    std::vector<DFSStack> dfs_stack;
    dfs_stack.push_back(temp);
    biconnected_dfs(dfs_stack);
    stack.clear();
    std::tr1::unordered_map<Label, Label> body_to_body; 
    std::tr1::unordered_map<Label, std::vector<Label> > merge_history2; 
    
    FeatureCombine node_combine_alg(feature_mgr, rag); 

    // merge nodes in biconnected_components (ignore components with '0' node)
    for (int i = 0; i < biconnected_components.size(); ++i) {
        bool found_zero = false;
        std::tr1::unordered_set<Label> merge_nodes;
        for (int j = 0; j < biconnected_components[i].size()-1; ++j) {
            Label region1 = biconnected_components[i][j].region1;     
            Label region2 = biconnected_components[i][j].region2;
            if (!region1 || !region2) {
                found_zero = true;
            }

            if (body_to_body.find(region1) != body_to_body.end()) {
                region1 = body_to_body[region1];
            }
            if (body_to_body.find(region2) != body_to_body.end()) {
                region2 = body_to_body[region2];
            }

            assert(region1 != region2);

            merge_nodes.insert(region1);
            merge_nodes.insert(region2);
        }
        if (!found_zero) {
            Label articulation_region = biconnected_components[i][biconnected_components[i].size()-1].region1;
            RagNode_uit* articulation_node = rag->find_rag_node(articulation_region);
            bool found_preserve = false; 
            for (std::tr1::unordered_set<Label>::iterator iter = merge_nodes.begin(); iter != merge_nodes.end(); ++iter) {
                Label region2 = *iter;
                if (body_to_body.find(region2) != body_to_body.end()) {
                    region2 = body_to_body[region2];
                }

                RagNode_uit* rag_node = rag->find_rag_node(region2);

                assert(rag_node);
                if (articulation_node != rag_node) {
                    for (RagNode_uit::edge_iterator edge_iter = rag_node->edge_begin();
                            edge_iter != rag_node->edge_end(); ++edge_iter) {
                        if ((*edge_iter)->is_preserve()) {
                            found_preserve = true;
                            break;
                        }
                    } 
                }

                if (found_preserve) {
                    break;
                }
            }

            if (found_preserve) {
                continue;
            }

            for (std::tr1::unordered_set<Label>::iterator iter = merge_nodes.begin(); iter != merge_nodes.end(); ++iter) {
                Label region2 = *iter;
                if (body_to_body.find(region2) != body_to_body.end()) {
                    region2 = body_to_body[region2];
                }

                RagNode_uit* rag_node = rag->find_rag_node(region2);
                assert(rag_node);
                if (articulation_node != rag_node) {
                    RagNode_uit::node_iterator n_iter = rag_node->node_begin();
                  
                    Label other_id = (*n_iter)->get_node_id();
                    rag_join_nodes(*rag, *n_iter, rag_node, &node_combine_alg);  
 
                    watershed_to_body[region2] = other_id;
                    for (std::vector<Label>::iterator iter2 = merge_history[region2].begin(); iter2 != merge_history[region2].end(); ++iter2) {
                        watershed_to_body[*iter2] = other_id;
                    }

                    body_to_body[region2] = other_id;
                    for (std::vector<Label>::iterator iter2 = merge_history2[region2].begin(); iter2 != merge_history2[region2].end(); ++iter2) {
                        body_to_body[*iter2] = other_id;
                    }

                    merge_history[other_id].push_back(region2);
                    merge_history[other_id].insert(merge_history[other_id].end(), merge_history[region2].begin(), merge_history[region2].end());
                    merge_history.erase(region2);

                    merge_history2[other_id].push_back(region2); 
                    merge_history2[other_id].insert(merge_history2[other_id].end(), merge_history2[region2].begin(), merge_history2[region2].end());
                    merge_history2.erase(region2);
                }
            }
        }
    }


    visited.clear();
    node_depth.clear();
    low_count.clear();
    prev_id.clear();
    biconnected_components.clear();
    stack.clear();
    return num_removed;
}

// return low point (0 is default edge)
void Stack2::biconnected_dfs(std::vector<DFSStack>& dfs_stack)
{
    while (!dfs_stack.empty()) {
        DFSStack entry = dfs_stack.back();
        RagNode_uit* rag_node = entry.rag_node;
        Label previous = entry.previous;
        int count = entry.count;
        dfs_stack.pop_back();

        if (visited.find(rag_node->get_node_id()) == visited.end()) {
            visited.insert(rag_node->get_node_id());
            node_depth[rag_node->get_node_id()] = count;
            low_count[rag_node->get_node_id()] = count;
            prev_id[rag_node->get_node_id()] = previous;
        }

        bool skip = false;
        int curr_pos = 0;
        for (RagNode_uit::node_iterator iter = rag_node->node_begin(); iter != rag_node->node_end(); ++iter) {
            RagEdge_uit* rag_edge = rag->find_rag_edge(rag_node, *iter);
            if (rag_edge->is_false_edge()) {
                continue;
            }

            if (curr_pos < entry.start_pos) {
                ++curr_pos;
                continue;
            }
            if (prev_id[(*iter)->get_node_id()] == rag_node->get_node_id()) {
                OrderedPair current_edge(rag_node->get_node_id(), (*iter)->get_node_id());
                int temp_low = low_count[(*iter)->get_node_id()];
                low_count[rag_node->get_node_id()] = std::min(low_count[rag_node->get_node_id()], temp_low);

                if (temp_low >= count) {
                    OrderedPair popped_edge;
                    biconnected_components.push_back(std::vector<OrderedPair>());
                    do {
                        popped_edge = stack.back();
                        stack.pop_back();
                        biconnected_components[biconnected_components.size()-1].push_back(popped_edge);
                    } while (!(popped_edge == current_edge));
                    OrderedPair articulation_pair(rag_node->get_node_id(), rag_node->get_node_id());
                    biconnected_components[biconnected_components.size()-1].push_back(articulation_pair);
                } 
            } else if (visited.find((*iter)->get_node_id()) == visited.end()) {
                OrderedPair current_edge(rag_node->get_node_id(), (*iter)->get_node_id());
                stack.push_back(current_edge);

                DFSStack temp;
                temp.previous = rag_node->get_node_id();
                temp.rag_node = (*iter);
                temp.count = count+1;
                temp.start_pos = 0;
                entry.start_pos = curr_pos;
                dfs_stack.push_back(entry);
                dfs_stack.push_back(temp);
                skip = true;
                break;
            } else if ((*iter)->get_node_id() != previous) {
                low_count[rag_node->get_node_id()] = std::min(low_count[rag_node->get_node_id()], node_depth[(*iter)->get_node_id()]);
                if (count > node_depth[(*iter)->get_node_id()]) {
                    stack.push_back(OrderedPair(rag_node->get_node_id(), (*iter)->get_node_id()));
                }
            }
            ++curr_pos;
        }

        if (skip) {
            continue;
        }

        bool border = rag_node->is_boundary();
        if (previous && border) {
            low_count[rag_node->get_node_id()] = 0;
            stack.push_back(OrderedPair(0, rag_node->get_node_id()));
        }
    }
}

void Stack2::determine_edge_locations(bool use_probs)
{
    best_edge_z.clear();
    best_edge_loc.clear();

    unsigned int plane_size = width * height;

    if (depth == 0 || height == 0 || width == 0) {
        return;
    }

    for (unsigned int z = 1; z < (depth-1); ++z) {
        int z_spot = z * plane_size;

        EdgeCount curr_edge_z;
        EdgeLoc curr_edge_loc;


        for (unsigned int y = 1; y < (height-1); ++y) {
            int y_spot = y * width;
            for (unsigned int x = 1; x < (width-1); ++x) {
                unsigned long long curr_spot = x + y_spot + z_spot;
                unsigned int spot0 = watershed_to_body[(watershed[curr_spot])];
                unsigned int spot1 = watershed_to_body[(watershed[curr_spot-1])];
                unsigned int spot2 = watershed_to_body[(watershed[curr_spot+1])];
                unsigned int spot3 = watershed_to_body[(watershed[curr_spot-width])];
                unsigned int spot4 = watershed_to_body[(watershed[curr_spot+width])];
                unsigned int spot5 = watershed_to_body[(watershed[curr_spot-plane_size])];
                unsigned int spot6 = watershed_to_body[(watershed[curr_spot+plane_size])];

                double incr = 1.0;
                if (use_probs) {
                    // pick plane with a lot of low edge probs
                    incr = 1.0 - prediction_array[0][curr_spot];
                }

                if (spot1 && (spot0 != spot1)) {
                    RagEdge_uit* edge = rag->find_rag_edge(spot0, spot1);
                    curr_edge_z[edge] += incr;  
                    curr_edge_loc[edge] = Location(x,y,z);  
                }
                if (spot2 && (spot0 != spot2)) {
                    RagEdge_uit* edge = rag->find_rag_edge(spot0, spot2);
                    curr_edge_z[edge] += incr;  
                    curr_edge_loc[edge] = Location(x,y,z);  
                }
                if (spot3 && (spot0 != spot3)) {
                    RagEdge_uit* edge = rag->find_rag_edge(spot0, spot3);
                    curr_edge_z[edge] += incr;  
                    curr_edge_loc[edge] = Location(x,y,z);  
                }
                if (spot4 && (spot0 != spot4)) {
                    RagEdge_uit* edge = rag->find_rag_edge(spot0, spot4);
                    curr_edge_z[edge] += incr;  
                    curr_edge_loc[edge] = Location(x,y,z);  
                }
                if (spot5 && (spot0 != spot5)) {
                    RagEdge_uit* edge = rag->find_rag_edge(spot0, spot5);
                    curr_edge_z[edge] += incr;  
                    curr_edge_loc[edge] = Location(x,y,z);  
                }
                if (spot6 && (spot0 != spot6)) {
                    RagEdge_uit* edge = rag->find_rag_edge(spot0, spot6);
                    curr_edge_z[edge] += incr; 
                    curr_edge_loc[edge] = Location(x,y,z);  
                }
            }
        }

        for (EdgeCount::iterator iter = curr_edge_z.begin(); iter != curr_edge_z.end(); ++iter) {
            if (iter->second > best_edge_z[iter->first]) {
                best_edge_z[iter->first] = iter->second;
                best_edge_loc[iter->first] = curr_edge_loc[iter->first];
            }
        }
    } 


    return;
}

bool Stack2::add_edge_constraint2(unsigned int x1, unsigned int y1, unsigned int z1,
        unsigned int x2, unsigned int y2, unsigned int z2)
{
    Label body1 = get_body_id(x1, y1, z1);    
    Label body2 = get_body_id(x2, y2, z2);    

    if (body1 == body2) {
        return false;
    }

    if (!body1 || !body2) {
        return true;
    }

    RagEdge_uit* edge = rag->find_rag_edge(body1, body2);

    if (!edge) {
        RagNode_uit* node1 = rag->find_rag_node(body1);
        RagNode_uit* node2 = rag->find_rag_node(body2);
        edge = rag->insert_rag_edge(node1, node2);
        edge->set_weight(1.0);
        edge->set_false_edge(true);
    }
    edge->set_preserve(true);

    return true;
}


bool Stack2::add_edge_constraint(boost::python::tuple loc1, boost::python::tuple loc2)
{
    return add_edge_constraint2(boost::python::extract<int>(loc1[0]),
            boost::python::extract<int>(loc1[1]), boost::python::extract<int>(loc1[2]),
            boost::python::extract<int>(loc2[0]), boost::python::extract<int>(loc2[1]),
            boost::python::extract<int>(loc2[2]));    
}

Label Stack2::get_body_id(unsigned int x, unsigned int y, unsigned int z)
{
    x += padding;
    //y += padding;
    y = height - y - 1 - padding;
    z += padding;
    unsigned int plane_size = width * height;
    unsigned long long curr_spot = x + y * width + z * plane_size; 
    Label body_id = watershed[curr_spot];
    if (!watershed_to_body.empty()) {
        body_id = watershed_to_body[body_id];
    }
    return body_id;
}

Label Stack2::get_gt_body_id(unsigned int x, unsigned int y, unsigned int z)
{
    unsigned int w2 = width - 2*padding;
    unsigned int h2 = height - 2*padding;
    unsigned int d2 = depth - 2*padding;
    y = h2 - y - 1;
    unsigned int plane_size = w2 * h2;
    unsigned long long curr_spot = x + y * w2 + z * plane_size; 
    Label body_id = gtruth[curr_spot];
    return body_id;
}

Label* Stack2::get_label_volume(){

    Label * temp_labels = new Label[(width-(2*padding))*(height-(2*padding))*(depth-(2*padding))];
    Label * temp_labels_iter = temp_labels;
    unsigned int plane_size = width * height;

    for (int z = padding; z < (depth - padding); ++z) {
        int z_spot = z * plane_size;
        for (int y = padding; y < (height - padding); ++y) {
            int y_spot = y * width;
            for (int x = padding; x < (width - padding); ++x) {
                unsigned long long curr_spot = x+y_spot+z_spot;
                Label sv_id = watershed[curr_spot];
                Label body_id;
                if (!watershed_to_body.empty()) {
                    body_id = watershed_to_body[sv_id];
                } else {
                    body_id = sv_id;
                }
                *temp_labels_iter = body_id;
                ++temp_labels_iter;
            }
        }
    }

    return temp_labels;
}
boost::python::tuple Stack2::get_edge_loc2(RagEdge_uit* edge)
{
    if (best_edge_loc.find(edge) == best_edge_loc.end()) {
        throw ErrMsg("Edge location was not loaded!");
    }
    Location loc = best_edge_loc[edge];
    unsigned int x = boost::get<0>(loc) - padding;
    unsigned int y = boost::get<1>(loc) - padding; //height - boost::get<1>(loc) - 1 - padding;
    unsigned int z = boost::get<2>(loc) - padding;

    return boost::python::make_tuple(x, y, z); 
}

void Stack2::get_edge_loc(RagEdge_uit* edge, Label&x, Label& y, Label& z)
{
    if (best_edge_loc.find(edge) == best_edge_loc.end()) {
        throw ErrMsg("Edge location was not loaded!");
    }
    Location loc = best_edge_loc[edge];
    x = boost::get<0>(loc) - padding;
    y = boost::get<1>(loc) - padding; //height - boost::get<1>(loc) - 1 - padding;
    z = boost::get<2>(loc) - padding;

}

void Stack2::build_rag_border()
{
    if (feature_mgr && (feature_mgr->get_num_features() == 0)) {
        feature_mgr->add_median_feature();
        median_mode = true; 
    } 
    
    std::vector<double> predictions(prediction_array.size(), 0.0);
    std::vector<double> predictions2(prediction_array.size(), 0.0);

    unsigned int plane_size = width * height;
    for (unsigned int z = 1; z < (depth-1); ++z) {
        int z_spot = z * plane_size;
        for (unsigned int y = 1; y < (height-1); ++y) {
            int y_spot = y * width;
            for (unsigned int x = 1; x < (width-1); ++x) {
                unsigned long long curr_spot = x + y_spot + z_spot;
                unsigned int spot0 = watershed[curr_spot];
                unsigned int spot1 = watershed2[curr_spot];

                if (!spot0 || !spot1) {
                    continue;
                }

                assert(spot0 != spot1);

                RagNode_uit * node = rag->find_rag_node(spot0);
                if (!node) {
                    node = rag->insert_rag_node(spot0);
                }

                RagNode_uit * node2 = rag->find_rag_node(spot1);

                if (!node2) {
                    node2 = rag->insert_rag_node(spot1);
                }

                for (unsigned int i = 0; i < prediction_array.size(); ++i) {
                    predictions[i] = prediction_array[i][curr_spot];
                }
                for (unsigned int i = 0; i < prediction_array2.size(); ++i) {
                    predictions2[i] = prediction_array2[i][curr_spot];
                }

                if (feature_mgr && !median_mode) {
                    feature_mgr->add_val(predictions, node);
                    feature_mgr->add_val(predictions2, node2);
                }
                node->incr_size();
                node2->incr_size();

                rag_add_edge(rag, spot0, spot1, predictions, feature_mgr);
                rag_add_edge(rag, spot0, spot1, predictions2, feature_mgr);

                node->incr_boundary_size();
                node2->incr_boundary_size();
            }
        }
    }

    watershed_to_body[0] = 0;
    for (Rag_uit::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
        Label id = (*iter)->get_node_id();
        watershed_to_body[id] = id;
    }
}

void Stack2::disable_nonborder_edges()
{
    return;
}

void Stack2::enable_nonborder_edges()
{
    return;
}

void Stack2::agglomerate_rag(double threshold)
{
    if (threshold == 0.0) {
        return;
    }

    MergePriority* priority = new ProbPriority(feature_mgr, rag);
    priority->initialize_priority(threshold);
    
    DelayedPriorityCombine node_combine_alg(feature_mgr, rag, priority); 
    while (!(priority->empty())) {

        RagEdge_uit* rag_edge = priority->get_top_edge();

        if (!rag_edge) {
            continue;
        }

        RagNode_uit* rag_node1 = rag_edge->get_node1();
        RagNode_uit* rag_node2 = rag_edge->get_node2();
        Label node1 = rag_node1->get_node_id(); 
        Label node2 = rag_node2->get_node_id();
        rag_join_nodes(*rag, rag_node1, rag_node2, &node_combine_alg);  
        watershed_to_body[node2] = node1;
        for (std::vector<Label>::iterator iter = merge_history[node2].begin(); iter != merge_history[node2].end(); ++iter) {
            watershed_to_body[*iter] = node1;
        }
        merge_history[node1].push_back(node2);
        merge_history[node1].insert(merge_history[node1].end(), merge_history[node2].begin(), merge_history[node2].end());
        merge_history.erase(node2);
    }

    delete priority;
}

boost::python::list Stack2::get_transformations()
{
    boost::python::list transforms;

    for (std::tr1::unordered_map<Label, Label>::iterator iter = watershed_to_body.begin();
           iter != watershed_to_body.end(); ++iter)
    {
        if (iter->first != iter->second) {
            //boost::python::tuple<Label, Label> mapping(iter->first, iter->second);
            transforms.append(boost::python::make_tuple(iter->first, iter->second));
        }
    } 

    return transforms;
}

void Stack2::load_synapse_counts(std::tr1::unordered_map<Label, int>& synapse_bodies)
{
    for (int i = 0; i < all_locations.size(); ++i) {
        Label body_id = get_body_id(all_locations[i][0], all_locations[i][1], all_locations[i][2]); 
        if (body_id) {
            if (synapse_bodies.find(body_id) == synapse_bodies.end()) {
                synapse_bodies[body_id] = 0;
            }
            synapse_bodies[body_id]++;
        }
    }

}

void Stack2::write_graph_json(Json::Value& json_writer)
{
    std::tr1::unordered_map<Label, int> synapse_bodies;
    load_synapse_counts(synapse_bodies);

    int id = 0;
    for (std::tr1::unordered_map<Label, int>::iterator iter = synapse_bodies.begin();
        iter != synapse_bodies.end(); ++iter, ++id) {
        Json::Value synapse_pair;
        synapse_pair[(unsigned int)(0)] = iter->first;
        synapse_pair[(unsigned int)(1)] = iter->second;
        json_writer["synapse_bodies"][id] =  synapse_pair;
    }

    id = 0;
    for (Rag_uit::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
        if (is_orphan(*iter)) {
            json_writer["orphan_bodies"][id] = (*iter)->get_node_id();
            ++id;
        } 
    }
}

void Stack2::write_graph(string output_path)
{

    unsigned found = output_path.find_last_of(".");
    string dirname = output_path.substr(0,found);
    string filename = dirname + string("_edge_list.txt");

    FILE *fp = fopen(filename.c_str(), "wt");
    if(!fp){
        printf("%s could not be opened \n", filename.c_str());
        return;
    }
    fprintf(fp,"Nodes:\n");
    for (Rag_uit::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
        fprintf(fp,"%u %lu %u\n", (*iter)->get_node_id(), (*iter)->get_size(), 
                (is_orphan(*iter)? 1 : 0));
    }
    fprintf(fp,"\nEdges:\n");

    Label x, y, z;
    for (Rag_uit::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
        get_edge_loc(*iter, x, y, z); 
        fprintf(fp,"%u %u %lu %lu %u %u %lf %u %u %u\n", (*iter)->get_node1()->get_node_id(), (*iter)->get_node2()->get_node_id(),
                (*iter)->get_node1()->get_size(), (*iter)->get_node2()->get_size(),
                (*iter)->is_preserve()? 1:0, (*iter)->is_false_edge()? 1: 0,
                (*iter)->get_weight(), x, y, z);
    }
    fclose(fp);

}



