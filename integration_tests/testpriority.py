import libNeuroProofPriority as npp
import sys

status = npp.initialize_priority_scheduler(sys.argv[1], .1, .9, .5)

num_examined = 0

if not status:
    exit(1)

npp.set_edge_mode(0.1, 0.9, 0.5)

while npp.get_estimated_num_remaining_edges() > 0:
    priority_info = npp.get_next_edge()
    xyzlocation = priority_info.location
    (body1, body2) = priority_info.body_pair

    # proofread body pair at location
    
    npp.set_edge_result(priority_info.body_pair, False)
    num_examined += 1

if num_examined != 610:
    exit(1)

npp.export_priority_scheduler(sys.argv[2])

tempfile = open(sys.argv[2])
groundfile = open(sys.argv[3])

tempout = tempfile.read()
groundout = groundfile.read()

if tempout != groundout:
    exit(1)
    
print "SUCCESS"
