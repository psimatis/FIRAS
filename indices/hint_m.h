#ifndef _HINT_M_H_
#define _HINT_M_H_

#include "../def_global.h"
#include "../containers/relation.h"
#include "../containers/offsets.h"
#include "../indices/hierarchicalindex.h"

// HINT^m with subs+sort, skewness & sparsity optimizations and cash misses activated, from VLDB Journal - simplified version with no vertical links, full size offsets
class HINT_M_SubsSort_SS_CM_NoVLs : public HierarchicalIndex
{
public:
    Relation      *pOrgsInTmp;
//    Relation      *pOrgsInCopyTmp;
    Relation      *pOrgsAftTmp;
    Relation      *pRepsInTmp;
    Relation      *pRepsAftTmp;
    
    RelationId    *pOrgsInIds;
    vector<pair<Timestamp, Timestamp> > *pOrgsInTimestamps;
//    RelationId    *pOrgsInCopyIds;
//    vector<pair<Timestamp, Timestamp> > *pOrgsInCopyTimestamps;
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
//    Offsets_SS_CM_NoVLs *pOrgsInCopy_ioffsets;
    Offsets_SS_CM_NoVLs *pOrgsAft_ioffsets;
    Offsets_SS_CM_NoVLs *pRepsIn_ioffsets;
    Offsets_SS_CM_NoVLs *pRepsAft_ioffsets;
        
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

    inline void scanPartition_CheckBothTimestamps_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, bool (*compare)(const pair<Timestamp, Timestamp>&, const pair<Timestamp, Timestamp>&), Timestamp qstart, Timestamp qend, vector<RecordId> &results);
    inline void scanPartition_CheckEnd_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, Timestamp qt, vector<RecordId> &results);
    inline void scanPartition_CheckEnd_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, bool (*compare)(const pair<Timestamp, Timestamp>&, const pair<Timestamp, Timestamp>&), Timestamp qt, vector<RecordId> &results);
    inline void scanPartition_CheckStart_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, bool (*compare)(const pair<Timestamp, Timestamp>&, const pair<Timestamp, Timestamp>&), Timestamp qt, vector<RecordId> &results);
    inline void scanPartition_NoChecks_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, RelationId *ids, vector<RecordId> &results);
    inline void scanPartitions_NoChecks_gOverlaps(unsigned int level, Timestamp ts, Timestamp te, Offsets_SS_CM_NoVLs *ioffsets, RelationId *ids, vector<RecordId> &results);
    
public:
    // Construction
    HINT_M_SubsSort_SS_CM_NoVLs(const Relation &R, const unsigned int numBits, const unsigned int maxBits);
    void getStats();
    size_t getSize();
    ~HINT_M_SubsSort_SS_CM_NoVLs();
    
    // Generalized predicates, ACM SIGMOD'22 gOverlaps
    size_t executeBottomUp_Stabbing(const RangeQuery &q);
    size_t executeBottomUp_gOverlaps(const RangeQuery &q);
    
    void executeBottomUp_Stabbing(const RangeQuery &q, vector<RecordId> &results);
    void executeBottomUp_gOverlaps(const RangeQuery &q, vector<RecordId> &results);

    void print(const char *heading = "");
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
#endif // _HINT_M_H_
