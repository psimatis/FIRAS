#ifndef _OFFSETS_H_
#define _OFFSETS_H_

#include "relation.h"



class OffsetEntry_SS_CM_NoVLs
{
public:
    Timestamp tstamp;
    RelationIdIterator iterI;
    vector<pair<Timestamp, Timestamp> >::iterator iterT;
    bool isEmpty;
    
    OffsetEntry_SS_CM_NoVLs();
    OffsetEntry_SS_CM_NoVLs(Timestamp tstamp, RelationIdIterator iterI, vector<pair<Timestamp, Timestamp> >::iterator iterT, bool isEmpty);
    bool operator < (const OffsetEntry_SS_CM_NoVLs &rhs) const;
    bool operator >= (const OffsetEntry_SS_CM_NoVLs &rhs) const;
    ~OffsetEntry_SS_CM_NoVLs();
};

typedef vector<OffsetEntry_SS_CM_NoVLs> Offsets_SS_CM_NoVLs;
typedef Offsets_SS_CM_NoVLs::const_iterator Offsets_SS_CM_NoVLs_Iterator;
#endif //_OFFSETS_H_
