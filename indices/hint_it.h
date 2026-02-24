 #ifndef _HINT_IT_H_
#define _HINT_IT_H_

#include "../def_global.h"
#include "../containers/relation.h"
#include "../containers/offsets.h"
#include "../indices/hierarchicalindex.h"
#include "../indices/intervaltree.h"


struct hint_it_log
{
    unsigned int level;
    unsigned int type; // LOG_ORIGINALS_IN, LOG_ORIGINALS_IN_COPY, LOG_ORIGINALS_AFT, LOG_REPLICAS_IN, LOG_REPLICAS_AFT, LOG_ORIGINALS_IN_IT
    int left_idx;
    int right_idx;
    IntervalTreeNode *node; // For LOG_ORIGINALS_IN_IT: the node of the IT
    int it_type;  // For LOG_ORIGINALS_IN_IT: 0=recordsByStart, 1=recordsByEnd
    
    hint_it_log() {
        left_idx  = -1;
        right_idx = -1;
        it_type = -1;
    };
};


class HINT_IT_SubsSort_SS_CM_NoVLs : public HierarchicalIndex
{
private:
    Relation      *pOrgsInTmp;
    Relation      *pOrgsInCopyTmp;
    Relation      *pOrgsAftTmp;
    Relation      *pRepsInTmp;
    Relation      *pRepsAftTmp;
    
    RelationId    *pOrgsInIds;
    vector<pair<Timestamp, Timestamp> > *pOrgsInTimestamps;
    RelationId    *pOrgsInCopyIds;
    vector<pair<Timestamp, Timestamp> > *pOrgsInCopyTimestamps;
    RelationId    *pOrgsAftIds;
    vector<pair<Timestamp, Timestamp> > *pOrgsAftTimestamps;
    RelationId    *pRepsInIds;
    vector<pair<Timestamp, Timestamp> > *pRepsInTimestamps;
    RelationId    *pRepsAftIds;
    vector<pair<Timestamp, Timestamp> > *pRepsAftTimestamps;
    
    RecordId      **pOrgsIn_sizes, **pOrgsAft_sizes;
    size_t        **pRepsIn_sizes, **pRepsAft_sizes;
    RecordId      **pOrgsIn_offsets, **pOrgsAft_offsets;
    size_t        **pRepsIn_offsets, **pRepsAft_offsets;
    Offsets_SS_CM_NoVLs *pOrgsIn_ioffsets;
    Offsets_SS_CM_NoVLs *pOrgsInCopy_ioffsets;
    Offsets_SS_CM_NoVLs *pOrgsAft_ioffsets;
    Offsets_SS_CM_NoVLs *pRepsIn_ioffsets;
    Offsets_SS_CM_NoVLs *pRepsAft_ioffsets;
    
    // Vector of interval trees for bottom level pOrgsIn partitions
    vector<IntervalTree*> pOrgsInTrees;
        
    // Construction
    inline void updateCounters(const Record &r);
    inline void updatePartitions(const Record &r);
    
    // &querying
    // Auxiliary functions to determine exactly how to scan a partition.
    inline bool getBounds(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, vector<pair<Timestamp, Timestamp> >::iterator &iterStart, vector<pair<Timestamp, Timestamp> >::iterator &iterEnd, RelationIdIterator &iterI);
    inline bool getBounds(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, RelationId *ids, RelationIdIterator &iterIStart, RelationIdIterator &iterIEnd);
    inline bool getBounds(unsigned int level, Timestamp ts, Timestamp te, Offsets_SS_CM_NoVLs *ioffsets, RelationId *ids, RelationIdIterator &iterIStart, RelationIdIterator &iterIEnd);
    
    // Auxiliary functions to scan a partition.
    inline void scanPartition_CheckBothTimestamps_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, bool (*compare)(const pair<Timestamp, Timestamp>&, const pair<Timestamp, Timestamp>&), Timestamp qstart, Timestamp qend, size_t &result);
    inline void scanPartition_CheckEnd_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, Timestamp qt, size_t &result);
    inline void scanPartition_CheckEnd_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, bool (*compare)(const pair<Timestamp, Timestamp>&, const pair<Timestamp, Timestamp>&), Timestamp qt, size_t &result);
    inline void scanPartition_CheckStart_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, bool (*compare)(const pair<Timestamp, Timestamp>&, const pair<Timestamp, Timestamp>&), Timestamp qt, size_t &result);
    inline void scanPartition_NoChecks_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, RelationId *ids, size_t &result);
    inline void scanPartitions_NoChecks_gOverlaps(unsigned int level, Timestamp ts, Timestamp te, Offsets_SS_CM_NoVLs *ioffsets, RelationId *ids, size_t &result);

    inline void scanPartitionNoChecks_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, RelationId *ids, hint_it_log &_log);
    inline void scanPartitionsNoChecks_gOverlaps(unsigned int level, Timestamp ts, Timestamp te, Offsets_SS_CM_NoVLs *ioffsets, RelationId *ids, hint_it_log &_log);
    inline void scanPartitionCheckEnd_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, bool (*compare)(const pair<Timestamp, Timestamp>&, const pair<Timestamp, Timestamp>&), Timestamp qt, hint_it_log &log);
    inline void scanPartitionCheckStart_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, bool (*compare)(const pair<Timestamp, Timestamp>&, const pair<Timestamp, Timestamp>&), Timestamp qt, hint_it_log &_log);
    inline vector<hint_it_log> scanPartitionCheckBothTimestamps_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, bool (*compare)(const pair<Timestamp, Timestamp>&, const pair<Timestamp, Timestamp>&), Timestamp qstart, Timestamp qend);
    

public:
    // Construction
    HINT_IT_SubsSort_SS_CM_NoVLs(const Relation &R, const unsigned int numBits, const unsigned int maxBits);
    void getStats();
    size_t getSize();
    ~HINT_IT_SubsSort_SS_CM_NoVLs();
    
    // Generalized predicates, ACM SIGMOD'22 gOverlaps
    size_t executeBottomUp_Stabbing(const RangeQuery &q);
    // size_t executeBottomUp_gOverlaps(const RangeQuery &q);
    
    // Indepedent range sampling
    void executeBottomUp_gOverlaps(const RangeQuery &q, const unsigned int sample_size);
    
    // Adjusted from Amagata @ IEEE ICDE'24
    RecordId get_random_sample(const hint_it_log &_log);
    size_t executeBottomUp_Stabbing(const RangeQuery& q, vector<hint_it_log>& candidate);
};


// Comparators
inline bool CompareTimestampPairsByStart(const pair<Timestamp, Timestamp> &lhs, const pair<Timestamp, Timestamp> &rhs)
{
    return (lhs.first < rhs.first);
}

inline bool CompareTimestampPairsByEnd(const pair<Timestamp, Timestamp> &lhs, const pair<Timestamp, Timestamp> &rhs)
{
    return (lhs.second < rhs.second);
}
#endif // _HINT_IT_H_