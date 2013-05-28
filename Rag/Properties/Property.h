#ifndef PROPERTY_H
#define PROPERTY_H

#include <boost/shared_ptr.hpp>

namespace NeuroProof {

class Property {
  public:
    virtual boost::shared_ptr<Property> copy() = 0;
    virtual ~Property() {}
    // TODO: add serialization/deserialization interface
};

typedef boost::shared_ptr<Property> PropertyPtr;

template <typename T>
class PropertyTemplate : public Property {
  public:
    PropertyTemplate(T data_) : Property(), data(data_) {}
    virtual PropertyPtr copy()
    {
        return PropertyPtr(new PropertyTemplate<T>(*this));
    }
    virtual T& get_data()
    {
        return data;
    }
    virtual void set_data(T data_)
    {
        data = data_;
    }
    // TODO: add serialization/deserialization interface -- just memory copy

  protected:
    T data;
};



}

#endif
