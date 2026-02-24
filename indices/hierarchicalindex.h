#ifndef _HIERARCHICALINDEX_H_
#define _HIERARCHICALINDEX_H_

#include "../def_global.h"
#include "../containers/relation.h"



// Framework
class HierarchicalIndex
{
protected:
    size_t numIndexedRecords;
    unsigned int numBits;
    unsigned int maxBits;
    unsigned int height;
    
    // Construction
    virtual inline void updateCounters(const Record &r) {};
    virtual inline void updatePartitions(const Record &r) {};

public:
    // Statistics
    size_t numPartitions;
    size_t numEmptyPartitions;
    float avgPartitionSize;
    size_t numOriginals, numReplicas;
    size_t numOriginalsIn, numOriginalsAft, numReplicasIn, numReplicasAft;


    // Construction
    HierarchicalIndex(const Relation &R, const unsigned int numBits, const unsigned int maxBits);
    virtual void print(const char c) {};
    virtual void getStats() {};
    virtual size_t getSize() { return 0; };
    virtual ~HierarchicalIndex() {};
    

    // &querying
    // HINT
    // Basic predicates of Allen's algebra
    virtual size_t execute_Equals(const RangeQuery &q) {return 0;};
    virtual size_t execute_Starts(const RangeQuery &q) {return 0;};
    virtual size_t execute_Started(const RangeQuery &q) {return 0;};
    virtual size_t execute_Finishes(const RangeQuery &q) {return 0;};
    virtual size_t execute_Finished(const RangeQuery &q) {return 0;};
    virtual size_t execute_Meets(const RangeQuery &q) {return 0;};
    virtual size_t execute_Met(const RangeQuery &q) {return 0;};
    virtual size_t execute_Overlaps(const RangeQuery &q) {return 0;};
    virtual size_t execute_Overlapped(const RangeQuery &q) {return 0;};
    virtual size_t execute_Contains(const RangeQuery &q) {return 0;};
    virtual size_t execute_Contained(const RangeQuery &q) {return 0;};
    virtual size_t execute_Precedes(const RangeQuery &q) {return 0;};
    virtual size_t execute_Preceded(const RangeQuery &q) {return 0;};
    
    // Generalized predicate, ACM SIGMOD'22 gOverlaps
    virtual size_t execute_gOverlaps(const StabbingQuery &q) {return 0;};
    virtual size_t execute_gOverlaps(const RangeQuery &q) {return 0;};

    
    // HINT^m
    // Basic predicates of Allen's algebra
    virtual size_t executeBottomUp_Equals(const RangeQuery &q) {return 0;};
    virtual size_t executeBottomUp_Starts(const RangeQuery &q) {return 0;};
    virtual size_t executeBottomUp_Started(const RangeQuery &q) {return 0;};
    virtual size_t executeBottomUp_Finishes(const RangeQuery &q) {return 0;};
    virtual size_t executeBottomUp_Finished(const RangeQuery &q) {return 0;};
    virtual size_t executeBottomUp_Meets(const RangeQuery &q) {return 0;};
    virtual size_t executeBottomUp_Met(const RangeQuery &q) {return 0;};
    virtual size_t executeBottomUp_Overlaps(const RangeQuery &q) {return 0;};
    virtual size_t executeBottomUp_Overlapped(const RangeQuery &q) {return 0;};
    virtual size_t executeBottomUp_Contains(const RangeQuery &q) {return 0;};
    virtual size_t executeBottomUp_Contained(const RangeQuery &q) {return 0;};
    virtual size_t executeBottomUp_Precedes(const RangeQuery &q) {return 0;};
    virtual size_t executeBottomUp_Preceded(const RangeQuery &q) {return 0;};

    // Generalized predicate, ACM SIGMOD'22 gOverlaps
//    virtual size_t executeTopDown_gOverlaps(const StabbingQuery &q) {return 0;};
//    virtual size_t executeTopDown_gOverlaps(const RangeQuery &q) {return 0;};
    virtual size_t executeBottomUp_Stabbing(const RangeQuery &q) {return 0;};
    virtual size_t executeBottomUp_gOverlaps(const RangeQuery &q) {return 0;};
    
    // Indepdent range sampling
    virtual RecordId get_random_sample(const hint_log &_log) { return 0; };
    
    virtual void executeBottomUp_Stabbing(const RangeQuery &q, const unsigned int sample_size) { };
    virtual void executeBottomUp_gOverlaps(const RangeQuery &q, const unsigned int sample_size) { };
    virtual void executeBottomUp_Stabbing(const RangeQuery &q, vector<hint_log> &candidates) { };
    virtual void executeBottomUp_gOverlaps(const RangeQuery &q, vector<hint_log> &candidates) { };

    // FIRAS sampling support
    virtual size_t executeBottomUp_Stabbing_Count(const RangeQuery &q) { return 0; };
    virtual void executeBottomUp_Stabbing_Collect(const RangeQuery &q, vector<RecordId> &results) { };
    virtual RecordId executeBottomUp_Stabbing_Sample(const RangeQuery &q) { return 0; };
};
#endif // _HIERARCHICALINDEX_H_
