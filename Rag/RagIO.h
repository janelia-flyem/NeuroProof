#ifndef RAGIO_H
#define RAGIO_H

#include <json/value.h>

namespace NeuroProof {

template <typename Region>
class Rag;

typedef unsigned int Label;

Rag<Label>* create_rag_from_jsonfile(const char * file_name);
Rag<Label>* create_rag_from_json(Json::Value& json_reader_vals);
bool create_jsonfile_from_rag(Rag<Label>* rag, const char * file_name);
bool create_json_from_rag(Rag<Label>* rag, Json::Value& json_writer, bool debug_mode=false);

}

#endif
