import json
import sys

# <prog> <big bodies> <syn bodies> <affinity> <body thres> <syn threshold> <mapping>

bigbodies = json.load(open(sys.argv[1]))
synbodies = json.load(open(sys.argv[2]))
paths = json.load(open(sys.argv[3]))
bodythres = float(sys.argv[4])
synthres = float(sys.argv[5])
gtadjacency = json.load(open(sys.argv[6]))

print("opened files")

# extract important bodies
important_bodies = set()
cumval = 0
for val in bigbodies:
    if cumval > bodythres:
        break
    important_bodies.add(val[0])
    cumval += val[1]


# add remaining synapses to other list
remaining_synbodies = {}

cumval = 0
for val in synbodies:
    if cumval > synthres:
        if val[0] not in important_bodies:
            remaining_synbodies[val[0]] = val[1]
    else:
        important_bodies.add(val[0])
    cumval += val[1]

# grab on paths that contain these endpoints, sort from highest to lowest confidence
sorted_paths = []
for path in paths:
    if (path[0] in important_bodies or path[1] in important_bodies) and (path[0] in remaining_synbodies or path[1] in remaining_synbodies):
        sorted_paths.append((path[2], path[0], path[1]))

sorted_paths.sort(reverse=True)
print("Total paths to merged bodies: ", len(sorted_paths))

# load gt maps
num_seg = len(remaining_synbodies)
gtbodies = set()
gt_mappings = {}

for seg2gt in gtadjacency:
    if seg2gt[0] in important_bodies:
        gtbodies.add(seg2gt[1])
        gt_mappings[seg2gt[0]] = seg2gt[1]
    if seg2gt[0] in remaining_synbodies:
        gt_mappings[seg2gt[0]] = seg2gt[1]

num_gt = len(gtbodies)
print("Syn fragments: ", num_seg, "GT big bodies covered: ", num_gt)

# iterate paths in order

num_correct = 0
num_incorrect = 0
num_orphan = 0

examined = {}
depth = 2
size_correct = 0
size_incorrect = 0
size_orphan = 0

for iter1, path in enumerate(sorted_paths):
    # only examine each synapse site once
    synsize = 0
    if path[1] in remaining_synbodies:
        if path[1] in examined and examined[path[1]] == depth:
            continue
        else:
            synsize = remaining_synbodies[path[1]]
            if path[1] not in examined:
                examined[path[1]] = 1
            else:
                examined[path[1]] += 1
    elif path[2] in examined and examined[path[2]] == depth:
        continue
    else:
        synsize = remaining_synbodies[path[2]]
        if path[2] not in examined:
            examined[path[2]] = 1
        else:
            examined[path[2]] += 1



    if path[1] not in gt_mappings or path[2] not in gt_mappings:
        continue

    if gt_mappings[path[1]] != gt_mappings[path[2]]:
        # check if there are any matches
        if gt_mappings[path[1]] not in gtbodies or gt_mappings[path[1]] not in gtbodies:
            num_orphan += 1
            size_orphan += synsize
        else:
            # no merge
            num_incorrect += 1
            size_incorrect += synsize

    else:
        num_correct += 1
        size_correct += synsize
        num_seg -= 1
        
    
    if (iter1 % 10) == 0:
        # merge yes and size, merge no and size, orphans and size, remaining segs, optimal GT, confidence
        print(num_correct, size_correct, num_incorrect, size_incorrect, num_orphan, size_orphan, num_seg, num_gt, path[0])

print("Final Results (yes count and size, no count and size, orphan count and size, #syn bodies left, #gt)")
print(num_correct, size_correct, num_incorrect, size_incorrect, num_orphan, size_orphan, num_seg, num_gt)






