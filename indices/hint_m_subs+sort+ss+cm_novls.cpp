#include "hint_m.h"
#include "../utils/weighted_sampling.hpp"



inline void HINT_M_SubsSort_SS_CM_NoVLs::updateCounters(const Record &r)
{
    int level = 0;
    Timestamp a = r.start >> (this->maxBits-this->numBits);
    Timestamp b = r.end   >> (this->maxBits-this->numBits);
    Timestamp prevb;
    int firstfound = 0, lastfound = 0;
    
    
    while (level < this->height && a <= b)
    {
        if (a%2)
        { //last bit of a is 1
            if (firstfound)
            {
                if ((a == b) && (!lastfound))
                {
                    this->pRepsIn_sizes[level][a]++;
                    lastfound = 1;
                }
                else
                    this->pRepsAft_sizes[level][a]++;
            }
            else
            {
                if ((a == b) && (!lastfound))
                    this->pOrgsIn_sizes[level][a]++;
                else
                    this->pOrgsAft_sizes[level][a]++;
                firstfound = 1;
            }
            a++;
        }
        if (!(b%2))
        { //last bit of b is 0
            prevb = b;
            b--;
            if ((!firstfound) && b < a)
            {
                if (!lastfound)
                    this->pOrgsIn_sizes[level][prevb]++;
                else
                    this->pOrgsAft_sizes[level][prevb]++;
            }
            else
            {
                if (!lastfound)
                {
                    this->pRepsIn_sizes[level][prevb]++;
                    lastfound = 1;
                }
                else
                {
                    this->pRepsAft_sizes[level][prevb]++;
                }
            }
        }
        a >>= 1; // a = a div 2
        b >>= 1; // b = b div 2
        level++;
    }
}


inline void HINT_M_SubsSort_SS_CM_NoVLs::updatePartitions(const Record &r)
{
    int level = 0;
    Timestamp a = r.start >> (this->maxBits-this->numBits);
    Timestamp b = r.end   >> (this->maxBits-this->numBits);
    Timestamp prevb;
    int firstfound = 0, lastfound = 0;
    

    while (level < this->height && a <= b)
    {
        if (a%2)
        { //last bit of a is 1
            if (firstfound)
            {
                if ((a == b) && (!lastfound))
                {
                    this->pRepsInTmp[level][this->pRepsIn_offsets[level][a]++] = Record(r.id, r.start, r.end);
                    lastfound = 1;
                }
                else
                {
                    this->pRepsAftTmp[level][this->pRepsAft_offsets[level][a]++] = Record(r.id, r.start, r.end);
                }
            }
            else
            {
                if ((a == b) && (!lastfound))
                {
                    this->pOrgsInTmp[level][this->pOrgsIn_offsets[level][a]++] = Record(r.id, r.start, r.end);
                    // When uncomment the next, remove ++ from [a]++.
//                    this->pOrgsInCopyTmp[level][this->pOrgsIn_offsets[level][a]++] = Record(r.id, r.start, r.end);
                }
                else
                {
                    this->pOrgsAftTmp[level][this->pOrgsAft_offsets[level][a]++] = Record(r.id, r.start, r.end);
                }
                firstfound = 1;
            }
            a++;
        }
        if (!(b%2))
        { //last bit of b is 0
            prevb = b;
            b--;
            if ((!firstfound) && b < a)
            {
                if (!lastfound)
                {
                    this->pOrgsInTmp[level][this->pOrgsIn_offsets[level][prevb]++] = Record(r.id, r.start, r.end);
                    // When uncomment the next, remove ++ from [prevb]++.
//                    this->pOrgsInCopyTmp[level][this->pOrgsIn_offsets[level][prevb]++] = Record(r.id, r.start, r.end);
                }
                else
                {
                    this->pOrgsAftTmp[level][this->pOrgsAft_offsets[level][prevb]++] = Record(r.id, r.start, r.end);
                }
            }
            else
            {
                if (!lastfound)
                {
                    this->pRepsInTmp[level][this->pRepsIn_offsets[level][prevb]++] = Record(r.id, r.start, r.end);
                    lastfound = 1;
                }
                else
                {
                    this->pRepsAftTmp[level][this->pRepsAft_offsets[level][prevb]++] = Record(r.id, r.start, r.end);
                }
            }
        }
        a >>= 1; // a = a div 2
        b >>= 1; // b = b div 2
        level++;
    }
}


