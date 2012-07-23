#include <vector>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include "Rag.h"
#include "../Algorithms/RagAlgs.h"
#include "AffinityPair.h"

#ifndef STACK_H
#define STACK_H

namespace NeuroProof {

typedef unsigned int Label;

class Stack {
  public:
    Stack(Label* watershed_, double* prediction_array_, int depth_, int height_, int width_, int padding_=1) : watershed(watershed_), prediction_array(prediction_array_), depth(depth_), height(height_), width(width_), padding(padding_)
    {
        rag = new Rag<Label>();
    }

    // ?!will need to add genericity for multiple pixel maps and different features
    void build_rag();

    void agglomerate_rag(double threshold);

    Label * get_label_volume()
    {
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

    int get_num_bodies()
    {
        return rag->get_num_regions();        
    }

    int get_width() const
    {
        return width;
    }

    int get_height() const
    {
        return height;
    }

    int get_depth() const
    {
        return depth;
    }

    int remove_inclusions();

    ~Stack()
    {
        delete rag;
        delete prediction_array;
        delete watershed;
    }

  private:
    
    int biconnected_dfs(int count, Label previous, RagNode<Label>* rag_node, boost::shared_ptr<PropertyList<Label> > node_properties, std::tr1::unordered_set<Label>& visited, std::tr1::unordered_map<Label, int>& node_depth, std::vector<OrderedPair<Label> >& stack, std::vector<std::vector<OrderedPair<Label> > >& biconnected_components);
    
    
    Rag<Label> * rag;
    Label* watershed;
    double* prediction_array;
    std::tr1::unordered_map<Label, Label> watershed_to_body; 
    std::tr1::unordered_map<Label, std::vector<Label> > merge_history; 
    int depth, height, width;
    int padding;
};


void Stack::build_rag()
{
    boost::shared_ptr<PropertyList<Label> > edge_list = EdgePropertyList<Label>::create_edge_list();
    rag->bind_property_list("median", edge_list);
    unsigned int plane_size = width * height;

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

                if (spot1 && (spot0 != spot1)) {
                    rag_add_edge(rag, spot0, spot1, prediction_array[curr_spot], edge_list);
                }
                if (spot2 && (spot0 != spot2)) {
                    rag_add_edge(rag, spot0, spot2, prediction_array[curr_spot], edge_list);
                }
                if (spot3 && (spot0 != spot3)) {
                    rag_add_edge(rag, spot0, spot3, prediction_array[curr_spot], edge_list);
                }
                if (spot4 && (spot0 != spot4)) {
                    rag_add_edge(rag, spot0, spot4, prediction_array[curr_spot], edge_list);
                }
                if (spot5 && (spot0 != spot5)) {
                    rag_add_edge(rag, spot0, spot5, prediction_array[curr_spot], edge_list);
                }
                if (spot6 && (spot0 != spot6)) {
                    rag_add_edge(rag, spot0, spot6, prediction_array[curr_spot], edge_list);
                }
            }
        }
    } 

    boost::shared_ptr<PropertyList<Label> > node_list = NodePropertyList<Label>::create_node_list();
    rag->bind_property_list("border_node", node_list);

    for (unsigned int z = 1; z < (depth-1); z+=(depth-3)) {
        int z_spot = z * plane_size;
        for (unsigned int y = 1; y < (height-1); ++y) {
            int y_spot = y * width;
            for (unsigned int x = 1; x < (width-1); ++x) {
                unsigned long long curr_spot = x + y_spot + z_spot;
                property_list_add_template_property(node_list, rag->find_rag_node(watershed[curr_spot]), true);
            }
        }
    }
    for (unsigned int z = 1; z < (depth-1); ++z) {
        int z_spot = z * plane_size;
        for (unsigned int y = 1; y < (height-1); y+=(height-3)) {
            int y_spot = y * width;
            for (unsigned int x = 1; x < (width-1); ++x) {
                unsigned long long curr_spot = x + y_spot + z_spot;
                property_list_add_template_property(node_list, rag->find_rag_node(watershed[curr_spot]), true);
            }
        }
    }
    for (unsigned int z = 1; z < (depth-1); ++z) {
        int z_spot = z * plane_size;
        for (unsigned int y = 1; y < (height-1); ++y) {
            int y_spot = y * width;
            for (unsigned int x = 1; x < (width-1); x+=(width-3)) {
                unsigned long long curr_spot = x + y_spot + z_spot;
                property_list_add_template_property(node_list, rag->find_rag_node(watershed[curr_spot]), true);
            }
        }
    }   

    for (Rag<Label>::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
        Label id = (*iter)->get_node_id();
        watershed_to_body[id] = id;
    }

}



