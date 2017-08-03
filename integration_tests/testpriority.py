import libNeuroProofPriority as npp
import sys

graph_json = sys.argv[1]
output_path = sys.argv[2]
groundtruth_path = sys.argv[3]

status = npp.initialize_priority_scheduler(graph_json, .1, .9, .5)
assert status, "initialize_priority_scheduler() failed."

npp.set_edge_mode(0.1, 0.9, 0.5)

num_examined = 0
while npp.get_estimated_num_remaining_edges() > 0:
    priority_info = npp.get_next_edge()
    xyzlocation = priority_info.location
    (body1, body2) = priority_info.body_pair

    # proofread body pair at location
    
    npp.set_edge_result(priority_info.body_pair, False)
    num_examined += 1

assert num_examined == 610

npp.export_priority_scheduler(output_path)

tempout = open(output_path).read()
groundout = open(groundtruth_path).read()

assert tempout == groundout, \
    "Computed output ({}) doesn't match golden output ({})."\
    .format(output_path, groundtruth_path)
    
print("SUCCESS")
