#include "offsets.h"



OffsetEntry_SS_CM_NoVLs::OffsetEntry_SS_CM_NoVLs()
{
}
    

OffsetEntry_SS_CM_NoVLs::OffsetEntry_SS_CM_NoVLs(Timestamp tstamp, RelationIdIterator iterI, vector<pair<Timestamp, Timestamp> >::iterator iterT, bool isEmpty)
{
    this->tstamp = tstamp;
    this->iterI  = iterI;
    this->iterT  = iterT;
    this->isEmpty = isEmpty;
}


bool OffsetEntry_SS_CM_NoVLs::operator < (const OffsetEntry_SS_CM_NoVLs &rhs) const
{
    return this->tstamp < rhs.tstamp;
}


bool OffsetEntry_SS_CM_NoVLs::operator >= (const OffsetEntry_SS_CM_NoVLs &rhs) const
{
    return this->tstamp >= rhs.tstamp;
}


OffsetEntry_SS_CM_NoVLs::~OffsetEntry_SS_CM_NoVLs()
{
}
