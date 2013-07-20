/*!
 * \file
 * Interface for importing and exporting a Rag of type
 * Index_t (unsigned int) to the JSON format
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
*/ 

#ifndef RAGIO_H
#define RAGIO_H

#include <json/value.h>
#include "../Utilities/Glb.h"

namespace NeuroProof {

// forward declare rag 
template <typename Region>
class Rag;

/*!
 * Generates a rag from a json file that lists all the edges
 * \param file_name json file name
 * \return heap created rag
*/
Rag<Index_t>* create_rag_from_jsonfile(const char * file_name);


/*!
 * Generates a rag from json data that contains all of the edges
 * \param json_reader_vals json value
 * \return heap created rag
*/
Rag<Index_t>* create_rag_from_json(Json::Value& json_reader_vals);

/*!
 * Generates a json file that lists all the edges in the provided rag
 * \param rag rag to be exported
 * \param file_name name of json file to be written
 * \return true if successful, false otherwise
*/
bool create_jsonfile_from_rag(Rag<Index_t>* rag, const char * file_name);

/*!
 * Generates json lists all the edges in the provided rag.
 * \param rag to be exported to json
 * \param json_writer json data that will be written
 * \param debug_mode determines whether certain edges are listed or not
 * \return true if successful, false otherwise
*/
bool create_json_from_rag(Rag<Index_t>* rag, Json::Value& json_writer);

}

#endif
