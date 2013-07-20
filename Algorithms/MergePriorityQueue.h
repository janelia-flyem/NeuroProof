#ifndef _Merge_Priority_Queue
#define _Merge_Priority_Queue


#include "assert.h"
#include <cstdlib>
#include <vector>

#include "../Rag/Rag.h"
#include "../Rag/RagEdge.h"

#define MAX_QVAL 2


using namespace NeuroProof;
using namespace std;


template<class K, class V>
class QueueElement{
    K  _key;
    V _val;
    bool validity;	
public:
    QueueElement(): _key(0), validity(true){};
    QueueElement(K pkey, V pval): _key(pkey),	_val(pval), validity(true){};
    K get_key(){return _key;};
    void set_key(K pkey){_key=pkey;};
    V get_val(){return _val;};	
    void invalidate() {validity = false;};
    bool valid(){return validity;};	 		     	
    void reassign_qloc(int ploc, Rag_t* prag){ 
	Node_t node1 = _val.first;
        Node_t node2 = _val.second;
	RagEdge_t* rag_edge = prag->find_rag_edge(node1, node2);
	if(!rag_edge)
	    return;

        rag_edge->set_property("qloc", ploc);
    };
    //QueueElement<K,T>& operator=(const QueueElement<K,T>& another);
};


template<class T>
class MergePriorityQueue{
    size_t _queue_size;

    size_t _storage_size;
    vector<T> *_storage;	

    size_t parent(size_t loc){return (loc/2);};	

    void exchange(size_t a, size_t b);	
    Rag_t* rag;	

public:
    MergePriorityQueue(Rag_t* prag): rag(prag){};  	
    void set_storage(vector<T> *parray);
    T heap_extract_min();
    void heap_insert(T qelem);
    void heap_delete(size_t loc);   			
    bool is_empty(){return ((_queue_size<1)?true:false);};
    size_t get_size(){return _queue_size;};
    void min_heapify(size_t loc);
    void heap_decrease_key(size_t loc, T qelem);		
    void heap_increase_key(size_t loc, T qelem);		
    void invalidate(size_t loc) {_storage->at(loc-1).invalidate();};		
    
};



template <class T>
void MergePriorityQueue<T>::set_storage(vector<T> *parray){
     
    assert(parray->size()>0);	

    _storage = parray;
    _storage_size = _storage->size();
    _queue_size = _storage_size;

    for (int i = _storage_size/2; i>=1; i--)
	min_heapify(i);	 	  	
}

template <class T>
T MergePriorityQueue<T>::heap_extract_min(){

    assert(_queue_size>0);
    T min;

    _storage->at(0).reassign_qloc(-1, rag);	 	
    min = _storage->at(0);
    _storage->at(0) = _storage->at(_queue_size-1); 	
    _storage->at(0).reassign_qloc(0, rag);	
    _queue_size--;
    min_heapify(1);  
    return min;
}

template <class T>
void MergePriorityQueue<T>::heap_delete(size_t loc){ //zero_based

    assert(_queue_size>0);
    
    _storage->at(loc-1).reassign_qloc(-1, rag);	 	
    _storage->at(loc-1) = _storage->at(_queue_size-1); 	
    _storage->at(loc-1).reassign_qloc(loc-1, rag);	
    _queue_size--;
    min_heapify(loc);  
    
}

template <class T> 
void MergePriorityQueue<T>::min_heapify(size_t loc){

    size_t left = 2*loc ; 
    size_t right = 2*loc +1; 

    size_t smallest; 	
    if ( (left<= _queue_size) && (_storage->at(left-1).get_key() < _storage->at(loc-1).get_key() ))	 		
	smallest = left;
    else
	smallest = loc;

    if ((right<= _queue_size) && (_storage->at(right-1).get_key() < _storage->at(smallest-1).get_key() ))	 		
	smallest = right;

    if (smallest != loc){
	exchange(loc, smallest);
	min_heapify(smallest);
    }

}

template <class T>
void MergePriorityQueue<T>::exchange(size_t a, size_t b){
    T tmp;
    tmp = _storage->at(a-1);
    _storage->at(a-1) = _storage->at(b-1);
    _storage->at(b-1) = tmp;

    _storage->at(a-1).reassign_qloc(a-1, rag);		
    _storage->at(b-1).reassign_qloc(b-1, rag);		
 	
}
template <class T>
void MergePriorityQueue<T>::heap_decrease_key(size_t loc, T qelem){

    if (qelem.get_key() >= _storage->at(loc-1).get_key())
	return;    	

    _storage->at(loc-1).set_key(qelem.get_key()); 
    size_t i = loc;
    while (i>1 && (_storage->at(parent(i)-1).get_key() > _storage->at(i-1).get_key())){
	exchange(i, parent(i));
	i = parent(i);
    }	
}

template <class T>
void MergePriorityQueue<T>::heap_increase_key(size_t loc, T qelem){
    _storage->at(loc-1).set_key(qelem.get_key());
    min_heapify(loc);
}


template <class T>
void MergePriorityQueue<T>::heap_insert(T qelem){

    if (_queue_size == _storage_size){
	_storage->push_back(qelem);
	_storage_size = _storage->size();
	_queue_size = _storage_size;
    }
    else if (_queue_size < _storage_size){
	_storage->at(_queue_size) = qelem;
	_queue_size++;
    }	 	
    _storage->at(_queue_size-1).reassign_qloc(_queue_size-1, rag);	
    _storage->at(_queue_size-1).set_key(MAX_QVAL);	
     	
    heap_decrease_key(_queue_size, qelem);	    		
}



typedef QueueElement< double, std::pair<Node_t, Node_t> >  QE;





#endif