HINT_M_SubsSort_SS_CM_NoVLs::HINT_M_SubsSort_SS_CM_NoVLs(const Relation &R, const unsigned int numBits, const unsigned int maxBits)  : HierarchicalIndex(R, numBits, maxBits)
{
//    Offsets_SS_CM_NoVLs dummySE;
    PartitionId tmp = -1;
    
    
    // Step 1: one pass to count the contents inside each partition.
    this->pOrgsIn_sizes  = (RecordId **)malloc(this->height*sizeof(RecordId *));
    this->pOrgsAft_sizes = (RecordId **)malloc(this->height*sizeof(RecordId *));
    this->pRepsIn_sizes  = (size_t **)malloc(this->height*sizeof(size_t *));
    this->pRepsAft_sizes = (size_t **)malloc(this->height*sizeof(size_t *));
    this->pOrgsIn_offsets  = (RecordId **)malloc(this->height*sizeof(RecordId *));
    this->pOrgsAft_offsets = (RecordId **)malloc(this->height*sizeof(RecordId *));
    this->pRepsIn_offsets  = (size_t **)malloc(this->height*sizeof(size_t *));
    this->pRepsAft_offsets = (size_t **)malloc(this->height*sizeof(size_t *));
    
    for (auto l = 0; l < this->height; l++)
    {
        auto cnt = (int)(pow(2, this->numBits-l));
        
        //calloc allocates memory and sets each counter to 0
        this->pOrgsIn_sizes[l]  = (RecordId *)calloc(cnt, sizeof(RecordId));
        this->pOrgsAft_sizes[l] = (RecordId *)calloc(cnt, sizeof(RecordId));
        this->pRepsIn_sizes[l]  = (size_t *)calloc(cnt, sizeof(size_t));
        this->pRepsAft_sizes[l] = (size_t *)calloc(cnt, sizeof(size_t));
        this->pOrgsIn_offsets[l]  = (RecordId *)calloc(cnt+1, sizeof(RecordId));
        this->pOrgsAft_offsets[l] = (RecordId *)calloc(cnt+1, sizeof(RecordId));
        this->pRepsIn_offsets[l]  = (size_t *)calloc(cnt+1, sizeof(size_t));
        this->pRepsAft_offsets[l] = (size_t *)calloc(cnt+1, sizeof(size_t));
    }
    
    for (const Record &r : R)
        this->updateCounters(r);
    
    
    // Step 2: allocate necessary memory.
    this->pOrgsInTmp  = new Relation[this->height];
//    this->pOrgsInCopyTmp  = new Relation[this->height];
    this->pOrgsAftTmp = new Relation[this->height];
    this->pRepsInTmp  = new Relation[this->height];
    this->pRepsAftTmp = new Relation[this->height];
    
    for (auto l = 0; l < this->height; l++)
    {
        auto cnt = (int)(pow(2, this->numBits-l));
        size_t sumOin = 0, sumOaft = 0, sumRin = 0, sumRaft = 0;
        
        for (auto pId = 0; pId < cnt; pId++)
        {
            this->pOrgsIn_offsets[l][pId]  = sumOin;
// //            this->pOrgsInCopy_offsets[l][pId]  = sumOin;
            this->pOrgsAft_offsets[l][pId] = sumOaft;
            this->pRepsIn_offsets[l][pId]  = sumRin;
            this->pRepsAft_offsets[l][pId] = sumRaft;
            sumOin  += this->pOrgsIn_sizes[l][pId];
            sumOaft += this->pOrgsAft_sizes[l][pId];
            sumRin  += this->pRepsIn_sizes[l][pId];
            sumRaft += this->pRepsAft_sizes[l][pId];
        }
        this->pOrgsIn_offsets[l][cnt]  = sumOin;
// //        this->pOrgsInCopy_offsets[l][cnt]  = sumOin;
        this->pOrgsAft_offsets[l][cnt] = sumOaft;
        this->pRepsIn_offsets[l][cnt]  = sumRin;
        this->pRepsAft_offsets[l][cnt] = sumRaft;
        
        this->pOrgsInTmp[l].resize(sumOin);
//        this->pOrgsInCopyTmp[l].resize(sumOin);
        this->pOrgsAftTmp[l].resize(sumOaft);
        this->pRepsInTmp[l].resize(sumRin);
        this->pRepsAftTmp[l].resize(sumRaft);
    }
    
    
    // Step 3: fill partitions.
    for (const Record &r : R)
        this->updatePartitions(r);

    
    // Step 4: sort partition contents; first need to reset the offsets
    for (auto l = 0; l < this->height; l++)
    {
        auto cnt = (int)(pow(2, this->numBits-l));
        size_t sumOin = 0, sumOaft = 0, sumRin = 0, sumRaft = 0;
        
        for (auto pId = 0; pId < cnt; pId++)
        {
            this->pOrgsIn_offsets[l][pId]  = sumOin;
// //            this->pOrgsInCopy_offsets[l][pId]  = sumOin;
            this->pOrgsAft_offsets[l][pId] = sumOaft;
            this->pRepsIn_offsets[l][pId]  = sumRin;
            this->pRepsAft_offsets[l][pId] = sumRaft;
            sumOin  += this->pOrgsIn_sizes[l][pId];
            sumOaft += this->pOrgsAft_sizes[l][pId];
            sumRin  += this->pRepsIn_sizes[l][pId];
            sumRaft += this->pRepsAft_sizes[l][pId];
        }
        this->pOrgsIn_offsets[l][cnt]  = sumOin;
// //        this->pOrgsInCopy_offsets[l][cnt]  = sumOin;
        this->pOrgsAft_offsets[l][cnt] = sumOaft;
        this->pRepsIn_offsets[l][cnt]  = sumRin;
        this->pRepsAft_offsets[l][cnt] = sumRaft;
    }
    
    for (auto l = 0; l < this->height; l++)
    {
        auto cnt = (int)(pow(2, this->numBits-l));
        for (auto pId = 0; pId < cnt; pId++)
        {
            sort(this->pOrgsInTmp[l].begin()+this->pOrgsIn_offsets[l][pId], this->pOrgsInTmp[l].begin()+this->pOrgsIn_offsets[l][pId+1]);
// //            sort(this->pOrgsInCopyTmp[l].begin()+this->pOrgsInCopy_offsets[l][pId], this->pOrgsInCopyTmp[l].begin()+this->pOrgsInCopy_offsets[l][pId+1], CompareByEnd);
//            sort(this->pOrgsInCopyTmp[l].begin()+this->pOrgsIn_offsets[l][pId], this->pOrgsInCopyTmp[l].begin()+this->pOrgsIn_offsets[l][pId+1], CompareRecordsByEnd);
            sort(this->pOrgsAftTmp[l].begin()+this->pOrgsAft_offsets[l][pId], this->pOrgsAftTmp[l].begin()+this->pOrgsAft_offsets[l][pId+1]);
            sort(this->pRepsInTmp[l].begin()+this->pRepsIn_offsets[l][pId], this->pRepsInTmp[l].begin()+this->pRepsIn_offsets[l][pId+1], CompareRecordsByEnd);
            sort(this->pRepsAftTmp[l].begin()+this->pRepsAft_offsets[l][pId], this->pRepsAftTmp[l].begin()+this->pRepsAft_offsets[l][pId+1], CompareRecordsByEnd);
        }
    }

    
    // Step 5: break-down data to create id- and timestamp-dedicated arrays; free auxiliary memory.
    this->pOrgsInIds  = new RelationId[this->height];
//    this->pOrgsInCopyIds  = new RelationId[this->height];
    this->pOrgsAftIds = new RelationId[this->height];
    this->pRepsInIds  = new RelationId[this->height];
    this->pRepsAftIds = new RelationId[this->height];
    this->pOrgsInTimestamps  = new vector<pair<Timestamp, Timestamp> >[this->height];
//    this->pOrgsInCopyTimestamps  = new vector<pair<Timestamp, Timestamp> >[this->height];
    this->pOrgsAftTimestamps = new vector<pair<Timestamp, Timestamp> >[this->height];
    this->pRepsInTimestamps  = new vector<pair<Timestamp, Timestamp> >[this->height];
    this->pRepsAftTimestamps = new vector<pair<Timestamp, Timestamp> >[this->height];
    for (auto l = 0; l < this->height; l++)
    {
        auto cnt = pOrgsInTmp[l].size();
        
        this->pOrgsInIds[l].resize(cnt);
//        this->pOrgsInCopyIds[l].resize(cnt);
        this->pOrgsInTimestamps[l].resize(cnt);
//        this->pOrgsInCopyTimestamps[l].resize(cnt);
        for (auto j = 0; j < cnt; j++)
        {
            this->pOrgsInIds[l][j] = this->pOrgsInTmp[l][j].id;
            this->pOrgsInTimestamps[l][j].first = this->pOrgsInTmp[l][j].start;
            this->pOrgsInTimestamps[l][j].second = this->pOrgsInTmp[l][j].end;
//            this->pOrgsInCopyIds[l][j] = this->pOrgsInCopyTmp[l][j].id;
//            this->pOrgsInCopyTimestamps[l][j].first = this->pOrgsInCopyTmp[l][j].start;
//            this->pOrgsInCopyTimestamps[l][j].second = this->pOrgsInCopyTmp[l][j].end;
        }
        
        cnt = pOrgsAftTmp[l].size();
        this->pOrgsAftIds[l].resize(cnt);
        this->pOrgsAftTimestamps[l].resize(cnt);
        for (auto j = 0; j < cnt; j++)
        {
            this->pOrgsAftIds[l][j] = this->pOrgsAftTmp[l][j].id;
            this->pOrgsAftTimestamps[l][j].first = this->pOrgsAftTmp[l][j].start;
            this->pOrgsAftTimestamps[l][j].second = this->pOrgsAftTmp[l][j].end;
        }
        
        cnt = pRepsInTmp[l].size();
        this->pRepsInIds[l].resize(cnt);
        this->pRepsInTimestamps[l].resize(cnt);
        for (auto j = 0; j < cnt; j++)
        {
            this->pRepsInIds[l][j] = this->pRepsInTmp[l][j].id;
            this->pRepsInTimestamps[l][j].first = this->pRepsInTmp[l][j].start;
            this->pRepsInTimestamps[l][j].second = this->pRepsInTmp[l][j].end;
        }

        cnt = pRepsAftTmp[l].size();
        this->pRepsAftIds[l].resize(cnt);
        this->pRepsAftTimestamps[l].resize(cnt);
        for (auto j = 0; j < cnt; j++)
        {
            this->pRepsAftIds[l][j] = this->pRepsAftTmp[l][j].id;
            this->pRepsAftTimestamps[l][j].first = this->pRepsAftTmp[l][j].start;
            this->pRepsAftTimestamps[l][j].second = this->pRepsAftTmp[l][j].end;
        }
    }
    
    
    // Free auxiliary memory
    for (auto l = 0; l < this->height; l++)
    {
        free(this->pOrgsIn_offsets[l]);
// //        free(this->pOrgsInCopy_offsets[l]);
        free(this->pOrgsAft_offsets[l]);
        free(this->pRepsIn_offsets[l]);
        free(this->pRepsAft_offsets[l]);
    }
    free(this->pOrgsIn_offsets);
// //    free(this->pOrgsInCopy_offsets);
    free(this->pOrgsAft_offsets);
    free(this->pRepsIn_offsets);
    free(this->pRepsAft_offsets);
    
    delete[] this->pOrgsInTmp;
//    delete[] this->pOrgsInCopyTmp;
    delete[] this->pOrgsAftTmp;
    delete[] this->pRepsInTmp;
    delete[] this->pRepsAftTmp;
    

    // Step 4: create offset pointers
    this->pOrgsIn_ioffsets  = new Offsets_SS_CM_NoVLs[this->height];
//    this->pOrgsInCopy_ioffsets  = new Offsets_SS_CM_NoVLs[this->height];
    this->pOrgsAft_ioffsets = new Offsets_SS_CM_NoVLs[this->height];
    this->pRepsIn_ioffsets  = new Offsets_SS_CM_NoVLs[this->height];
    this->pRepsAft_ioffsets = new Offsets_SS_CM_NoVLs[this->height];
    for (int l = this->height-1; l > -1; l--)
    {
        auto cnt = (int)(pow(2, this->numBits-l));
        size_t sumOin = 0, sumOaft = 0, sumRin = 0, sumRaft = 0;
        
        for (auto pId = 0; pId < cnt; pId++)
        {
            bool isEmpty = true;
            
//            dummySE.tstamp = pId >> 1;//((pId >> (this->maxBits-this->numBits)) >> 1);
            isEmpty = (this->pOrgsIn_sizes[l][pId] == 0);
            this->pOrgsIn_ioffsets[l].push_back(OffsetEntry_SS_CM_NoVLs(pId, this->pOrgsInIds[l].begin()+sumOin, this->pOrgsInTimestamps[l].begin()+sumOin, isEmpty));
            
//            dummySE.tstamp = pId >> 1;//((pId >> (this->maxBits-this->numBits)) >> 1);
//            isEmpty = (this->pOrgsIn_sizes[l][pId] == 0);
//            this->pOrgsInCopy_ioffsets[l].push_back(OffsetEntry_SS_CM_NoVLs(pId, this->pOrgsInCopyIds[l].begin()+sumOin, this->pOrgsInCopyTimestamps[l].begin()+sumOin, isEmpty));

//            dummySE.tstamp = pId >> 1;//((pId >> (this->maxBits-this->numBits)) >> 1);
            isEmpty = (this->pOrgsAft_sizes[l][pId] == 0);
            this->pOrgsAft_ioffsets[l].push_back(OffsetEntry_SS_CM_NoVLs(pId, this->pOrgsAftIds[l].begin()+sumOaft, this->pOrgsAftTimestamps[l].begin()+sumOaft, isEmpty));
            
//            dummySE.tstamp = pId >> 1;//((pId >> (this->maxBits-this->numBits)) >> 1);
            isEmpty = (this->pRepsIn_sizes[l][pId] == 0);
            this->pRepsIn_ioffsets[l].push_back(OffsetEntry_SS_CM_NoVLs(pId, this->pRepsInIds[l].begin()+sumRin, this->pRepsInTimestamps[l].begin()+sumRin, isEmpty));
            
//            dummySE.tstamp = pId >> 1;//((pId >> (this->maxBits-this->numBits)) >> 1);
            isEmpty = (this->pRepsAft_sizes[l][pId] == 0);
            this->pRepsAft_ioffsets[l].push_back(OffsetEntry_SS_CM_NoVLs(pId, this->pRepsAftIds[l].begin()+sumRaft, this->pRepsAftTimestamps[l].begin()+sumRaft, isEmpty));
            
            sumOin += this->pOrgsIn_sizes[l][pId];
            sumOaft += this->pOrgsAft_sizes[l][pId];
            sumRin += this->pRepsIn_sizes[l][pId];
            sumRaft += this->pRepsAft_sizes[l][pId];
            
            if (isEmpty)
                this->numEmptyPartitions++;
        }
    }
    
    
    // Free auxliary memory
    for (auto l = 0; l < this->height; l++)
    {
        free(this->pOrgsIn_sizes[l]);
        free(this->pOrgsAft_sizes[l]);
        free(this->pRepsIn_sizes[l]);
        free(this->pRepsAft_sizes[l]);
    }
    free(this->pOrgsIn_sizes);
    free(this->pOrgsAft_sizes);
    free(this->pRepsIn_sizes);
    free(this->pRepsAft_sizes);
}


