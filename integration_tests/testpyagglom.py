import sys
import platform
import h5py
import numpy

segh5 = sys.argv[1]
predh5 = sys.argv[2]
classifier = sys.argv[3]
threshold = float(sys.argv[4])

from neuroproof import Agglomeration

# open as uint32 and float respectively
seg = numpy.array(h5py.File(segh5)['stack'], numpy.uint32)
pred = numpy.array(h5py.File(predh5)['volume/predictions'], numpy.float32)

pred = pred.transpose((2,1,0,3))
pred = pred.copy()

res = Agglomeration.agglomerate(seg, pred, classifier, threshold)

# The 'golden' results depend on std::unordered, and therefore
# the expected answer is different on Mac and Linux.
if platform.system() == "Darwin":
    expected_unique = [239]
else:
    # Depending on which linux stdlib we use, we might get different results
    expected_unique = [232, 233]
    
result_unique = len(numpy.unique(res))
assert result_unique in expected_unique, \
    "Wrong number of unique labels in the segmentation. Expected one of {}, but got {}"\
    .format(expected_unique, len(numpy.unique(res)))

print("SUCCESS")
