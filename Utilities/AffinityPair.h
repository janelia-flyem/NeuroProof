#ifndef AFFINITYPAIR
#define AFFINITYPAIR

#include <boost/functional/hash.hpp>
#include <tr1/unordered_set>


namespace NeuroProof {


struct OrderedPair {
        OrderedPair() {}
        OrderedPair(unsigned int region1_, unsigned int region2_)
        {
            if (region1_ < region2_) {
                region1 = region1_;
                region2 = region2_;
            } else {
                region1 = region2_;
                region2 = region1_;
            }
        }

        bool operator<(const OrderedPair& ordered_pair2) const
        {
            return is_less(ordered_pair2);
        }

        bool is_less(const OrderedPair& ordered_pair2) const
        {
            if (region1 < ordered_pair2.region1) {
                return true;
            } else if (region1 == ordered_pair2.region1 && region2 < ordered_pair2.region2) {
                return true;
            }
            return false;
        }

        bool operator==(const OrderedPair& ordered_pair2) const
        {
            return is_equal(ordered_pair2);
        }

        bool is_equal(const OrderedPair& ordered_pair2) const
        {
            return ((region1 == ordered_pair2.region1) && (region2 == ordered_pair2.region2));
        }

        // hash function
        size_t operator()(const OrderedPair& ordered_pair) const
        {
            return get_hash(ordered_pair);
        }
            
        size_t get_hash(const OrderedPair& ordered_pair) const
        {
            size_t seed = 0;
            boost::hash_combine(seed, (ordered_pair.region1));
            boost::hash_combine(seed, (ordered_pair.region2));
            return seed;    
        }
        
        unsigned int region1;
        unsigned int region2;
};


struct AffinityPair : public OrderedPair {
        AffinityPair() : OrderedPair() {}
        AffinityPair(unsigned int region1_, unsigned int region2_) : OrderedPair(region1_, region2_) { }

        bool operator<(const AffinityPair& affinity_pair2) const
        {
            return this->is_less(affinity_pair2);
        }

        bool operator==(const AffinityPair& affinity_pair2) const
        {
            return this->is_equal(affinity_pair2);;
        }

        // hash function
        size_t operator()(const AffinityPair& affinity_pair) const
        {
            return this->get_hash(affinity_pair);
        }
        
        // fundamental datastructure for using AffinityPairs
        typedef std::tr1::unordered_set<AffinityPair, AffinityPair> Hash;

        // probability of connection
        double weight;
        unsigned long long size;
};


struct AffinityPairWeightCmp {
    bool operator()(const AffinityPair& p1, const AffinityPair& p2) const
    {
        return (p1.weight < p2.weight);
    }
};


}

#endif