void HINT_M_SubsSort_SS_CM_NoVLs::getStats()
{
    size_t sum = 0;
    for (auto l = 0; l < this->height; l++)
    {
        auto cnt = pow(2, this->numBits-l);
        
        this->numPartitions += cnt;

        this->numOriginalsIn  += this->pOrgsInIds[l].size();
        this->numOriginalsAft += this->pOrgsAftIds[l].size();
        this->numReplicasIn   += this->pRepsInIds[l].size();
        this->numReplicasAft  += this->pRepsAftIds[l].size();
    }
    
    this->avgPartitionSize = (float)(this->numIndexedRecords+this->numReplicasIn+this->numReplicasAft)/(this->numPartitions-numEmptyPartitions);
}


size_t HINT_M_SubsSort_SS_CM_NoVLs::getSize()
{
    size_t size = 0;

    size += 4*sizeof(pOrgsInIds);
    size += 4*sizeof(pOrgsInTimestamps);
    size += 4*sizeof(pOrgsIn_ioffsets);

    for (auto l = 0; l < this->height; l++)
    {
        size += 4*sizeof(RelationId);
        size += 4*sizeof(vector<pair<Timestamp, Timestamp>>);
        size += 4*sizeof(Offsets_SS_CM_NoVLs);

        size += this->pOrgsInIds[l].size() * sizeof(RecordId);
//        size += this->pOrgsInCopyIds[l].size() * sizeof(RecordId);
        size += this->pOrgsAftIds[l].size() * sizeof(RecordId);
        size += this->pRepsInIds[l].size() * sizeof(RecordId);
        size += this->pRepsAftIds[l].size() * sizeof(RecordId);
        
        size += this->pOrgsInTimestamps[l].size() * 2 * sizeof(Timestamp);
//        size += this->pOrgsInCopyTimestamps[l].size() * 2 * sizeof(Timestamp);
        size += this->pOrgsAftTimestamps[l].size() * 2 * sizeof(Timestamp);
        size += this->pRepsInTimestamps[l].size() * 2 * sizeof(Timestamp);
        size += this->pRepsAftTimestamps[l].size() * 2 * sizeof(Timestamp);

        size += this->pOrgsIn_ioffsets[l].size() * sizeof(OffsetEntry_SS_CM_NoVLs);
        size += this->pOrgsAft_ioffsets[l].size() * sizeof(OffsetEntry_SS_CM_NoVLs);
        size += this->pRepsIn_ioffsets[l].size() * sizeof(OffsetEntry_SS_CM_NoVLs);
        size += this->pRepsAft_ioffsets[l].size() * sizeof(OffsetEntry_SS_CM_NoVLs);
    }

    return size;
}


