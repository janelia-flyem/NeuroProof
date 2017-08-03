import libNeuroProofPriority as npp
import sys
import random

# read file
status = npp.initialize_priority_scheduler(sys.argv[1], .1, .9, .5)
print(status)


# body mode
#npp.set_edge_mode(0.1, 0.9, 0.5)
num_examined = 0
npp.set_body_mode(25000, 0)

decision = 0
undo = 0
num_undo = 0

npp.estimate_work()
estimated = npp.get_estimated_num_remaining_edges()

print(("Num estimated edges: ", estimated))

while npp.get_estimated_num_remaining_edges() > 0:
    priority_info = npp.get_next_edge()
    xyzlocation = priority_info.location
    (body1, body2) = priority_info.body_pair

    # proofread body pair at location
    npp.set_edge_result(priority_info.body_pair, decision%2)
    decision+=1
    num_examined += 1
    undo += 1
    if not undo%5:
	num_examined -= 1
	decision -= 1
	status = npp.undo()
	if not status:
	    print("Won't undo")
	    decision += 1
	    num_examined += 1
	else:
	    num_undo += 1
    #npp.set_body_mode(25000, 0)
print(("Num body: ", num_examined))
print(("Num undo: ", num_undo))

# synapse mode
#npp.set_edge_mode(0.1, 0.9, 0.5)
num_examined = 0
npp.set_synapse_mode(0.1)

num_undo = 0
while npp.get_estimated_num_remaining_edges() > 0:
    priority_info = npp.get_next_edge()
    xyzlocation = priority_info.location
    (body1, body2) = priority_info.body_pair

    # proofread body pair at location
    npp.set_edge_result(priority_info.body_pair, decision%2)
    decision+=1
    num_examined += 1
    undo += 1
    if not undo%5:
	status = npp.undo()
	decision -= 1
    	num_examined -= 1
	if not status:
	    print("Won't undo")
	    decision += 1
	    num_examined += 1
	else:
	    num_undo += 1
    #npp.set_synapse_mode(0.1)

print(("Num synapse ", num_examined))
print(("Num undo: ", num_undo))

# dump graph
npp.export_priority_scheduler(sys.argv[2])
tempfile = open(sys.argv[2])
tempout = tempfile.read()
print("SUCCESS")
