/*!
 * Contains singleton class for managing properties.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/

#ifndef GLBPROPERTYMANAGER_H
#define GLBPROPERTYMANAGER_H

#include "Property.h"
#include "../../Utilities/ErrMsg.h"

#include <tr1/unordered_map>
#include <string>

namespace NeuroProof {

// forward declaration of RagElement
class RagElement;

/*!
 * Singleton class that provides storage for all rag nodes and edges.
 * It is possible that implementing the property manager at the scope
 * of the RAG makes more sense.  By implementing a singleton, one can
 * reference the node and edge properties without going through the
 * rag.  This leads to simple interfacing and gives developers the option
 * to use properties with rag elements when no rag is created.
*/
class GlbPropertyManager {
  public:
    /*! 
     * Return the singleton instance (create if it is the first call)
     * \return property manager object
    */ 
    static GlbPropertyManager* get_instance()
    {
        if (manager == 0) {
            manager = new GlbPropertyManager;
        }
        return manager;
    }
    
    /*!
     * Set property for a rag element given a key and value. Value will be
     * stored in a derived templated Property type.
     * \param element rag element
     * \param key property name to reference given property
     * \param val property value
    */
    template <typename T>
    void set_property(RagElement* element, std::string key, T val)
    {
        PropertyPtr property = PropertyPtr(new PropertyTemplate<T>(val)); 
        Properties_t& properties = property_db[element];
        properties[key] = property;
    }
   
    /*!
     * Set property ptr directly for a rag element.  Value must point to a type
     * derived from Property.
     * \param element rag element
     * \param key property name to reference given property
     * \param property pointer to a type derived from Property
    */
    void set_property_ptr(RagElement* element, std::string key, PropertyPtr property)
    {
        Properties_t& properties = property_db[element];
        properties[key] = property;
    }

    /*!
     * Get property associated with rag element and key
     * \param element rag element
     * \param key property name to reference given property
     * \return reference to property data
    */
    template <typename T>
    T& get_property(RagElement* element, std::string key)
    {
        Properties_t& properties = property_db[element];
        if (properties.find(key) == properties.end()) {
            throw ErrMsg("Property Error: " + key + " not found");
        }
        PropertyPtr property = properties[key];
        
        boost::shared_ptr<PropertyTemplate<T> > property_tem =
            boost::shared_polymorphic_downcast<PropertyTemplate<T> >(property);
        
        return property_tem->get_data();
    }

    /*!
     * Get property pointer associated with rag element and key
     * \param element rag element
     * \param key property name to reference given property
     * \return shared pointer to property
    */ 
    PropertyPtr get_property_ptr(RagElement* element, std::string key)
    {
        Properties_t& properties = property_db[element];
        if (properties.find(key) == properties.end()) {
            throw ErrMsg("Property Error: " + key + " not found");
        }
        return properties[key];
    }
   
    /*!
     * Determines whether a property with the given key exists 
     * \param element rag element
     * \param key property name to reference given property
     * \return true if property key exists
    */
    bool has_property(RagElement* element, std::string key)
    {
        Properties_t& properties = property_db[element];
        return !(properties.find(key) == properties.end());
    }
    
    /*!
     * Remove property from a rag element with a given key
     * \param element rag element
     * \param key property name to reference given property
    */
    void rm_property(RagElement* element, std::string key)
    {
        Properties_t& properties = property_db[element];
        properties.erase(key);
    }  

    /*!
     * Copy all properties from one rag element to another
     * \param element1 source rag element
     * \param element2 destination rag element
    */ 
    void cp_properties(RagElement* element1, RagElement* element2)
    {
        Properties_t& properties = property_db[element1];
        Properties_t& properties_dest = property_db[element2];
       
        for (Properties_t::iterator iter = properties.begin();
              iter != properties.end(); ++iter) {
            properties_dest[iter->first] = iter->second->copy();
        }  
    }
    
    /*!
     * Move all properties from one rag element to another
     * \param element1 source rag element
     * \param element2 destination rag element
    */
    void mv_properties(RagElement* element1, RagElement* element2)
    {
        Properties_t& properties = property_db[element1];
        Properties_t& properties_dest = property_db[element2];
        
        for (Properties_t::iterator iter = properties.begin();
                iter != properties.end(); ++iter) {
            properties_dest[iter->first] = iter->second;
        }

        properties.clear(); 
    }

    /*!
     * Remove all properties from a rag element
     * \param element rag element
    */
    void rm_properties(RagElement* element)
    {
        property_db.erase(element);
    }

  private:
    //! Empty private constructor for singleton
    GlbPropertyManager() { }

    //! Destroy singleton
    ~GlbPropertyManager()
    {
        property_db.clear();   
    }

    //! pointer to singleton object
    static GlbPropertyManager* manager;
    
    typedef std::tr1::unordered_map<std::string, PropertyPtr> Properties_t;
    typedef std::tr1::unordered_map<RagElement*, Properties_t> PropertyMap_t;
    
    //! mapping for RagElement to property map
    PropertyMap_t property_db;
};

}

#endif