HINT_M_SubsSort_SS_CM_NoVLs::~HINT_M_SubsSort_SS_CM_NoVLs()
{
    delete[] this->pOrgsIn_ioffsets;
    delete[] this->pOrgsAft_ioffsets;
    delete[] this->pRepsIn_ioffsets;
    delete[] this->pRepsAft_ioffsets;
    
    delete[] this->pOrgsInIds;
    delete[] this->pOrgsInTimestamps;
    delete[] this->pOrgsAftIds;
    delete[] this->pOrgsAftTimestamps;
    delete[] this->pRepsInIds;
    delete[] this->pRepsInTimestamps;
    delete[] this->pRepsAftIds;
    delete[] this->pRepsAftTimestamps;
}


// Auxiliary functions to determine exactly how to scan a partition.
inline bool HINT_M_SubsSort_SS_CM_NoVLs::getBounds(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, vector<pair<Timestamp, Timestamp> >::iterator &iterBegin, vector<pair<Timestamp, Timestamp> >::iterator &iterEnd, RelationIdIterator &iterI)
{
    Offsets_SS_CM_NoVLs_Iterator iterIO = ioffsets[level].begin()+t;
    Offsets_SS_CM_NoVLs_Iterator iterIOEnd = ioffsets[level].end();
    
    if (!iterIO->isEmpty)
    {
        iterI = iterIO->iterI;
        iterBegin = iterIO->iterT;
        iterEnd = ((iterIO+1 != iterIOEnd) ? (iterIO+1)->iterT : timestamps[level].end());
        
        return true;
    }
    else
        return false;
}


inline bool HINT_M_SubsSort_SS_CM_NoVLs::getBounds(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, RelationId *ids, RelationIdIterator &iterIBegin, RelationIdIterator &iterIEnd)
{
    Offsets_SS_CM_NoVLs_Iterator iterIO = ioffsets[level].begin()+t;
    Offsets_SS_CM_NoVLs_Iterator iterIOEnd = ioffsets[level].end();

    if (!iterIO->isEmpty)
    {
        iterIBegin = iterIO->iterI;
        iterIEnd = ((iterIO+1 != iterIOEnd) ? (iterIO+1)->iterI : ids[level].end());

        return true;
    }
    else
        return false;
}


inline bool HINT_M_SubsSort_SS_CM_NoVLs::getBounds(unsigned int level, Timestamp ts, Timestamp te, Offsets_SS_CM_NoVLs *ioffsets, RelationId *ids, RelationIdIterator &iterIBegin, RelationIdIterator &iterIEnd)
{
    Offsets_SS_CM_NoVLs_Iterator iterIO = ioffsets[level].begin()+ts;
    Offsets_SS_CM_NoVLs_Iterator iterIOEnd = ioffsets[level].end();

    while ((iterIO != iterIOEnd) && (iterIO->isEmpty) && (iterIO->tstamp <= te))
           iterIO++;
    
    if ((iterIO != iterIOEnd) && (iterIO->tstamp <= te))
    {
        iterIBegin = iterIO->iterI;
        
        iterIO = ioffsets[level].begin()+te+1;
//        if (iterIO->isEmpty)
//            iterIO++;
        iterIEnd = ((iterIO != iterIOEnd) ? iterIEnd = iterIO->iterI: ids[level].end());
        
        return true;
    }
    else
        return false;
}


inline void HINT_M_SubsSort_SS_CM_NoVLs::scanPartition_CheckBothTimestamps_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, bool (*compare)(const pair<Timestamp, Timestamp>&, const pair<Timestamp, Timestamp>&), Timestamp qstart, Timestamp qend, size_t &result)
{
    vector<pair<Timestamp, Timestamp> >::iterator iter, iterBegin, iterEnd;
    RelationIdIterator iterI;


    if (this->getBounds(level, t, ioffsets, timestamps, iterBegin, iterEnd, iterI))
    {
        vector<pair<Timestamp, Timestamp> >::iterator pivot = lower_bound(iterBegin, iterEnd, make_pair(qend+1, qend+1), compare);
        for (iter = iterBegin; iter != pivot; iter++)
        {
            if (qstart <= iter->second)
            {
#ifdef WORKLOAD_COUNT
                result++;
#else
// std::cout << "Hit ID: " << *iterI << std::endl;
                result ^= (*iterI);
#endif
            }
            iterI++;
        }
    }
}

