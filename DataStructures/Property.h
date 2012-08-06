#ifndef PROPERTY_H
#define PROPERTY_H

#include <boost/shared_ptr.hpp>
#include "../Utilities/ErrMsg.h"
#include "AffinityPair.h"
#include "RagEdge.h"
#include "RagNode.h"
#include <tr1/unordered_map>

namespace NeuroProof {

class Property {
  public:
    virtual boost::shared_ptr<Property> copy() = 0;
    // add serialization/deserialization interface
};

template <typename T>
class PropertyTemplate : public Property {
  public:
    PropertyTemplate(T data_) : Property(), data(data_) {}
    virtual boost::shared_ptr<Property> copy()
    {
        return boost::shared_ptr<Property>(new PropertyTemplate<T>(*this));
    }
    virtual T& get_data()
    {
        return data;
    }
    virtual void set_data(T data_)
    {
        data = data_;
    }
    // add serialization/deserialization interface -- just memory copy

  protected:
    T data;
};



}


#endif
