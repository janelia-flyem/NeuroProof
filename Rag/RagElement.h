/*!
 * Base class for rag node and rag edge.  Contains common interface
 * for communicating with the property manager.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/ 

#ifndef RAGELEMENT_H
#define RAGELEMENT_H

#include "Property.h"
#include "../Utilities/ErrMsg.h"
#include <tr1/unordered_map>
#include <string>

namespace NeuroProof {

/*!
 * Basic element used in Region Adjacency Grap (RAG).  Contains
 * basic property handling interface for both rag nodes and edges.
*/
class RagElement {
  public:
    /*!
     * Define empty constructor
    */
    RagElement() {}
    
    /*!
     * Define empty virtual destructor
    */
    virtual ~RagElement() {}

    /*! 
     * Copy constructor, calls copy for all properties
     * \param dup_element rag element to duplicate
    */
    RagElement(const RagElement& dup_element);
    
    /*!
     * Assignment operator overload
     * \param rag that will be assigned
    */
    RagElement& operator=(const RagElement& dup_element);
      
    /*!
     * Set any data-type to a rag element with a given property name
     * \param key property name to reference given property
     * \param property data to be save at this element with the given property name
    */ 
    template <typename T>
    void set_property(std::string key, T val);

    /*!
     * Set property ptr directly at the given property name
     * \param key property name to reference given property
     * \param property ptr to be saved at this element with the given property name
    */
    void set_property_ptr(std::string key, PropertyPtr property);

    /*!
     * Get property of specified data type at the given property name.
     * No property name exception errors will not be caught here.
     * \param key property name to reference a given property
     * \return reference to property data
    */
    template <typename T>
    T& get_property(std::string key);

    /*!
     * Get property pointer for the give property name
     * \param key property name to reference a given property
     * \return shared pointer to property
    */
    PropertyPtr get_property_ptr(std::string key);

    /*!
     * Determine if property with the given property name exists
     * for the rag element
     * \param key property name to reference a given property
     * \return existence of property
    */
    bool has_property(std::string key);
  
    /*!
     * Remove the reference to the property for the given property name
     * \param key property name to reference a given property
    */
    void rm_property(std::string key);

    /*!
     * Copies all properties from one rag element to another
     * \param element2 rag element destination
    */
    void cp_properties(RagElement* element2);
    
    /*!
     * Moves all properties from one rag element to another
     * \param element2 rag element destination
    */
    void mv_properties(RagElement* element2);

    /*!
     * Removes all properties associated with rag element
    */ 
    void rm_properties();
  
  private:
    typedef std::tr1::unordered_map<std::string, PropertyPtr> Properties_t;
    //! Properties stored for rag element (string index)
    Properties_t properties;
};

// inline functions
inline RagElement::RagElement(const RagElement& dup_element)
{
    for (Properties_t::const_iterator iter = dup_element.properties.begin();
            iter != dup_element.properties.end(); ++iter) {
        properties[iter->first] = iter->second->copy();
    }  
}

inline RagElement& RagElement::operator=(const RagElement& dup_element)
{
    if (this != &dup_element) {
        RagElement element_temp(dup_element);
        std::swap(properties, element_temp.properties);
    }

    return *this; 
}

template <typename T> inline void RagElement::set_property(std::string key, T val)
{
    PropertyPtr property = PropertyPtr(new PropertyTemplate<T>(val)); 
    properties[key] = property;
}

inline void RagElement::set_property_ptr(std::string key, PropertyPtr property)
{
    properties[key] = property;
}

template <typename T> inline T& RagElement::get_property(std::string key)
{
    if (properties.find(key) == properties.end()) {
        throw ErrMsg("Property Error: " + key + " not found");
    }
    PropertyPtr property = properties[key];

    boost::shared_ptr<PropertyTemplate<T> > property_tem =
        boost::dynamic_pointer_cast<PropertyTemplate<T> >(property);

    return property_tem->get_data();
}
    
inline PropertyPtr RagElement::get_property_ptr(std::string key)
{
    if (properties.find(key) == properties.end()) {
        throw ErrMsg("Property Error: " + key + " not found");
    }
    return properties[key];
}

inline bool RagElement::has_property(std::string key)
{
    return !(properties.find(key) == properties.end());
}

inline void RagElement::rm_property(std::string key)
{
    properties.erase(key);
}

inline void RagElement::cp_properties(RagElement* element2)
{
    for (Properties_t::iterator iter = properties.begin();
            iter != properties.end(); ++iter) {
        element2->properties[iter->first] = iter->second->copy();
    }
}

inline void RagElement::mv_properties(RagElement* element2)
{
    for (Properties_t::iterator iter = properties.begin();
            iter != properties.end(); ++iter) {
        element2->properties[iter->first] = iter->second;
    }
    properties.clear(); 
}

inline void RagElement::rm_properties()
{
    properties.clear();
} 



}

#endif