inline void HINT_M_SubsSort_SS_CM_NoVLs::scanPartition_CheckBothTimestamps_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, bool (*compare)(const pair<Timestamp, Timestamp>&, const pair<Timestamp, Timestamp>&), Timestamp qstart, Timestamp qend, vector<RecordId> &results) {
    vector<pair<Timestamp, Timestamp> >::iterator iter, iterBegin, iterEnd;
    RelationIdIterator iterI;

    if (this->getBounds(level, t, ioffsets, timestamps, iterBegin, iterEnd, iterI)) {
        vector<pair<Timestamp, Timestamp> >::iterator pivot = lower_bound(iterBegin, iterEnd, make_pair(qend+1, qend+1), compare);
        for (iter = iterBegin; iter != pivot; iter++) {
            if (qstart <= iter->second)
                results.push_back(*iterI);
            iterI++;
        }
    }
}

inline void HINT_M_SubsSort_SS_CM_NoVLs::scanPartition_CheckEnd_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, Timestamp qt, size_t &result)
{
    vector<pair<Timestamp, Timestamp> >::iterator iter, iterBegin, iterEnd;
    RelationIdIterator iterI;


    if (this->getBounds(level, t, ioffsets, timestamps, iterBegin, iterEnd, iterI))
    {
        for (iter = iterBegin; iter != iterEnd; iter++)
        {
            if (qt <= iter->second)
            {
#ifdef WORKLOAD_COUNT
                result++;
#else
                result ^= (*iterI);
#endif
            }
            iterI++;
        }
    }
}


inline void HINT_M_SubsSort_SS_CM_NoVLs::scanPartition_CheckEnd_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, bool (*compare)(const pair<Timestamp, Timestamp>&, const pair<Timestamp, Timestamp>&), Timestamp qt, size_t &result)
{
    vector<pair<Timestamp, Timestamp> >::iterator iter, iterBegin, iterEnd;
    RelationIdIterator iterI;


    if (this->getBounds(level, t, ioffsets, timestamps, iterBegin, iterEnd, iterI))
    {
        iter = lower_bound(iterBegin, iterEnd, make_pair(qt, qt), compare);
        iterI += iter-iterBegin;
        while (iter != iterEnd)
        {
#ifdef WORKLOAD_COUNT
            result++;
#else
// std::cout << "Hit ID: " << *iterI << std::endl;
            result ^= (*iterI);
#endif

            iter++;
            iterI++;
        }
    }
}

inline void HINT_M_SubsSort_SS_CM_NoVLs::scanPartition_CheckEnd_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, Timestamp qt, vector<RecordId> &results)
{
    vector<pair<Timestamp, Timestamp> >::iterator iter, iterBegin, iterEnd;
    RelationIdIterator iterI;


    if (this->getBounds(level, t, ioffsets, timestamps, iterBegin, iterEnd, iterI))
    {
        for (iter = iterBegin; iter != iterEnd; iter++)
        {
            if (qt <= iter->second)
                results.push_back(*iterI);
            iterI++;
        }
    }
}


inline void HINT_M_SubsSort_SS_CM_NoVLs::scanPartition_CheckEnd_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, bool (*compare)(const pair<Timestamp, Timestamp>&, const pair<Timestamp, Timestamp>&), Timestamp qt, vector<RecordId> &results) {
    vector<pair<Timestamp, Timestamp> >::iterator iter, iterBegin, iterEnd;
    RelationIdIterator iterI;

    if (this->getBounds(level, t, ioffsets, timestamps, iterBegin, iterEnd, iterI)) {
        iter = lower_bound(iterBegin, iterEnd, make_pair(qt, qt), compare);
        iterI += iter-iterBegin;
        while (iter != iterEnd) {
            results.push_back(*iterI);
            iter++;
            iterI++;
        }
    }
}


inline void HINT_M_SubsSort_SS_CM_NoVLs::scanPartition_CheckStart_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, bool (*compare)(const pair<Timestamp, Timestamp>&, const pair<Timestamp, Timestamp>&), Timestamp qt, size_t &result)
{
    vector<pair<Timestamp, Timestamp> >::iterator iter, iterBegin, iterEnd;
    RelationIdIterator iterI;


    if (this->getBounds(level, t, ioffsets, timestamps, iterBegin, iterEnd, iterI))
    {
        vector<pair<Timestamp, Timestamp> >::iterator pivot = lower_bound(iterBegin, iterEnd, make_pair(qt+1, qt+1), compare);
        for (iter = iterBegin; iter != pivot; iter++)
        {
#ifdef WORKLOAD_COUNT
            result++;
#else
// std::cout << "Hit ID: " << *iterI << std::endl;
            result ^= (*iterI);
#endif

            iterI++;
        }
    }
}

inline void HINT_M_SubsSort_SS_CM_NoVLs::scanPartition_CheckStart_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, bool (*compare)(const pair<Timestamp, Timestamp>&, const pair<Timestamp, Timestamp>&), Timestamp qt, vector<RecordId> &results) {
    vector<pair<Timestamp, Timestamp> >::iterator iter, iterBegin, iterEnd;
    RelationIdIterator iterI;

    if (this->getBounds(level, t, ioffsets, timestamps, iterBegin, iterEnd, iterI)) {
        vector<pair<Timestamp, Timestamp> >::iterator pivot = lower_bound(iterBegin, iterEnd, make_pair(qt+1, qt+1), compare);
        for (iter = iterBegin; iter != pivot; iter++) {
            results.push_back(*iterI);
            iterI++;
        }
    }
}


//inline bool getBounds(unsigned int level, Timestamp t, PartitionId &next_from, Offsets_SS_CM *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, RelationIdIterator &iterIBegin, RelationIdIterator &iterIEnd);
inline void HINT_M_SubsSort_SS_CM_NoVLs::scanPartition_NoChecks_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, RelationId *ids, size_t &result)
{
    RelationIdIterator iterI, iterIBegin, iterIEnd;

    if (this->getBounds(level, t, ioffsets, ids, iterIBegin, iterIEnd))
    {
        for (iterI = iterIBegin; iterI != iterIEnd; iterI++)
        {
#ifdef WORKLOAD_COUNT
            result++;
#else
// std::cout << "Hit ID: " << *iterI << std::endl;
            result ^= (*iterI);
#endif
        }
    }
}


inline void HINT_M_SubsSort_SS_CM_NoVLs::scanPartition_NoChecks_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, RelationId *ids, vector<RecordId> &results) {
    RelationIdIterator iterI, iterIBegin, iterIEnd;

    if (this->getBounds(level, t, ioffsets, ids, iterIBegin, iterIEnd)) {
        for (iterI = iterIBegin; iterI != iterIEnd; iterI++)
            results.push_back(*iterI);
    }
}


inline void HINT_M_SubsSort_SS_CM_NoVLs::scanPartitions_NoChecks_gOverlaps(unsigned int level, Timestamp ts, Timestamp te, Offsets_SS_CM_NoVLs *ioffsets, RelationId *ids, size_t &result)
{
    RelationIdIterator iterI, iterIBegin, iterIEnd;

    if (this->getBounds(level, ts, te, ioffsets, ids, iterIBegin, iterIEnd))
    {
//        cout << "from: " << *iterIBegin << " to " << *iterIEnd << endl;
        for (iterI = iterIBegin; iterI != iterIEnd; iterI++)
        {
#ifdef WORKLOAD_COUNT
            result++;
#else
            result ^= (*iterI);
#endif
        }
    }
}

