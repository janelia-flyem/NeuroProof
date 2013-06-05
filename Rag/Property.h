/*!
 * Defines basic property interface that will enable flexible storage
 * of properties for each rag element.  A templated derived class is
 * defined that allows users to wrap any data type into a property type
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/
#ifndef PROPERTY_H
#define PROPERTY_H

// all properties will be accessed through smart pointers
#include <boost/shared_ptr.hpp>

namespace NeuroProof {

/*!
 * Defines an abstract class interface that will be used to
 * store property information for each Rag element
*/
class Property {
  public:
    /*!
     * Pure virtual copy: must be defined for all properties
     * \return shared pointer to property copied property type
    */
    virtual boost::shared_ptr<Property> copy() = 0;
    
    /*!
     * Empty virtual destructor for base class to allow
     * derived classes to implement
    */
    virtual ~Property() {}
    
    // TODO: add serialization/deserialization interface
};

typedef boost::shared_ptr<Property> PropertyPtr;

/*!
 * Defines a class of type property that wraps any data
 * type into a property type
*/
template <typename T>
class PropertyTemplate : public Property {
  public:
    /*!
     * Constructor that loads data of any type
     * \param data_ data of type T to be stored in property
    */
    PropertyTemplate(T data_) : Property(), data(data_) {}
    
    /*!
     * Creates a new property template by calling the default copy
     * constructor for this class and the templated type
     * \return property pointer corresponding for new property template
    */
    virtual PropertyPtr copy()
    {
        return PropertyPtr(new PropertyTemplate<T>(*this));
    }
    
    /*!
     * Returns the wrapped data
     * \return templated data stored in property template
    */
    virtual T& get_data()
    {
        return data;
    }
    
    /*!
     * Ovewrites the stored data with the data specified
     * \param data_ property data
    */
    virtual void set_data(T data_)
    {
        data = data_;
    }
    // TODO: add serialization/deserialization interface -- just memory copy

  private:
    //! property data of any type -- implement copy constructor as necessary
    T data;
};

}

#endif
