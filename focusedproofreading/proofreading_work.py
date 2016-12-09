import json
import sys

# <prog> <big bodies> <syn bodies> <affinity> <body thres> <syn threshold> <mapping>

bigbodies = json.load(open(sys.argv[1]))
synbodies = json.load(open(sys.argv[2]))
paths = json.load(open(sys.argv[3]))
bodythres = float(sys.argv[4])
synthres = float(sys.argv[5])
gtadjacency = json.load(open(sys.argv[6]))

print "Opened all files"

# extract important bodies
important_bodies = set()
cumval = 0
for val in bigbodies:
    if cumval > bodythres:
        break
    important_bodies.add(val[0])
    cumval += val[1]

cumval = 0
for val in synbodies:
    if cumval > synthres:
        break
    important_bodies.add(val[0])
    cumval += val[1]

# load gt maps
num_seg = len(important_bodies)
gtbodies = set()

gt_mappings = {}
merge_history = {} # dictionary of sets

current_mappings = {}
for body in important_bodies:
    merge_history[body] = set([body])
    current_mappings[body] = body

for seg2gt in gtadjacency:
    if seg2gt[0] in important_bodies:
        gtbodies.add(seg2gt[1])
        gt_mappings[seg2gt[0]] = seg2gt[1]


remove_bodies = []
for body in important_bodies:
    if body not in gt_mappings:
        print "Orphan: ", body
        remove_bodies.append(body)

for body in remove_bodies:
    important_bodies.remove(body)

num_gt = len(gtbodies)
print "Seg bodies: ", num_seg, "GT bodies: ", num_gt

# grab on paths that contain these endpoints, sort from highest to lowest confidence
sorted_paths = []
for path in paths:
    if path[0] in important_bodies and path[1] in important_bodies:
        sorted_paths.append((path[2], path[0], path[1]))

sorted_paths.sort(reverse=True)
print "Total paths: ", len(sorted_paths)


# iterate paths in order

#check gt mappings; if endpoint already merged ignore; if not merge check if mapping exists;

num_correct = 0
num_incorrect = 0
num_red = 0

for iter1, path in enumerate(sorted_paths):
    if gt_mappings[path[1]] != gt_mappings[path[2]]:
        # no merge
        num_incorrect += 1 
    else:
        # check if already merged
        if current_mappings[path[1]] == current_mappings[path[2]]:
            num_red += 1
        else:
            num_seg -= 1
            num_correct += 1
            current_body = current_mappings[path[1]]
            current_body2 = current_mappings[path[2]]
            # merge path[2] onto path[1]
            merge_history[current_body] = merge_history[current_body].union(merge_history[current_body2]) 
            
            # update current_mappings
            for body in merge_history[current_body2]:
                current_mappings[body] = current_body


    if iter1 % 10:
        # merge yes, merge no, redundant ignore, remaining segs, optimal GT, confidence
        print num_correct, num_incorrect, num_red, num_seg, num_gt, path[0]

print "Final Results (yes, no, redundant, #seg, #gt)"
print num_correct, num_incorrect, num_red, num_seg, num_gt






