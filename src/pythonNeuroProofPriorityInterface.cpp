#include "pythonNeuroProofPriorityInterface.h"
#include "../Utilities/ErrMsg.h"
#include "../EdgeEditor/EdgeEditor.h"
#include "../Rag/RagIO.h"

#include <json/json.h>
#include <json/value.h>
#include <vector>

#include <fstream>
#include <boost/python.hpp>
//#include <Python.h>

using namespace NeuroProof;
using namespace boost::python;
using std::ifstream; using std::ofstream; using std::cout; using std::endl;
using std::vector;

typedef unsigned int Label;

static EdgeEditor* priority_scheduler = 0;
Rag_t* rag = 0;

// false if file does not exist or json is not properly formatted
// exception thrown if min, max, or start val are illegal
bool initialize_priority_scheduler(const char * json_file, double min_val, double max_val, double start_val)
{
    if ((min_val < 0) || (max_val > 1.0) || (min_val > max_val) || (start_val > max_val) || (start_val < min_val) ) {
        throw ErrMsg("Priority scheduler filter bounds not properly set");  
    }
    if (!priority_scheduler) {
        delete priority_scheduler;
    }
   
    ifstream fin(json_file);
    Json::Reader json_reader;
    Json::Value json_vals;
    if (!json_reader.parse(fin, json_vals)) {
        throw ErrMsg("Error: Json incorrectly formatted");
    }
    fin.close();

    rag = create_rag_from_json(json_vals);
    if (!rag) {
        return false;
    }

    priority_scheduler = new EdgeEditor(*rag, min_val, max_val, start_val, json_vals);
    //priority_scheduler->updatePriority();

    return true;
}

// false if file cannot be written
bool export_priority_scheduler(const char * json_file)
{
    if (!priority_scheduler) {
        throw ErrMsg("Scheduler not initialized");
    }
    
    try {
        Json::Value json_writer;
        ofstream fout(json_file);
        if (!fout) {
            throw ErrMsg("Error: output file could not be opened");
        }

        bool status = create_json_from_rag(rag, json_writer);
        if (!status) {
            throw ErrMsg("Error in rag export");
        }

        priority_scheduler->export_json(json_writer); 

        fout << json_writer; 
        fout.close();
    } catch (ErrMsg& msg) {
        cout << msg.str << endl;
        return false;
    }

    return true;
}

double get_average_prediction_error()
{
    return 0.0;
}

double get_percent_prediction_correct()
{
    return 0.0;
}


// number of edges yet to be processed in the scheduler
unsigned int get_estimated_num_remaining_edges()
{
    if (!priority_scheduler) {
        throw ErrMsg("Scheduler not initialized");
    }

    return priority_scheduler->getNumRemaining();
}

void set_synapse_mode(double ignore_size)
{
    if (!priority_scheduler) {
        throw ErrMsg("Scheduler not initialized");
    }
    priority_scheduler->set_synapse_mode(ignore_size);
}

void set_body_mode(double ignore_size, int depth)
{
    if (!priority_scheduler) {
        throw ErrMsg("Scheduler not initialized");
    }
    priority_scheduler->set_body_mode(ignore_size, depth);
}

void set_orphan_mode(double ignore_size, double threshold, bool synapse_orphan)
{
    if (!priority_scheduler) {
        throw ErrMsg("Scheduler not initialized");
    }
    priority_scheduler->set_orphan_mode(ignore_size);
}

void set_edge_mode(double lower, double upper, double start)
{
    if (!priority_scheduler) {
        throw ErrMsg("Scheduler not initialized");
    }
    priority_scheduler->set_edge_mode(lower, upper, start);
}


void estimate_work()
{
    if (!priority_scheduler) {
        throw ErrMsg("Scheduler not initialized");
    }

    priority_scheduler->estimateWork();
}

// empty PriorityInfo if no more edges
PriorityInfo get_next_edge()
{
    PriorityInfo priority_info;
    if (!priority_scheduler) {
        throw ErrMsg("Scheduler not initialized");
    }

    if (priority_scheduler->isFinished()) {
        return priority_info;
    }
    boost::tuple<unsigned int, unsigned int, unsigned int> location;
    boost::tuple<Label, Label> body_pair = priority_scheduler->getTopEdge(location);
   
    priority_info.body_pair = make_tuple(boost::get<0>(body_pair), boost::get<1>(body_pair));
    priority_info.location = make_tuple(boost::get<0>(location), boost::get<1>(location), boost::get<2>(location));
    return priority_info;
}

double get_edge_val(PriorityInfo priority_info)
{
    if (!priority_scheduler) {
        throw ErrMsg("Scheduler not initialized");
    }

    Label l1 = extract<Label>(priority_info.body_pair[0]);
    Label l2 = extract<Label>(priority_info.body_pair[1]);

    RagEdge_t* edge = rag->find_rag_edge(l1, l2);

    if (!edge) {
        throw ErrMsg("Edge not found");
    }
    return edge->get_weight();
}



boost::python::list get_qa_violators(unsigned int threshold)
{
    PriorityInfo priority_info;
    if (!priority_scheduler) {
        throw ErrMsg("Scheduler not initialized");
    }

    vector<Label> violators = priority_scheduler->getQAViolators(threshold);

    boost::python::list violators_np;

    for (unsigned int i = 0; i < violators.size(); ++i) {
        violators_np.append(violators[i]);
    }

    return violators_np;
}


// exception throw if edge does not exist or connection probability specified is illegal
void set_edge_result(tuple body_pair, bool remove)
{
    if (!priority_scheduler) {
        throw ErrMsg("Scheduler not initialized");
    }

    Label l1 = extract<Label>(body_pair[0]);
    Label l2 = extract<Label>(body_pair[1]);
    boost::tuple<Label, Label> body_pair_temp(l1, l2);
    priority_scheduler->removeEdge(body_pair_temp, remove);
}

bool undo()
{
    if (!priority_scheduler) {
        throw ErrMsg("Scheduler not initialized");
    }

    return priority_scheduler->undo();
}

BOOST_PYTHON_MODULE(libNeuroProofPriority)
{
    def("initialize_priority_scheduler", initialize_priority_scheduler);
    def("export_priority_scheduler", export_priority_scheduler);
    def("get_next_edge", get_next_edge);
    def("get_qa_violators", get_qa_violators);
    def("set_edge_result", set_edge_result);
    def("undo", undo);
    def("get_percent_prediction_correct", get_percent_prediction_correct);
    def("get_average_prediction_error", get_average_prediction_error);
    def("get_estimated_num_remaining_edges", get_estimated_num_remaining_edges);
    def("set_synapse_mode", set_synapse_mode);
    def("set_body_mode", set_body_mode);
    def("set_orphan_mode", set_orphan_mode);
    def("set_edge_mode", set_edge_mode);
    def("get_edge_val", get_edge_val);
    def("estimate_work", estimate_work);

    class_<PriorityInfo>("PriorityInfo")
        .def_readwrite("body_pair", &PriorityInfo::body_pair)
        .def_readonly("location", &PriorityInfo::location);
}

