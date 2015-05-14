#ifndef FEATURECACHE_H
#define FEATURECACHE_H

#include <vector>
#include <string>
#include <cassert>

namespace NeuroProof {

struct FeatureCache {
    virtual unsigned int deserialize(char * bytes) = 0;
    virtual void serialize(std::string& buffer) = 0;
};

struct CountCache : public FeatureCache {
    CountCache() : count(0) {}
    signed long long count; 

    virtual unsigned int deserialize(char * bytes)
    {
        assert(sizeof(signed long long) == 8);
        
        unsigned int bytes_read = 0;
        count = *((signed long long *) bytes);
        
        bytes_read = sizeof(signed long long);
        bytes += sizeof(signed long long);

        return bytes_read;
    }
    virtual void serialize(std::string& buffer)
    {
        assert(sizeof(signed long long) == 8);

        // write 64-bit count
        buffer += std::string((char*)(&count), sizeof(signed long long));
    } 
};


struct MomentCache : public FeatureCache{
    MomentCache(unsigned int num_moments) : count(0), vals(num_moments, 0) {}
    // will overwrite previous cache
    virtual unsigned int deserialize(char * bytes)
    {
        assert(sizeof(unsigned long long) == 8);
        assert(sizeof(unsigned int) == 4);
        assert(sizeof(double) == 8);
        
        unsigned int bytes_read = 0;
        count = *((unsigned long long *) bytes);

        bytes_read = sizeof(unsigned long long);
        bytes += sizeof(unsigned long long);

        // num_moments specified must correspond to what was stored in the buffer
        unsigned int num_moments = *((unsigned int*) bytes);
        assert(num_moments == vals.size());

        bytes_read += sizeof(unsigned int);
        bytes += sizeof(unsigned int);

        for (unsigned int i = 0; i < num_moments; ++i) {
            double val = *((double*) bytes);
            bytes_read += sizeof(double);
            bytes += sizeof(double);
            vals[i] = val;
        }

        return bytes_read;
    }
    virtual void serialize(std::string& buffer)
    {
        assert(sizeof(unsigned long long) == 8);
        assert(sizeof(unsigned int) == 4);
        assert(sizeof(double) == 8);
        
        // write 64-bit count
        buffer += std::string((char*)(&count), sizeof(unsigned long long));

        // write number of moments (64 bit)
        unsigned int num_moments = vals.size();
        buffer += std::string((char*)(&num_moments), sizeof(unsigned int));
        
        // write all vals (double)
        for (unsigned int i = 0; i < num_moments; ++i) {
            buffer += std::string((char*)&vals[i], sizeof(double)); 
        }
    }

    unsigned long long count; 
    std::vector<double> vals;
};

struct HistCache : public FeatureCache {
    HistCache(unsigned num_bins) : count(0), hist(num_bins, 0) {}
    unsigned long long count;
    std::vector<unsigned long long> hist;

    virtual unsigned int deserialize(char * bytes)
    {
        assert(sizeof(unsigned long long) == 8);
        assert(sizeof(unsigned int) == 4);
        
        unsigned int bytes_read = 0;
        count = *((unsigned long long *) bytes);

        bytes_read = sizeof(unsigned long long);
        bytes += sizeof(unsigned long long);

        // num_bins specified must correspond to what was stored in the buffer
        unsigned int num_bins = *((unsigned int*) bytes);
        assert(num_bins == hist.size());

        bytes_read += sizeof(unsigned int);
        bytes += sizeof(unsigned int);

        for (unsigned int i = 0; i < num_bins; ++i) {
            unsigned long long val = *((unsigned long long*) bytes);
            bytes_read += sizeof(unsigned long long);
            bytes += sizeof(unsigned long long);
            hist[i] = val;
        }

        return bytes_read;
    }
    virtual void serialize(std::string& buffer)
    {
        assert(sizeof(unsigned long long) == 8);
        assert(sizeof(unsigned int) == 4);
        
        // write 64-bit count
        buffer += std::string((char*)(&count), sizeof(unsigned long long));

        // write number of bins (64 bit)
        unsigned int num_bins = hist.size();
        buffer += std::string((char*)(&num_bins), sizeof(unsigned int));
        
        // write all hist (unsigned long long)
        for (unsigned int i = 0; i < num_bins; ++i) {
            buffer += std::string((char*)&hist[i], sizeof(unsigned long long)); 
        }
    }

};

}

#endif


