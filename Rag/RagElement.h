/*!
 * Base class for rag node and rag edge.  Contains common interface
 * for communicating with the property manager.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/ 

#ifndef RAGELEMENT_H
#define RAGELEMENT_H

#include "Features/GlbPropertyManager.h"

namespace NeuroProof {

/*!
 * Basic element used in Region Adjacency Grap (RAG).  Contains
 * basic property handling interface for both rag nodes and edges.
*/
class RagElement {
  public:
    /*!
     * Set any data-type to a rag element with a given property name
     * \param key property name to reference given property
     * \param property data to be save at this element with the given property name
    */ 
    template <typename T>
    void set_property(std::string key, T property)
    {
        GlbProperty::get_instance()->set_property((*this), key, property);
    }

    /*!
     * Set property ptr directly at the given property name
     * \param key property name to reference given property
     * \param property ptr to be saved at this element with the given property name
    */
    void set_property_ptr(std::string key, PropertyPtr property)
    {
        GlbProperty::get_instance()->set_property_ptr((*this), std::string key, property);
    }

    /*!
     * Get property of specified data type at the given property name.
     * No property name exception errors will not be caught here.
     * \param key property name to reference a given property
     * \return reference to property data
    */
    template <typename T>
    T& get_property(std::string key)
    {
        return GlbProperty::get_instance()->get_property<T>((*this), key);
    }

    /*!
     * Get property pointer for the give property name
     * \param key property name to reference a given property
     * \return shared pointer to property
    */
    PropertyPtr get_property_ptr(std::string key)
    {
        return GlbProperty::get_instance()->get_property_ptr((*this), key);
    }

    /*!
     * Determine if property with the given property name exists
     * for the rag element
     * \param key property name to reference a given property
     * \return existence of property
    */
    bool has_property(std::string key)
    {
        GlbProperty::get_instance()->has_property((*this), key)
    }
  
    /*!
     * Remove the reference to the property for the given property name
     * \param key property name to reference a given property
    */
    void rm_property(std::string key)
    {
        GlbProperty::get_instance()->rm_property((*this), key)
    }

    /*!
     * Copies all properties from one rag element to another
     * \param element2 rag element destination
    */
    void cp_properties(RagElement* element2)
    {
        GlbProperty::get_instance()->mv_property((*this), element2)
    }
    
    /*!
     * Moves all properties from one rag element to another
     * \param element2 rag element destination
    */
    void mv_properties(RagElement* element2)
    {
        GlbProperty::get_instance()->mv_property((*this), element2)
    }
    
    /*!
     * Removes all properties associated with rag element
    */ 
    void rm_properties()
    {
        GlbProperty::get_instance()->rm_property((*this))
    } 
};

}

#endif
