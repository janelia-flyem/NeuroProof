#include "RagUtils.h"
#include "Rag.h"
#include "RagIO.h"
#include "../Utilities/ErrMsg.h"

#include <json/json.h>
#include <json/value.h>
#include <iostream>
#include <fstream>
#include <string>
#include <boost/tuple/tuple.hpp>

using std::cout; using std::endl; using std::ifstream; using std::ofstream;
using std::string;

namespace NeuroProof {

typedef boost::tuple<unsigned int, unsigned int, unsigned int> Location;

Rag<Label>* create_rag_from_jsonfile(const char * file_name)
{
    try {
        Json::Reader json_reader;
        Json::Value json_reader_vals;
        ifstream fin(file_name);
        if (!fin) {
            throw ErrMsg("Error: input file: " + string(file_name) + " cannot be opened");
        }
     
        if (!json_reader.parse(fin, json_reader_vals)) {
            throw ErrMsg("Error: Json incorrectly formatted");
        }
        fin.close();
        return create_rag_from_json(json_reader_vals);
    } catch (ErrMsg& msg) {
        cout << msg.str << endl;
    }  

    return 0;
}

Rag<Label>* create_rag_from_json(Json::Value& json_reader_vals)
{
    Rag<Label>* rag = 0;
    try {
        rag = new Rag<Label>;
        Json::Value edge_list = json_reader_vals["edge_list"];

        if (edge_list.size() == 0) {
            return 0;
        }

        for (int i = 0; i < edge_list.size(); ++i) {
            Label node1 = edge_list[i]["node1"].asUInt();
            Label node2 = edge_list[i]["node2"].asUInt();
            unsigned int size1 = edge_list[i].get("size1", 1).asUInt();
            unsigned int size2 = edge_list[i].get("size2", 1).asUInt();
            double weight = edge_list[i].get("weight", 0.0).asDouble();
            unsigned int edge_size = edge_list[i].get("edge_size", 5).asUInt();
            
            bool preserve = edge_list[i].get("preserve", false).asUInt();
            bool false_edge = edge_list[i].get("false_edge", false).asUInt();


            RagNode<Label>* rag_node1 = rag->find_rag_node(node1);
            if (!rag_node1) {
                rag_node1 = rag->insert_rag_node(node1);
                rag_node1->set_size(size1);
            }
            RagNode<Label>* rag_node2 = rag->find_rag_node(node2);
            if (!rag_node2) {
                rag_node2 = rag->insert_rag_node(node2);
                rag_node2->set_size(size2);
            }
           
            if (!rag->find_rag_edge(rag_node1, rag_node2)) {
                RagEdge<Label>* rag_edge = rag->insert_rag_edge(rag_node1, rag_node2);
                rag_edge->set_weight(weight);
          
                unsigned int x, y, z;
                Json::Value location = edge_list[i]["location"];
                if (!location.empty()) {
                    //throw ErrMsg("location field does not exist for edge");
                    x = location[(unsigned int)(0)].asUInt();
                    y = location[(unsigned int)(1)].asUInt();
                    z = location[(unsigned int)(2)].asUInt();
                    rag_edge->set_property("location", Location(x,y,z));
                }

                rag_edge->set_preserve(preserve);
                rag_edge->set_false_edge(false_edge);

                rag_edge->set_property("edge_size", edge_size);                
            } 
        }

    } catch (ErrMsg& msg) {
        cout << msg.str << endl;
        if (rag) {
            delete rag;
            rag = 0;
        }
    }  

    return rag;
}


bool create_jsonfile_from_rag(Rag<Label>* rag, const char * file_name)
{
    try {
        Json::Value json_writer;
        ofstream fout(file_name);
        if (!fout) {
            throw ErrMsg("Error: output file could not be opened");
        }
        
        bool status = create_json_from_rag(rag, json_writer);
        if (!status) {
            throw ErrMsg("Error in rag export");
        }

        fout << json_writer; 
        fout.close();
    } catch (ErrMsg& msg) {
        cout << msg.str << endl;
        return false;
    }

    return true;
}

bool create_json_from_rag(Rag<Label>* rag, Json::Value& json_writer, bool debug_mode)
{
    try {
        int edge_num = -1;
        for (Rag<Label>::edges_iterator iter = rag->edges_begin();
            iter != rag->edges_end(); ++iter)
        {
            if (debug_mode && !((*iter)->is_preserve())) {
                continue;
            }
            ++edge_num;
            Json::Value json_edge;
            json_edge["node1"] = (*iter)->get_node1()->get_node_id();
            json_edge["node2"] = (*iter)->get_node2()->get_node_id();
            json_edge["size1"] = (unsigned int)((*iter)->get_node1()->get_size());
            json_edge["size2"] = (unsigned int)((*iter)->get_node2()->get_size());
            json_edge["weight"] = (*iter)->get_weight();
            json_edge["preserve"] = (*iter)->is_preserve();           
            json_edge["false_edge"] = (*iter)->is_false_edge();           

            try {
                Location location = (*iter)->get_property<Location>("location");
                json_edge["location"][(unsigned int)(0)] = boost::get<0>(location); 
                json_edge["location"][(unsigned int)(1)] = boost::get<1>(location); 
                json_edge["location"][(unsigned int)(2)] = boost::get<2>(location); 
            } catch (ErrMsg& msg) {
                //
            }
          
            try { 
                unsigned int edge_size = (*iter)->get_property<unsigned int>("edge_size");
                json_edge["edge_size"] = edge_size;
            } catch (ErrMsg& msg) {
            }

            json_writer["edge_list"][edge_num] = json_edge; 
        }
    } catch (ErrMsg& msg) {
        cout << msg.str << endl;
        return false;
    }

    return true;
}



}
