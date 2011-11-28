#!/usr/bin/python

from libNeuroProofRag import *


rag = Rag()
node1 = rag.insert_rag_node(1)
node2 = rag.insert_rag_node(2)
node1.set_size(55)
node2.set_size(65)
edge1 = rag.insert_rag_edge(node1, node2)
print "Num regions: ", rag.get_num_regions()
edge1.set_weight(3.6)

for node in node1:
    print "Node info: ", node.get_node_id(), " ", node.get_size()

for edge in node1.get_edges():
    print "Edge info: ", edge.get_weight()

for node in rag.get_nodes():
    print "Node info: ", node.get_node_id()

for edge in rag.get_edges():
    print "Edge info: ", edge.get_weight()

rag2 = Rag(rag)
rag.remove_rag_edge(edge1)
rag.remove_rag_node(node1)
print "Num regions after deletion: ", rag.get_num_regions()
del rag
create_jsonfile_from_rag(rag2, "temp.json")
del rag2

rag = create_rag_from_jsonfile("temp.json")
print "Num regions after restore: ", rag.get_num_regions()
