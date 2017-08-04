#include <Stack/Stack.h>
#include <boost/python.hpp>

using namespace NeuroProof;
using namespace boost::python;
using std::unordered_map;

// ?! inherit properly from Stack (reuse some implementation, etC)
class StackPython {
  public:
    StackPython(object stack_labels, int padding_radius)
    {
        boost::python::tuple labels_shape(stack_labels.attr("shape"));

        unsigned int width = boost::python::extract<unsigned int>(labels_shape[2]);
        unsigned int height = boost::python::extract<unsigned int>(labels_shape[1]);
        unsigned int depth = boost::python::extract<unsigned int>(labels_shape[0]);

        labels = VolumeLabelData::create_volume();
        labels->reshape(vigra::MultiArrayShape<3>::type(width, height, depth));

        volume_forXYZ(*labels,x,y,z) {
            labels->set(x,y,z,
                    boost::python::extract<double>(stack_labels[boost::python::make_tuple(z,y,x)]));
        }

        Stack stack(labels);
        if (padding_radius > 0) stack.dilate_labelvol(padding_radius);
        labels = stack.get_labelvol();
    }

    boost::python::list find_overlaps(StackPython* stack2)
    {
        unordered_map<Label_t, unordered_map<Label_t, unsigned int> > labelcounts;

        volume_forXYZ(*labels,x,y,z) {
            Label_t label1 = (*labels)(x,y,z);
            Label_t label2 = (*(stack2->labels))(x,y,z);

            if (!label1 || !label2) {
                continue;
            }

            labelcounts[label1][label2]++;
        }
       
        // ?! write overlaps 
        boost::python::list overlaps;
        for (unordered_map<Label_t, unordered_map<Label_t, unsigned int> >::iterator iter =
                labelcounts.begin(); iter != labelcounts.end(); ++iter) {
       
            for (unordered_map<Label_t, unsigned int>::iterator iter2 = iter->second.begin();
                    iter2 != iter->second.end(); ++iter2) {
                overlaps.append(boost::python::make_tuple(iter->first, iter2->first, iter2->second));
            }
            
        }

        return overlaps;
    }

  private:
    VolumeLabelPtr labels;
};

//    class_<StackPython>("Stack", "Contains segmentation labels", init<object>(args("labels"), "Provide numpy labels to init Stack")[])
BOOST_PYTHON_MODULE(libNeuroProofMetrics)
{
    class_<StackPython>("Stack", "Contains segmentation labels", init<object, int>())
        .def("find_overlaps", &StackPython::find_overlaps)
        ;
}
