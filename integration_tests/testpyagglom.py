import sys
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

expected_unique = 239
result_unique = len(numpy.unique(res))
assert result_unique == 239, \
    "Expected {} unique labels (including 0) in the resulting segmentation, but got {}"\
    .format(expected_unique, len(numpy.unique(res)))

print("SUCCESS")
