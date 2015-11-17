#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE stack_capabilities

#include <boost/test/unit_test.hpp>

#include <Stack/VolumeLabelData.h>
#include <Stack/VolumeData.h>
#include <IO/StackIO.h>
#include <iostream>
#include <vector>
#include <tr1/unordered_set>

using namespace std;
using namespace NeuroProof;
using std::tr1::unordered_set;

BOOST_AUTO_TEST_CASE (stack_simple)
{
    char ** argv = boost::unit_test::framework::master_test_suite().argv;
    VolumeLabelPtr labels = import_h5labels(argv[1], "stack");
    vector<VolumeProbPtr> preds = import_3Dh5vol_array<Prob_t>(argv[2], "volume/predictions");

    BOOST_CHECK(6 == preds.size());
    BOOST_CHECK(labels->shape(0) == 100);
    BOOST_CHECK(labels->shape(1) == 200);
    BOOST_CHECK(labels->shape(2) == 50);

    BOOST_CHECK(preds[0]->shape(0) == 100);
    BOOST_CHECK(preds[1]->shape(1) == 200);
    BOOST_CHECK(preds[2]->shape(2) == 50);

    unordered_set<Label_t> label_set;
    volume_forXYZ(*labels,x,y,z) {
        label_set.insert((*labels)(x,y,z));
    }
    BOOST_CHECK(label_set.size() == 8180);
}