void Stack::agglomerate_rag(double threshold)
{
    EdgeRank_t ranking;
    const double Epsilon = 0.00001;

    boost::shared_ptr<PropertyList<Label> > median_properties = rag->retrieve_property_list("median");
    boost::shared_ptr<PropertyList<Label> > node_properties = rag->retrieve_property_list("border_node");

    for (Rag<Label>::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
        boost::shared_ptr<PropertyMedian> median_property = boost::shared_polymorphic_downcast<PropertyMedian>(median_properties->retrieve_property(*iter));
        double val = median_property->get_data();

        //        cout << val  << " " << (*iter)->get_node1()->get_node_id() << " " << (*iter)->get_node2()->get_node_id() << std::endl;
        if (val <= threshold) {
            ranking.insert(std::make_pair(val, std::make_pair((*iter)->get_node1()->get_node_id(), (*iter)->get_node2()->get_node_id())));
        }
    }    

    while (!ranking.empty()) {
        EdgeRank_t::iterator first_entry = ranking.begin();
        double curr_threshold = (*first_entry).first;
        Label node1 = (*first_entry).second.first;
        Label node2 = (*first_entry).second.second;
        ranking.erase(first_entry);

        //cout << curr_threshold << " " << node1 << " " << node2 << std::endl;

        if (curr_threshold > threshold) {
            break;
        }

        RagNode<Label>* rag_node1 = rag->find_rag_node(node1); 
        RagNode<Label>* rag_node2 = rag->find_rag_node(node2); 

        if (!(rag_node1 && rag_node2)) {
            continue;
        }
        RagEdge<Label>* rag_edge = rag->find_rag_edge(rag_node1, rag_node2);

        if (!rag_edge) {
            continue;
        }

        boost::shared_ptr<PropertyMedian> median_property = boost::shared_polymorphic_downcast<PropertyMedian>(median_properties->retrieve_property(rag_edge));
        double val = median_property->get_data();
        if (val > (curr_threshold + Epsilon)) {
            continue;
        } 

        rag_merge_edge_median(*rag, rag_edge, rag_node1, median_properties, node_properties, ranking);
        watershed_to_body[node2] = node1;
        for (std::vector<Label>::iterator iter = merge_history[node2].begin(); iter != merge_history[node2].end(); ++iter) {
            watershed_to_body[*iter] = node1;
        }
        merge_history[node1].push_back(node2);
        merge_history[node1].insert(merge_history[node1].end(), merge_history[node2].begin(), merge_history[node2].end());
        merge_history.erase(node2);
    }

}

