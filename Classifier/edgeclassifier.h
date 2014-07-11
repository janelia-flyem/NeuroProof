
#ifndef _edge_classifier
#define _edge_classifier

class EdgeClassifier{


public: 
	virtual void load_classifier(const char*)=0;
	virtual double predict(std::vector<double>&){

  	    //srand ( time(NULL) );
            //double val= rand()*(1.0/ RAND_MAX);
            double val= 0.5;
	    return val;	
	    
	}
	virtual void learn(std::vector< std::vector<double> >& pfeatures, std::vector<int>& plabels)=0;
	virtual void save_classifier(const char* rf_filename)=0;
	virtual bool is_trained()=0;

        virtual ~EdgeClassifier() {}
        
	virtual void set_ignore_featlist(std::vector<unsigned int>& pignore_list)=0;
	virtual void get_ignore_featlist(std::vector<unsigned int>& pignore_list)=0;


     	virtual void set_tree_weights(std::vector<double>& pwts){};	
	virtual void get_tree_responses(std::vector<double>& pfeatures,std::vector<double>& responses){};
	virtual void reduce_trees(){};
};


#endif
