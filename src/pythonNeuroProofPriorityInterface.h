#ifndef PYTHONNEUROPROOFPRIORITYINTERFACE_H
#define PYTHONNEUROPROOFPRIORITYINTERFACE_H

#include <boost/python/tuple.hpp>


struct PriorityInfo {
    boost::python::tuple body_pair;
    boost::python::tuple location;
};

// Most errors will results in a function return of false.  Other errors
// that are not easily handled will result in an ErrMsg exception being thrown


// false if file does not exist or json is not properly formatted
// exception thrown if min, max, or start val are illegal
bool initialize_priority_scheduler(const char * json_file, double min_val, double max_val, double start_val);

// false if file cannot be written
bool export_priority_scheduler(const char * json_file);

// empty PriorityInfo if no more edges
PriorityInfo get_next_edge();

// exception throw if edge does not exist or connection probability specified is illegal
void set_edge_result(boost::python::tuple body_pair, bool remove);

// number of edges yet to be processed in the scheduler
unsigned int get_estimated_num_remaining_edges();

bool undo();

// get current threshold and spot?

#endif