int Stack::remove_inclusions()
{
    int num_removed = 0;

    std::tr1::unordered_set<Label> visited;
    std::tr1::unordered_map<Label, int> node_depth;
    std::vector<std::vector<OrderedPair<Label> > > biconnected_components; 
    std::vector<OrderedPair<Label> > stack;

    boost::shared_ptr<PropertyList<Label> > node_properties = rag->retrieve_property_list("border_node");

    RagNode<Label>* rag_node = 0;
    for (Rag<Label>::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
        bool border = false;
        try {
            border = property_list_retrieve_template_property<Label, bool>(node_properties, *iter);
        } catch (...) {
            //
        }

        if (border) {
            rag_node = *iter;
        }
    }
    assert(rag_node);

    biconnected_dfs(0, 0, rag_node, node_properties, visited, node_depth, stack, biconnected_components);
    stack.clear();
    std::tr1::unordered_map<Label, Label> body_to_body; 
    std::tr1::unordered_map<Label, std::vector<Label> > merge_history2; 

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
            unsigned long long total_size = 0;
            RagNode<Label>* articulation_node = rag->find_rag_node(articulation_region);
            for (std::tr1::unordered_set<Label>::iterator iter = merge_nodes.begin(); iter != merge_nodes.end(); ++iter) {
                Label region1 = *iter;
                RagNode<Label>* rag_node = rag->find_rag_node(region1);
                total_size += rag_node->get_size();

            }
            articulation_node->set_size(total_size);
           
            for (std::tr1::unordered_set<Label>::iterator iter = merge_nodes.begin(); iter != merge_nodes.end(); ++iter) {
                Label region2 = *iter;
                if (body_to_body.find(region2) != body_to_body.end()) {
                    region2 = body_to_body[region2];
                }

                RagNode<Label>* rag_node = rag->find_rag_node(region2);
                assert(rag_node);
                if (articulation_node != rag_node) {
                    rag->remove_rag_node(rag_node); 
                    
                    watershed_to_body[region2] = articulation_region;
                    for (std::vector<Label>::iterator iter2 = merge_history[region2].begin(); iter2 != merge_history[region2].end(); ++iter2) {
                        watershed_to_body[*iter2] = articulation_region;
                    }

                    body_to_body[region2] = articulation_region;
                    for (std::vector<Label>::iterator iter2 = merge_history2[region2].begin(); iter2 != merge_history2[region2].end(); ++iter2) {
                        body_to_body[*iter2] = articulation_region;
                    }

                    merge_history[articulation_region].push_back(region2);
                    merge_history[articulation_region].insert(merge_history[articulation_region].end(), merge_history[region2].begin(), merge_history[region2].end());
                    merge_history.erase(region2);
                   
                    merge_history2[articulation_region].push_back(region2); 
                    merge_history2[articulation_region].insert(merge_history2[articulation_region].end(), merge_history2[region2].begin(), merge_history2[region2].end());
                    merge_history2.erase(region2);
                }
            }
        }
    }

    // ??!! accumulate weights through all of them and delete all nodes except for one with outside edges

    return num_removed;
}

// return low point (0 is default edge)
int Stack::biconnected_dfs(int count, Label previous, RagNode<Label>* rag_node, boost::shared_ptr<PropertyList<Label> > node_properties, std::tr1::unordered_set<Label>& visited, std::tr1::unordered_map<Label, int>& node_depth, std::vector<OrderedPair<Label> >& stack, std::vector<std::vector<OrderedPair<Label> > >& biconnected_components)
{
    visited.insert(rag_node->get_node_id());
    count += 1;
    bool border = false;
    try {
        border = property_list_retrieve_template_property<Label, bool>(node_properties, rag_node);
    } catch (...) {
        //
    }
    node_depth[rag_node->get_node_id()] = count;
    int low_count = count;

    for (RagNode<Label>::node_iterator iter = rag_node->node_begin(); iter != rag_node->node_end(); ++iter) {
        if (visited.find((*iter)->get_node_id()) == visited.end()) {
            OrderedPair<Label> current_edge(rag_node->get_node_id(), (*iter)->get_node_id());
            stack.push_back(current_edge);
            int temp_low = biconnected_dfs(count, rag_node->get_node_id(), (*iter), node_properties, visited, node_depth, stack, biconnected_components);
            low_count = std::min(low_count, temp_low);
            if (temp_low >= count) {
                OrderedPair<Label> popped_edge;
                biconnected_components.push_back(std::vector<OrderedPair<Label> >());
                do {
                    popped_edge = stack.back();
                    stack.pop_back();
                    biconnected_components[biconnected_components.size()-1].push_back(popped_edge);
                } while (!(popped_edge == current_edge));
                OrderedPair<Label> articulation_pair(rag_node->get_node_id(), rag_node->get_node_id());
                biconnected_components[biconnected_components.size()-1].push_back(articulation_pair);
            } 

        } else if ((*iter)->get_node_id() != previous) {
            low_count = std::min(low_count, node_depth[(*iter)->get_node_id()]);
            if (count > node_depth[(*iter)->get_node_id()]) {
                stack.push_back(OrderedPair<Label>(rag_node->get_node_id(), (*iter)->get_node_id()));
            }
        }
    }

    if (previous && border) {
        low_count = 0;
        stack.push_back(OrderedPair<Label>(0, rag_node->get_node_id()));
    }

    return low_count;
}


}

#endif