inline void HINT_M_SubsSort_SS_CM_NoVLs::scanPartitions_NoChecks_gOverlaps(unsigned int level, Timestamp ts, Timestamp te, Offsets_SS_CM_NoVLs *ioffsets, RelationId *ids, vector<RecordId> &results) {
    RelationIdIterator iterI, iterIBegin, iterIEnd;

    if (this->getBounds(level, ts, te, ioffsets, ids, iterIBegin, iterIEnd)) {
        for (iterI = iterIBegin; iterI != iterIEnd; iterI++)
            results.push_back(*iterI);
    }
}


// Generalized predicates, ACM SIGMOD'22 gOverlaps
#ifdef SIMPLIFIED
size_t HINT_M_SubsSort_SS_CM_NoVLs::executeBottomUp_Stabbing(const RangeQuery &q)
{
    size_t result = 0;
    vector<pair<Timestamp, Timestamp> >::iterator iter, iterBegin, iterEnd;
    RelationIdIterator iterI, iterIStart, iterIEnd;
    Timestamp a = q.start >> (this->maxBits-this->numBits); // prefix
    bool foundzero = false;
    bool foundone = false;


    for (auto l = 0; l < this->numBits; l++)
    {
        if (foundone && foundzero)
        {
            // Partition totally covers lowest-level partition range that includes query range
            // all contents are guaranteed to be results

            // Handle the partition that contains a: consider both originals and replicas
            this->scanPartition_NoChecks_gOverlaps(l, a, this->pOrgsIn_ioffsets, this->pOrgsInIds, result);
            this->scanPartition_NoChecks_gOverlaps(l, a, this->pOrgsAft_ioffsets, this->pOrgsAftIds, result);

            this->scanPartition_NoChecks_gOverlaps(l, a, this->pRepsIn_ioffsets, this->pRepsInIds, result);
            this->scanPartition_NoChecks_gOverlaps(l, a, this->pRepsAft_ioffsets, this->pRepsAftIds, result);

        }
        else
        {
            // Comparisons needed

            // Handle the partition that contains a: consider both originals and replicas, comparisons needed
            this->scanPartition_CheckBothTimestamps_gOverlaps(l, a, this->pOrgsIn_ioffsets, this->pOrgsInTimestamps, CompareTimestampPairsByStart, q.start, q.start, result);
            this->scanPartition_CheckStart_gOverlaps(l, a, this->pOrgsAft_ioffsets, this->pOrgsAftTimestamps, CompareTimestampPairsByStart, q.start, result);
            this->scanPartition_CheckEnd_gOverlaps(l, a, this->pRepsIn_ioffsets, this->pRepsInTimestamps, CompareTimestampPairsByEnd, q.start, result);
            this->scanPartition_NoChecks_gOverlaps(l, a, this->pRepsAft_ioffsets, this->pRepsAftIds, result);

            if (a%2) //last bit of b is 1
                foundone = 1;
            if (!(a%2)) //last bit of a is 0
                foundzero = 1;
        }
        a >>= 1; // a = a div 2
    }

    // Handle root.
    if (foundone && foundzero)
    {
        // All contents are guaranteed to be results
        this->scanPartition_NoChecks_gOverlaps(this->numBits, a, this->pOrgsIn_ioffsets, this->pOrgsInIds, result);
    }
    else
    {
        // Comparisons needed
        this->scanPartition_CheckBothTimestamps_gOverlaps(this->numBits, a, this->pOrgsIn_ioffsets, this->pOrgsInTimestamps, CompareTimestampPairsByEnd, q.start, q.end, result);
    }


    return result;
}
#else
size_t HINT_M_SubsSort_SS_CM_NoVLs::executeBottomUp_Stabbing(const RangeQuery &q)
{
    size_t result = 0;
    vector<pair<Timestamp, Timestamp> >::iterator iter, iterBegin, iterEnd;
    RelationIdIterator iterI, iterIStart, iterIEnd;
    Timestamp a = q.start >> (this->maxBits-this->numBits); // prefix
    bool foundzero = false;
    bool foundone = false;


    for (auto l = 0; l < this->numBits; l++)
    {
        if (foundone && foundzero)
        {
            // Partition totally covers lowest-level partition range that includes query range
            // all contents are guaranteed to be results

            // Handle the partition that contains a: consider both originals and replicas
            this->scanPartition_NoChecks_gOverlaps(l, a, this->pOrgsIn_ioffsets, this->pOrgsInIds, result);
            this->scanPartition_NoChecks_gOverlaps(l, a, this->pOrgsAft_ioffsets, this->pOrgsAftIds, result);

            this->scanPartition_NoChecks_gOverlaps(l, a, this->pRepsIn_ioffsets, this->pRepsInIds, result);
            this->scanPartition_NoChecks_gOverlaps(l, a, this->pRepsAft_ioffsets, this->pRepsAftIds, result);

        }
        else
        {
            // Comparisons needed

            // Handle the partition that contains a: consider both originals and replicas, comparisons needed
            if (!foundzero && !foundone)
            {
                this->scanPartition_CheckBothTimestamps_gOverlaps(l, a, this->pOrgsIn_ioffsets, this->pOrgsInTimestamps, CompareTimestampPairsByStart, q.start, q.end, result);
                this->scanPartition_CheckStart_gOverlaps(l, a, this->pOrgsAft_ioffsets, this->pOrgsAftTimestamps, CompareTimestampPairsByStart, q.end, result);
            }
            else if (foundzero)
            {
                this->scanPartition_CheckStart_gOverlaps(l, a, this->pOrgsIn_ioffsets, this->pOrgsInTimestamps, CompareTimestampPairsByStart, q.end, result);
                this->scanPartition_CheckStart_gOverlaps(l, a, this->pOrgsAft_ioffsets, this->pOrgsAftTimestamps, CompareTimestampPairsByStart, q.end, result);
            }
            else if (foundone)
            {
//                this->scanPartition_CheckEnd_gOverlaps(l, a, this->pOrgsInCopy_ioffsets, this->pOrgsInCopyTimestamps, CompareTimestampPairsByEnd, q.start, result);
                this->scanPartition_CheckEnd_gOverlaps(l, a, this->pOrgsIn_ioffsets, this->pOrgsInTimestamps, q.start, result);
                this->scanPartition_NoChecks_gOverlaps(l, a, this->pOrgsAft_ioffsets, this->pOrgsAftIds, result);
            }

            if (!foundzero)
            {
                this->scanPartition_CheckEnd_gOverlaps(l, a, this->pRepsIn_ioffsets, this->pRepsInTimestamps, CompareTimestampPairsByEnd, q.start, result);
            }
            else
            {
                this->scanPartition_NoChecks_gOverlaps(l, a, this->pRepsIn_ioffsets, this->pRepsInIds, result);
            }
            this->scanPartition_NoChecks_gOverlaps(l, a, this->pRepsAft_ioffsets, this->pRepsAftIds, result);

            if (a%2) //last bit of b is 1
                foundone = 1;
            if (!(a%2)) //last bit of a is 0
                foundzero = 1;
        }
        a >>= 1; // a = a div 2
    }

    // Handle root.
    if (foundone && foundzero)
    {
        // All contents are guaranteed to be results
        this->scanPartition_NoChecks_gOverlaps(this->numBits, a, this->pOrgsIn_ioffsets, this->pOrgsInIds, result);
    }
    else
    {
        // Comparisons needed
        this->scanPartition_CheckBothTimestamps_gOverlaps(this->numBits, a, this->pOrgsIn_ioffsets, this->pOrgsInTimestamps, CompareTimestampPairsByEnd, q.start, q.end, result);
    }


    return result;
}
#endif


