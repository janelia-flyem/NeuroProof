
#include <Python.h>
#include <boost/python.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <Classifier/vigraRFclassifier.h>
#include <Classifier/opencvRFclassifier.h>

using namespace boost::python;
using namespace boost::algorithm;

namespace NeuroProof { namespace python {

class ClassifierPy {
  public:
    ClassifierPy(std::string fn)
    {
        if (ends_with(fn, ".h5")) {
            eclfr = new VigraRFclassifier(fn.c_str());	
        } else if (ends_with(fn, ".xml")) {	
            eclfr = new OpencvRFclassifier(fn.c_str());	
        }
    }
    ~ClassifierPy()
    {
        delete eclfr;
    }

  private:
    EdgeClassifier* eclfr;
};


BOOST_PYTHON_MODULE(_classifier_python)
{
    class_<ClassifierPy>("Classifier", init<std::string>());
}


}}


