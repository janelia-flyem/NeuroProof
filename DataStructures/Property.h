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

class PropertyCompute : public Property {
  public:
    virtual boost::shared_ptr<Property> copy() = 0;
    virtual double get_data() = 0;
    virtual void add_point(double val, unsigned int x = 0, unsigned int y = 0, unsigned int z = 0) = 0;
    virtual void merge_property(boost::shared_ptr<PropertyCompute> property2) = 0;
};

#define NUM_BINS 101

class PropertyMedian : public PropertyCompute {
  public:
    PropertyMedian() : count(0)
    {
        for (int i = 0; i < NUM_BINS; ++i) {
            hist[i] = 0;
        }
    }
    boost::shared_ptr<Property> copy()
    {
        return boost::shared_ptr<Property>(new PropertyMedian(*this));
    }    
    void add_point(double val, unsigned int x = 0, unsigned int y = 0, unsigned int z = 0)
    {
        hist[int(val * (NUM_BINS - 1) + 0.5)]++;
        ++count;   
    }
    double get_data()
    {
        int mid_point1 = count / 2;
    
        int spot1 = -1;
        int spot2 = -1;
        int curr_count = 0;
        for (int i = 0; i < NUM_BINS; ++i) {
            curr_count += hist[i];
            
            if (curr_count >= mid_point1 && spot1 == -1) {
                spot1 = i;   
            }
            if (curr_count > mid_point1) {
                spot2 = i;
                break; 
            }
        }
        double median_spot = (double(spot1) + spot2) / 2;
        return (median_spot /= (NUM_BINS - 1));
    }

    void merge_property(boost::shared_ptr<PropertyCompute> property2)
    {
        boost::shared_ptr<PropertyMedian> property = boost::shared_polymorphic_downcast<PropertyMedian>(property2);
        count += property->count;
        for (int i = 0; i < NUM_BINS; ++i) {
            hist[i] += property->hist[i];
        }
    }
        
  private:
    int count;
    int hist[NUM_BINS];
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