size_t HINT_M_SubsSort_SS_CM_NoVLs::executeBottomUp_gOverlaps(const RangeQuery &q)
{
    size_t result = 0;
    vector<pair<Timestamp, Timestamp> >::iterator iter, iterBegin, iterEnd;
    RelationIdIterator iterI, iterIBegin, iterIEnd;
    Timestamp a = q.start >> (this->maxBits-this->numBits); // prefix
    Timestamp b = q.end   >> (this->maxBits-this->numBits); // prefix
    bool foundzero = false;
    bool foundone = false;
    
    
    for (auto l = 0; l < this->numBits; l++)
    {
        if (foundone && foundzero)
        {
            // Partition totally covers lowest-level partition range that includes query range
            // all contents are guaranteed to be results
            
            // Handle the partition that contains a: consider both originals and replicas
            this->scanPartition_NoChecks_gOverlaps(l, a, this->pRepsIn_ioffsets, this->pRepsInIds, result);
            this->scanPartition_NoChecks_gOverlaps(l, a, this->pRepsAft_ioffsets, this->pRepsAftIds, result);
            
            this->scanPartitions_NoChecks_gOverlaps(l, a, b, this->pOrgsIn_ioffsets, this->pOrgsInIds, result);
            this->scanPartitions_NoChecks_gOverlaps(l, a, b, this->pOrgsAft_ioffsets, this->pOrgsAftIds, result);
        }
        else
        {
            // Comparisons needed

            // Handle the partition that contains a: consider both originals and replicas, comparisons needed
            if (a == b)
            {
                // Special case when query overlaps only one partition, Lemma 3
                if (!foundzero && !foundone)
                {
                    this->scanPartition_CheckBothTimestamps_gOverlaps(l, a, this->pOrgsIn_ioffsets, this->pOrgsInTimestamps, CompareTimestampPairsByStart, q.start, q.end, result);
                    this->scanPartition_CheckStart_gOverlaps(l, a, this->pOrgsAft_ioffsets, this->pOrgsAftTimestamps, CompareTimestampPairsByStart, q.end, result);
                }
                else if (foundzero)
                {
                    this->scanPartition_CheckStart_gOverlaps(l, a, this->pOrgsIn_ioffsets, this->pOrgsInTimestamps, CompareTimestampPairsByStart, q.end, result);
                    this->scanPartition_CheckStart_gOverlaps(l, a, this->pOrgsAft_ioffsets, this->pOrgsAftTimestamps, CompareTimestampPairsByStart, q.end, result);
                }
                else if (foundone)
                {
//                    this->scanPartition_CheckEnd_gOverlaps(l, a, this->pOrgsInCopy_ioffsets, this->pOrgsInCopyTimestamps, CompareTimestampPairsByEnd, q.start, result);
                    this->scanPartition_CheckEnd_gOverlaps(l, a, this->pOrgsIn_ioffsets, this->pOrgsInTimestamps, q.start, result);
//                    this->scanPartition_CheckBothTimestamps_gOverlaps(l, a, this->pOrgsIn_ioffsets, this->pOrgsInTimestamps, CompareTimestampPairsByStart, q.start, q.end, result);

                    this->scanPartition_NoChecks_gOverlaps(l, a, this->pOrgsAft_ioffsets, this->pOrgsAftIds, result);
                }
            }
            else
            {
                // Lemma 1
                if (!foundzero)
                {
//                    this->scanPartition_CheckEnd_gOverlaps(l, a, this->pOrgsInCopy_ioffsets, this->pOrgsInCopyTimestamps, CompareTimestampPairsByEnd, q.start, result);
                    this->scanPartition_CheckEnd_gOverlaps(l, a, this->pOrgsIn_ioffsets, this->pOrgsInTimestamps, q.start, result);
//                    this->scanPartition_CheckBothTimestamps_gOverlaps(l, a, this->pOrgsIn_ioffsets, this->pOrgsInTimestamps, CompareTimestampPairsByStart, q.start, q.end, result);
                }
                else
                {
                    this->scanPartition_NoChecks_gOverlaps(l, a, this->pOrgsIn_ioffsets, this->pOrgsInIds, result);
                }
                this->scanPartition_NoChecks_gOverlaps(l, a, this->pOrgsAft_ioffsets, this->pOrgsAftIds, result);
            }
            
            // Lemma 1, 3
            if (!foundzero)
            {
                this->scanPartition_CheckEnd_gOverlaps(l, a, this->pRepsIn_ioffsets, this->pRepsInTimestamps, CompareTimestampPairsByEnd, q.start, result);
            }
            else
            {
                this->scanPartition_NoChecks_gOverlaps(l, a, this->pRepsIn_ioffsets, this->pRepsInIds, result);
            }
            this->scanPartition_NoChecks_gOverlaps(l, a, this->pRepsAft_ioffsets, this->pRepsAftIds, result);
            
            if (a < b)
            {
                if (!foundone)
                {
                    // Handle the rest before the partition that contains b: consider only originals, no comparisons needed
                    this->scanPartitions_NoChecks_gOverlaps(l, a+1, b-1, this->pOrgsIn_ioffsets, this->pOrgsInIds, result);
                    this->scanPartitions_NoChecks_gOverlaps(l, a+1, b-1, this->pOrgsAft_ioffsets, this->pOrgsAftIds, result);

                    // Handle the partition that contains b: consider only originals, comparisons needed
                    this->scanPartition_CheckStart_gOverlaps(l, b, this->pOrgsIn_ioffsets, this->pOrgsInTimestamps, CompareTimestampPairsByStart, q.end, result);
                    this->scanPartition_CheckStart_gOverlaps(l, b, this->pOrgsAft_ioffsets, this->pOrgsAftTimestamps, CompareTimestampPairsByStart, q.end, result);
                }
                else
                {
                    this->scanPartitions_NoChecks_gOverlaps(l, a+1, b, this->pOrgsIn_ioffsets, this->pOrgsInIds, result);
                    this->scanPartitions_NoChecks_gOverlaps(l, a+1, b, this->pOrgsAft_ioffsets, this->pOrgsAftIds, result);
                }
            }
            
            if ((!foundone) && (b%2)) //last bit of b is 1
                foundone = 1;
            if ((!foundzero) && (!(a%2))) //last bit of a is 0
                foundzero = 1;
        }
        a >>= 1; // a = a div 2
        b >>= 1; // b = b div 2
    }
    
    // Handle root.
    if (foundone && foundzero)
    {
        // All contents are guaranteed to be results
        this->scanPartition_NoChecks_gOverlaps(this->numBits, a, this->pOrgsIn_ioffsets, this->pOrgsInIds, result);
    }
    else
    {
        // Comparisons needed
        this->scanPartition_CheckBothTimestamps_gOverlaps(this->numBits, a, this->pOrgsIn_ioffsets, this->pOrgsInTimestamps, CompareTimestampPairsByStart, q.start, q.end, result);
    }


    return result;
}



