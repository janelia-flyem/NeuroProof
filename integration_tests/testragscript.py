from libNeuroProofRag import *
import sys

rag = Rag()
node1 = rag.insert_rag_node(1)
node2 = rag.insert_rag_node(2)
node1.set_size(55)
node2.set_size(65)
edge1 = rag.insert_rag_edge(node1, node2)

if rag.get_num_regions() != 2:
    exit(1)

edge1.set_weight(3.6)

for node in node1:
    if node.get_size() != 65:
        exit(1)
    if node.get_node_id() != 2:
        exit(1)

for edge in node1.get_edges():
    if edge.get_weight() != 3.6:
        exit(1)

# tests node iterator
for node in rag.get_nodes():
    pass

# tests edge iterator
for edge in rag.get_edges():
    if edge.get_weight() != 3.6:
        exit(1)

rag2 = Rag(rag)
rag.remove_rag_edge(edge1)
rag.remove_rag_node(node1)

if rag.get_num_regions() != 1:
    exit(1)
    
del rag
create_jsonfile_from_rag(rag2, sys.argv[1])
del rag2

rag = create_rag_from_jsonfile(sys.argv[1])
if rag.get_num_regions() != 2:
    exit(1)

print "SUCCESS"
