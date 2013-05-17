#ifndef _NODE_TYPE
#define _NODE_TYPE

#define MEAN_PROB 1
#define COUNT_TYPE 2

#include <vector>
#include <map>
#include <cstdio>

template <typename Region>
class NodeType {

    Region node_type;
    std::multimap< Region, unsigned long long > type_list;
    double sum_mitop;	
    double npixels;	
    int mthd;    
 
public:
    NodeType() : node_type(0), sum_mitop(0), npixels(0), mthd(MEAN_PROB){ type_list.clear();};	

    void update(std::vector<double>& predictions);	
    void set_type();	
    void set_type(Region ptype){node_type=ptype;};	
    Region get_node_type() {return node_type;};
    //void add_mitop(double mp){};	

};


template<typename Region> void NodeType<Region>::update(std::vector<double>& predictions) {

    double mitop = predictions[2];
    double cytop = predictions[1];
    double bdryp = predictions[0];	 	

    if (mthd == MEAN_PROB){
	sum_mitop += mitop; 
	npixels++;
    }
    else if (mthd == COUNT_TYPE){
	Region ntype = 0;
        if ( mitop > bdryp && mitop > 2*cytop) // p(mito) > 2*p(cyto)
	    ntype = 2;
	else //if ( cytop > bdryp)	
	    ntype = 1;

    	typename std::multimap< Region, unsigned long long >::iterator type_itr = type_list.find(ntype);

    //node_itr = type_list.find(ntype);	

    	if(type_itr == type_list.end()){
	    type_list.insert(std::make_pair(ntype,1));
    	} 	
    	else{
	    (type_itr->second) += 1;	
    	}	
    }
}


template<typename Region> void  NodeType<Region>::set_type() {
 
    if (mthd == MEAN_PROB){	
   	double mito_pct = sum_mitop/npixels;	
	if(mito_pct > 1.0){
           printf("somephing wrong with mito_ratio %d\n", node_type);
	    node_type = 0;	
    	}
			
   	if (mito_pct >= 0.35)	
	    node_type = 2;
   	else
	    node_type = 1;	

    }
    else if (mthd == COUNT_TYPE){
    	typename std::multimap< Region, unsigned long long >::iterator type_itr;
    	Region maxidx=0;
	unsigned long long maxt=0;	  	
    	for(type_itr=type_list.begin(); type_itr!=type_list.end(); type_itr++){
	    if (type_itr->second > maxt){	
	    	maxt = type_itr->second;
	    	maxidx = type_itr->first;	
	    }
    	}
    	if (maxidx==0) 	
	    printf("type not found, node %d\n",node_type);
    	node_type = maxidx;
    }
}
 
#endif