void HINT_M_SubsSort_SS_CM_NoVLs::executeBottomUp_gOverlaps(const RangeQuery &q, vector<RecordId> &results) {
    vector<pair<Timestamp, Timestamp> >::iterator iter, iterBegin, iterEnd;
    RelationIdIterator iterI, iterIBegin, iterIEnd;
    Timestamp a = q.start >> (this->maxBits-this->numBits); // prefix
    Timestamp b = q.end   >> (this->maxBits-this->numBits); // prefix
    bool foundzero = false;
    bool foundone = false;
    
    for (auto l = 0; l < this->numBits; l++) {
        if (foundone && foundzero) {
            this->scanPartition_NoChecks_gOverlaps(l, a, this->pRepsIn_ioffsets, this->pRepsInIds, results);
            this->scanPartition_NoChecks_gOverlaps(l, a, this->pRepsAft_ioffsets, this->pRepsAftIds, results);
            
            this->scanPartitions_NoChecks_gOverlaps(l, a, b, this->pOrgsIn_ioffsets, this->pOrgsInIds, results);
            this->scanPartitions_NoChecks_gOverlaps(l, a, b, this->pOrgsAft_ioffsets, this->pOrgsAftIds, results);
        }
        else {
            if (a == b) {
                if (!foundzero && !foundone) {
                    this->scanPartition_CheckBothTimestamps_gOverlaps(l, a, this->pOrgsIn_ioffsets, this->pOrgsInTimestamps, CompareTimestampPairsByStart, q.start, q.end, results);
                    this->scanPartition_CheckStart_gOverlaps(l, a, this->pOrgsAft_ioffsets, this->pOrgsAftTimestamps, CompareTimestampPairsByStart, q.end, results);
                }
                else if (foundzero) {
                    this->scanPartition_CheckStart_gOverlaps(l, a, this->pOrgsIn_ioffsets, this->pOrgsInTimestamps, CompareTimestampPairsByStart, q.end, results);
                    this->scanPartition_CheckStart_gOverlaps(l, a, this->pOrgsAft_ioffsets, this->pOrgsAftTimestamps, CompareTimestampPairsByStart, q.end, results);
                }
                else if (foundone) {
//                    this->scanPartition_CheckEnd_gOverlaps(l, a, this->pOrgsInCopy_ioffsets, this->pOrgsInCopyTimestamps, CompareTimestampPairsByEnd, q.start, results);
                    this->scanPartition_CheckEnd_gOverlaps(l, a, this->pOrgsIn_ioffsets, this->pOrgsInTimestamps, q.start, results);

                    this->scanPartition_NoChecks_gOverlaps(l, a, this->pOrgsAft_ioffsets, this->pOrgsAftIds, results);
                }
            }
            else {
                // Lemma 1
                if (!foundzero) 
//                    this->scanPartition_CheckEnd_gOverlaps(l, a, this->pOrgsInCopy_ioffsets, this->pOrgsInCopyTimestamps, CompareTimestampPairsByEnd, q.start, results);
                    this->scanPartition_CheckEnd_gOverlaps(l, a, this->pOrgsIn_ioffsets, this->pOrgsInTimestamps, q.start, results);
                else
                    this->scanPartition_NoChecks_gOverlaps(l, a, this->pOrgsIn_ioffsets, this->pOrgsInIds, results);
                this->scanPartition_NoChecks_gOverlaps(l, a, this->pOrgsAft_ioffsets, this->pOrgsAftIds, results);
            }
            
            // Lemma 1, 3
            if (!foundzero)
                this->scanPartition_CheckEnd_gOverlaps(l, a, this->pRepsIn_ioffsets, this->pRepsInTimestamps, CompareTimestampPairsByEnd, q.start, results);
            else
                this->scanPartition_NoChecks_gOverlaps(l, a, this->pRepsIn_ioffsets, this->pRepsInIds, results);
            this->scanPartition_NoChecks_gOverlaps(l, a, this->pRepsAft_ioffsets, this->pRepsAftIds, results);
            
            if (a < b) {
                if (!foundone) {
                    this->scanPartitions_NoChecks_gOverlaps(l, a+1, b-1, this->pOrgsIn_ioffsets, this->pOrgsInIds, results);
                    this->scanPartitions_NoChecks_gOverlaps(l, a+1, b-1, this->pOrgsAft_ioffsets, this->pOrgsAftIds, results);

                    this->scanPartition_CheckStart_gOverlaps(l, b, this->pOrgsIn_ioffsets, this->pOrgsInTimestamps, CompareTimestampPairsByStart, q.end, results);
                    this->scanPartition_CheckStart_gOverlaps(l, b, this->pOrgsAft_ioffsets, this->pOrgsAftTimestamps, CompareTimestampPairsByStart, q.end, results);
                }
                else {
                    this->scanPartitions_NoChecks_gOverlaps(l, a+1, b, this->pOrgsIn_ioffsets, this->pOrgsInIds, results);
                    this->scanPartitions_NoChecks_gOverlaps(l, a+1, b, this->pOrgsAft_ioffsets, this->pOrgsAftIds, results);
                }
            }
            
            if ((!foundone) && (b%2)) //last bit of b is 1
                foundone = 1;
            if ((!foundzero) && (!(a%2))) //last bit of a is 0
                foundzero = 1;
        }
        a >>= 1; // a = a div 2
        b >>= 1; // b = b div 2
    }
    
    // Handle root.
    if (foundone && foundzero)
        this->scanPartition_NoChecks_gOverlaps(this->numBits, a, this->pOrgsIn_ioffsets, this->pOrgsInIds, results);
    else
        this->scanPartition_CheckBothTimestamps_gOverlaps(this->numBits, a, this->pOrgsIn_ioffsets, this->pOrgsInTimestamps, CompareTimestampPairsByStart, q.start, q.end, results);    
}
