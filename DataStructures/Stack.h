#include <vector>
#include <tr1/unordered_map>
#include "Rag.h"
#include "../Algorithms/RagAlgs.h"

namespace NeuroProof {

typedef unsigned int Label;

class Stack {
  public:
    Stack(unsigned int* watershed_, double* prediction_array_, int depth_, int height_, int width_, int padding_=1) : watershed(watershed_), prediction_array(predictions_array_), depth(depth_), height(height_), width(width_), padding(padding_)
    {
        rag = new Rag();
    }

    // ?!will need to add genericity for multiple pixel maps and different features
    void build_rag();

    void agglomerate_rag(double threshold);

    // ?! how to convert to numpy array
    unsigned int * get_label_volume()
    {
        return 0;
    }

    int num_bodies()
    {
        return rag->get_num_regions();        
    }


  private:
    Rag<Label> * rag;
    unsigned int* watershed;
    double* prediction_array;
    std::tr1::unordered_map<Label, Label> watershed_to_body; 
    std::tr1::unordered_map<Label, vector<Label> > merge_history; 
    int depth, height, width;
    int padding;
};


void Stack::build_rag()
{
    boost::shared_ptr<PropertyList<Label> > edge_list = EdgePropertyList<Label>::create_edge_list();
    rag->bind_property_list("median", edge_list);

    for (unsigned int z = 1; z < (depth-1); ++z) {
        int z_spot = z * plane_size;
        for (unsigned int y = 1; y < (height-1); ++y) {
            int y_spot = y * width;
            for (unsigned int x = 1; x < (width-1); ++x) {
                unsigned long long curr_spot = x + y_spot + z_spot;
                unsigned int spot0 = watershed_array[curr_spot];
                unsigned int spot1 = watershed_array[curr_spot-1];
                unsigned int spot2 = watershed_array[curr_spot+1];
                unsigned int spot3 = watershed_array[curr_spot-width];
                unsigned int spot4 = watershed_array[curr_spot+width];
                unsigned int spot5 = watershed_array[curr_spot-plane_size];
                unsigned int spot6 = watershed_array[curr_spot+plane_size];

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
        for (unsigned int y = 1; y < (height-1); y+=(height-3)) {
            int y_spot = y * width;
            for (unsigned int x = 1; x < (width-1); x+=(width-3)) {
                unsigned long long curr_spot = x + y_spot + z_spot;
                property_list_add_template_property(node_list, rag->find_rag_node(watershed_array[curr_spot]), true);
            }
        }
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
        merge_history[node1].insert(merge_history[node1].end(), merge_history[node2].begin(), merge_history[node2].end());
        merge_history.erase(node2);
    }

}




}
