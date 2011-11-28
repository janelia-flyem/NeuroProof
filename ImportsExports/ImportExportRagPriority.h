#ifndef IMPORTEXPORTRAGPRIORITY_H
#define IMPORTEXPORTRAGPRIORITY_H

namespace NeuroProof {

template <typename Region>
class Rag;

typedef unsigned int Label;

Rag<Label>* create_rag_from_jsonfile(const char * file_name);
bool create_jsonfile_from_rag(Rag<Label>* rag, const char * file_name);

}

#endif
