#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE rag_capabilities

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

#include "../RagUtils.h"
#include "../Rag.h"
#include "../RagIO.h"

using namespace boost::unit_test_framework; 
using namespace NeuroProof;


BOOST_AUTO_TEST_SUITE (rag_simple)

BOOST_AUTO_TEST_CASE (rag_general)
{
    Rag_t* test_rag = new Rag_t();
    RagNode_t* node = test_rag->insert_rag_node(5);
    node->set_property("temp", int(9));
    int temp_var = node->get_property<int>("temp");
    BOOST_CHECK(9 == temp_var);

    RagNode_t* node2 = test_rag->insert_rag_node(6);
    RagNode_t* node3 = test_rag->insert_rag_node(7);

    node2->set_size(10);
    node->set_size(1);

    test_rag->insert_rag_edge(node2, node3);
    test_rag->insert_rag_edge(node, node3);
    test_rag->insert_rag_edge(node, node2);

    BOOST_CHECK(test_rag->get_rag_size() == 11);

    BOOST_CHECK(test_rag->get_num_regions() == 3);
    BOOST_CHECK(test_rag->get_num_edges() == 3);

    Rag_t* test_rag2 = new Rag_t(*test_rag);
    
    node->rm_property("temp");
    test_rag->remove_rag_node(node);
    
    BOOST_CHECK(test_rag->get_num_regions() == 2);
    BOOST_CHECK(test_rag->get_num_edges() == 1);
    
    delete test_rag;
    
    RagNode_t* node_temp = test_rag2->find_rag_node(5);
    temp_var = node_temp->get_property<int>("temp");
    BOOST_CHECK(9 == temp_var);
    BOOST_CHECK(test_rag2->get_num_regions() == 3);
    BOOST_CHECK(test_rag2->get_num_edges() == 3);


    delete test_rag2;
}


BOOST_AUTO_TEST_CASE (rag_node_combine)
{
    Rag_t* test_rag = new Rag_t();
    RagNode_t* node = test_rag->insert_rag_node(5);
    node->set_property("temp", int(9));

    RagNode_t* node2 = test_rag->insert_rag_node(6);
    RagNode_t* node3 = test_rag->insert_rag_node(7);

    node2->set_size(10);
    node->set_size(1);
    
    test_rag->insert_rag_edge(node2, node3);
    test_rag->insert_rag_edge(node, node3);
    test_rag->insert_rag_edge(node, node2);

    rag_join_nodes(*test_rag, node2, node, 0);

    int temp_var = 0;
    try {
        temp_var = node2->get_property<int>("temp");
    } catch (ErrMsg& msg) {
    }
    
    BOOST_CHECK(0 == temp_var);
    BOOST_CHECK(11 == node2->get_size());

    BOOST_CHECK(test_rag->get_num_regions() == 2);
    BOOST_CHECK(test_rag->get_num_edges() == 1);

    delete test_rag;
}

#include <iostream>

BOOST_AUTO_TEST_CASE (rag_json_create)
{
    Json::Value json_vals;
    Json::Value json_edge;

    json_edge["node1"] = 5; 
    json_edge["node2"] = 9; 
    json_edge["size1"] = 2000; 
    json_edge["size2"] = 1500; 
    json_edge["weight"] = 0.3; 
    json_vals["edge_list"][(unsigned int) 0] = json_edge;
    
    json_edge["node1"] = 5; 
    json_edge["node2"] = 19; 
    json_edge["size1"] = 2000; 
    json_edge["size2"] = 3500; 
    json_edge["weight"] = 0.5; 
    json_vals["edge_list"][(unsigned int) 1] = json_edge;
    
    Rag_t* rag = create_rag_from_json(json_vals);


    BOOST_CHECK(rag->get_num_regions() == 3);
    BOOST_CHECK(rag->get_num_edges() == 2);
    BOOST_CHECK(rag->get_rag_size() == 7000);

    RagEdge_t* edge = rag->find_rag_edge(5, 9);
    BOOST_CHECK_CLOSE(edge->get_weight(), 0.3, 0.000001);
    
    delete rag;
}


BOOST_AUTO_TEST_SUITE_END()


