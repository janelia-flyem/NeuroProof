# need to eventually get path lengths -- can just iterate with different thresholds for now

import json
import sys

# <prog> <graph> <body list>

important_bodies = set()

from neuroproof import FocusedProofreading

graph = FocusedProofreading.Graph(sys.argv[1])

bodies1 = json.load(open(sys.argv[2]))
#bodies2 = json.load(open(sys.argv[3]))

important_bodies = set()

for val in bodies1:
    important_bodies.add(val[0])
#for val in bodies2:
#    important_bodies.add(val[0])

num_bodies = len(important_bodies)
bad_bodies = []

all_paths = []
for iter1, body in enumerate(important_bodies):
    #if iter1 < 20483:
    #    continue
    print "start"
    path = graph.find_close_bodies(body,3,0.1) # grab to 10 percent certainty (infinite path size) (try limit to 1, 2, 3, etc) 
    print iter1, num_bodies
    #if iter1 > 10:
    #    break

    if path is not None:
        for element in path:
            # avoid duplicate paths
            if body < element[0] and element[0] in important_bodies:
                all_paths.append([body, element[0], element[1]])
    else:
        bad_bodies.append(body)

fout = open("allpaths.json", 'w')
fout.write(json.dumps(all_paths))

fout = open("nomatchsegbodies.json", 'w')
fout.write(json.dumps(bad_bodies))
