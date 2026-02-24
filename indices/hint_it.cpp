#include "hint_it.h"
#include "../utils/weighted_sampling.hpp"
 


inline void HINT_IT_SubsSort_SS_CM_NoVLs::updateCounters(const Record &r) {
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


inline void HINT_IT_SubsSort_SS_CM_NoVLs::updatePartitions(const Record &r) {
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
                    this->pOrgsInTmp[level][this->pOrgsIn_offsets[level][a]] = Record(r.id, r.start, r.end);
                    this->pOrgsInCopyTmp[level][this->pOrgsIn_offsets[level][a]++] = Record(r.id, r.start, r.end);
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
                    this->pOrgsInTmp[level][this->pOrgsIn_offsets[level][prevb]] = Record(r.id, r.start, r.end);
                    this->pOrgsInCopyTmp[level][this->pOrgsIn_offsets[level][prevb]++] = Record(r.id, r.start, r.end);
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


HINT_IT_SubsSort_SS_CM_NoVLs::HINT_IT_SubsSort_SS_CM_NoVLs(const Relation &R, const unsigned int numBits, const unsigned int maxBits)  : HierarchicalIndex(R, numBits, maxBits){
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
    this->pOrgsInCopyTmp  = new Relation[this->height];
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
            this->pOrgsAft_offsets[l][pId] = sumOaft;
            this->pRepsIn_offsets[l][pId]  = sumRin;
            this->pRepsAft_offsets[l][pId] = sumRaft;
            sumOin  += this->pOrgsIn_sizes[l][pId];
            sumOaft += this->pOrgsAft_sizes[l][pId];
            sumRin  += this->pRepsIn_sizes[l][pId];
            sumRaft += this->pRepsAft_sizes[l][pId];
        }
        this->pOrgsIn_offsets[l][cnt]  = sumOin;
        this->pOrgsAft_offsets[l][cnt] = sumOaft;
        this->pRepsIn_offsets[l][cnt]  = sumRin;
        this->pRepsAft_offsets[l][cnt] = sumRaft;
        
        this->pOrgsInTmp[l].resize(sumOin);
        this->pOrgsInCopyTmp[l].resize(sumOin);
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
            this->pOrgsAft_offsets[l][pId] = sumOaft;
            this->pRepsIn_offsets[l][pId]  = sumRin;
            this->pRepsAft_offsets[l][pId] = sumRaft;
            sumOin  += this->pOrgsIn_sizes[l][pId];
            sumOaft += this->pOrgsAft_sizes[l][pId];
            sumRin  += this->pRepsIn_sizes[l][pId];
            sumRaft += this->pRepsAft_sizes[l][pId];
        }
        this->pOrgsIn_offsets[l][cnt]  = sumOin;
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
            sort(this->pOrgsInCopyTmp[l].begin()+this->pOrgsIn_offsets[l][pId], this->pOrgsInCopyTmp[l].begin()+this->pOrgsIn_offsets[l][pId+1], CompareRecordsByEnd);
            sort(this->pOrgsAftTmp[l].begin()+this->pOrgsAft_offsets[l][pId], this->pOrgsAftTmp[l].begin()+this->pOrgsAft_offsets[l][pId+1]);
            sort(this->pRepsInTmp[l].begin()+this->pRepsIn_offsets[l][pId], this->pRepsInTmp[l].begin()+this->pRepsIn_offsets[l][pId+1], CompareRecordsByEnd);
            sort(this->pRepsAftTmp[l].begin()+this->pRepsAft_offsets[l][pId], this->pRepsAftTmp[l].begin()+this->pRepsAft_offsets[l][pId+1], CompareRecordsByEnd);
        }
    }

    
    // Step 5: break-down data to create id- and timestamp-dedicated arrays; free auxiliary memory.
    this->pOrgsInIds  = new RelationId[this->height];
    this->pOrgsInCopyIds  = new RelationId[this->height];
    this->pOrgsAftIds = new RelationId[this->height];
    this->pRepsInIds  = new RelationId[this->height];
    this->pRepsAftIds = new RelationId[this->height];
    this->pOrgsInTimestamps  = new vector<pair<Timestamp, Timestamp> >[this->height];
    this->pOrgsInCopyTimestamps  = new vector<pair<Timestamp, Timestamp> >[this->height];
    this->pOrgsAftTimestamps = new vector<pair<Timestamp, Timestamp> >[this->height];
    this->pRepsInTimestamps  = new vector<pair<Timestamp, Timestamp> >[this->height];
    this->pRepsAftTimestamps = new vector<pair<Timestamp, Timestamp> >[this->height];
    for (auto l = 0; l < this->height; l++)
    {
        auto cnt = pOrgsInTmp[l].size();
        
        this->pOrgsInIds[l].resize(cnt);
        this->pOrgsInCopyIds[l].resize(cnt);
        this->pOrgsInTimestamps[l].resize(cnt);
        this->pOrgsInCopyTimestamps[l].resize(cnt);
        for (auto j = 0; j < cnt; j++)
        {
            this->pOrgsInIds[l][j] = this->pOrgsInTmp[l][j].id;
            this->pOrgsInTimestamps[l][j].first = this->pOrgsInTmp[l][j].start;
            this->pOrgsInTimestamps[l][j].second = this->pOrgsInTmp[l][j].end;
            this->pOrgsInCopyIds[l][j] = this->pOrgsInCopyTmp[l][j].id;
            this->pOrgsInCopyTimestamps[l][j].first = this->pOrgsInCopyTmp[l][j].start;
            this->pOrgsInCopyTimestamps[l][j].second = this->pOrgsInCopyTmp[l][j].end;
        }

        if (l == 0) {
           cnt = (int)(pow(2, this->numBits-l));
           this->pOrgsInTrees.resize(cnt);
           for (auto pId = 0; pId < cnt; pId++) {
               if (this->pOrgsIn_sizes[l][pId] > 0)
                   this->pOrgsInTrees[pId] = new IntervalTree(this->pOrgsInTmp[l].begin()+this->pOrgsIn_offsets[l][pId], this->pOrgsInTmp[l].begin()+this->pOrgsIn_offsets[l][pId+1]);
           }
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
        free(this->pOrgsAft_offsets[l]);
        free(this->pRepsIn_offsets[l]);
        free(this->pRepsAft_offsets[l]);
    }
   
   free(this->pOrgsIn_offsets);
   free(this->pOrgsAft_offsets);
   free(this->pRepsIn_offsets);
   free(this->pRepsAft_offsets);
   
    delete[] this->pOrgsInTmp;
    delete[] this->pOrgsInCopyTmp;
    delete[] this->pOrgsAftTmp;
    delete[] this->pRepsInTmp;
    delete[] this->pRepsAftTmp;
    

    // Step 4: create offset pointers
    this->pOrgsIn_ioffsets  = new Offsets_SS_CM_NoVLs[this->height];
    this->pOrgsInCopy_ioffsets  = new Offsets_SS_CM_NoVLs[this->height];
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
            
            isEmpty = (this->pOrgsIn_sizes[l][pId] == 0);
            this->pOrgsIn_ioffsets[l].push_back(OffsetEntry_SS_CM_NoVLs(pId, this->pOrgsInIds[l].begin()+sumOin, this->pOrgsInTimestamps[l].begin()+sumOin, isEmpty));
            
            isEmpty = (this->pOrgsIn_sizes[l][pId] == 0);
            this->pOrgsInCopy_ioffsets[l].push_back(OffsetEntry_SS_CM_NoVLs(pId, this->pOrgsInCopyIds[l].begin()+sumOin, this->pOrgsInCopyTimestamps[l].begin()+sumOin, isEmpty));

            isEmpty = (this->pOrgsAft_sizes[l][pId] == 0);
            this->pOrgsAft_ioffsets[l].push_back(OffsetEntry_SS_CM_NoVLs(pId, this->pOrgsAftIds[l].begin()+sumOaft, this->pOrgsAftTimestamps[l].begin()+sumOaft, isEmpty));
            
            isEmpty = (this->pRepsIn_sizes[l][pId] == 0);
            this->pRepsIn_ioffsets[l].push_back(OffsetEntry_SS_CM_NoVLs(pId, this->pRepsInIds[l].begin()+sumRin, this->pRepsInTimestamps[l].begin()+sumRin, isEmpty));
            
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


void HINT_IT_SubsSort_SS_CM_NoVLs::getStats() {
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


size_t HINT_IT_SubsSort_SS_CM_NoVLs::getSize() {
    size_t size = sizeof(*this);

    for (auto l = 0; l < this->height; l++) {
        auto numPartitions = pow(2, this->numBits-l);

        if (l == 0) {
            size += this->pOrgsInTrees.capacity() * sizeof(IntervalTree*);
            for (auto tree : this->pOrgsInTrees) {
                if (tree != nullptr)
                    size += tree->getSize();
            }
        }

        for (auto i = 0; i < numPartitions; i++) {
            size += this->pOrgsInIds[l][i].capacity() * sizeof(RecordId);
            size += this->pOrgsInCopyIds[l][i].capacity() * sizeof(RecordId);
            size += this->pOrgsAftIds[l][i].capacity() * sizeof(RecordId);
            size += this->pRepsInIds[l][i].capacity() * sizeof(RecordId);
            size += this->pRepsAftIds[l][i].capacity() * sizeof(RecordId);

            size += this->pOrgsInTimestamps[l][i].capacity() * sizeof(pair<Timestamp, Timestamp>);
            size += this->pOrgsInCopyTimestamps[l][i].capacity() * sizeof(pair<Timestamp, Timestamp>);
            size += this->pOrgsAftTimestamps[l][i].capacity() * sizeof(pair<Timestamp, Timestamp>);
            size += this->pRepsInTimestamps[l][i].capacity() * sizeof(pair<Timestamp, Timestamp>);
            size += this->pRepsAftTimestamps[l][i].capacity() * sizeof(pair<Timestamp, Timestamp>);
        }

        size += numPartitions * sizeof(RelationId);
        size += numPartitions * sizeof(vector<pair<Timestamp, Timestamp>>);
        size += numPartitions * sizeof(RelationId);
        size += numPartitions * sizeof(vector<pair<Timestamp, Timestamp>>);
        size += numPartitions * sizeof(RelationId);
        size += numPartitions * sizeof(vector<pair<Timestamp, Timestamp>>);
        size += numPartitions * sizeof(RelationId);
        size += numPartitions * sizeof(vector<pair<Timestamp, Timestamp>>);
        size += numPartitions * sizeof(RelationId);
        size += numPartitions * sizeof(vector<pair<Timestamp, Timestamp>>);

        size += this->pOrgsIn_ioffsets[l].size() * sizeof(Offsets_SS_CM_NoVLs);
        size += this->pOrgsAft_ioffsets[l].size() * sizeof(Offsets_SS_CM_NoVLs);
        size += this->pRepsIn_ioffsets[l].size() * sizeof(Offsets_SS_CM_NoVLs);
        size += this->pRepsAft_ioffsets[l].size() * sizeof(Offsets_SS_CM_NoVLs);
    }

    return size;
}


HINT_IT_SubsSort_SS_CM_NoVLs::~HINT_IT_SubsSort_SS_CM_NoVLs() {
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
    
    for (auto tree : this->pOrgsInTrees) {
        if (tree != nullptr)
            delete tree;
    }
}


// Auxiliary functions to determine exactly how to scan a partition.
inline bool HINT_IT_SubsSort_SS_CM_NoVLs::getBounds(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, vector<pair<Timestamp, Timestamp> >::iterator &iterBegin, vector<pair<Timestamp, Timestamp> >::iterator &iterEnd, RelationIdIterator &iterI) {
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


inline bool HINT_IT_SubsSort_SS_CM_NoVLs::getBounds(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, RelationId *ids, RelationIdIterator &iterIBegin, RelationIdIterator &iterIEnd) {
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


inline bool HINT_IT_SubsSort_SS_CM_NoVLs::getBounds(unsigned int level, Timestamp ts, Timestamp te, Offsets_SS_CM_NoVLs *ioffsets, RelationId *ids, RelationIdIterator &iterIBegin, RelationIdIterator &iterIEnd) {
    Offsets_SS_CM_NoVLs_Iterator iterIO = ioffsets[level].begin()+ts;
    Offsets_SS_CM_NoVLs_Iterator iterIOEnd = ioffsets[level].end();

    while ((iterIO != iterIOEnd) && (iterIO->isEmpty) && (iterIO->tstamp <= te))
           iterIO++;
    
    if ((iterIO != iterIOEnd) && (iterIO->tstamp <= te))
    {
        iterIBegin = iterIO->iterI;
        
        iterIO = ioffsets[level].begin()+te+1;
        iterIEnd = ((iterIO != iterIOEnd) ? iterIEnd = iterIO->iterI: ids[level].end());
        
        return true;
    }
    else
        return false;
}


inline void HINT_IT_SubsSort_SS_CM_NoVLs::scanPartition_CheckBothTimestamps_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, bool (*compare)(const pair<Timestamp, Timestamp>&, const pair<Timestamp, Timestamp>&), Timestamp qstart, Timestamp qend, size_t &result) {
       vector<pair<Timestamp, Timestamp> >::iterator iter, iterBegin, iterEnd;
       RelationIdIterator iterI;

       if (this->getBounds(level, t, ioffsets, timestamps, iterBegin, iterEnd, iterI)) {
           if (level == 0) {
               RangeQuery q(0, qstart, qend);
#ifdef WORKLOAD_COUNT
               result += this->pOrgsInTrees[t]->execute_gOverlaps(q); // Question: Shouldnt this be stabbing?
#else
               result ^= this->pOrgsInTrees[t]->execute_gOverlaps(q);
#endif
           }
           else {
               vector<pair<Timestamp, Timestamp> >::iterator pivot = lower_bound(iterBegin, iterEnd, make_pair(qend+1, qend+1), compare);
               for (iter = iterBegin; iter != pivot; iter++) {
                   if (qstart <= iter->second) {
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
   }


inline void HINT_IT_SubsSort_SS_CM_NoVLs::scanPartition_CheckEnd_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, Timestamp qt, size_t &result)
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


inline void HINT_IT_SubsSort_SS_CM_NoVLs::scanPartition_CheckEnd_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, bool (*compare)(const pair<Timestamp, Timestamp>&, const pair<Timestamp, Timestamp>&), Timestamp qt, size_t &result)
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


inline void HINT_IT_SubsSort_SS_CM_NoVLs::scanPartition_CheckStart_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, bool (*compare)(const pair<Timestamp, Timestamp>&, const pair<Timestamp, Timestamp>&), Timestamp qt, size_t &result)
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
//
//
//inline bool getBounds(unsigned int level, Timestamp t, PartitionId &next_from, Offsets_SS_CM *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, RelationIdIterator &iterIBegin, RelationIdIterator &iterIEnd);
inline void HINT_IT_SubsSort_SS_CM_NoVLs::scanPartition_NoChecks_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, RelationId *ids, size_t &result)
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


inline void HINT_IT_SubsSort_SS_CM_NoVLs::scanPartitions_NoChecks_gOverlaps(unsigned int level, Timestamp ts, Timestamp te, Offsets_SS_CM_NoVLs *ioffsets, RelationId *ids, size_t &result)
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


size_t HINT_IT_SubsSort_SS_CM_NoVLs::executeBottomUp_Stabbing(const RangeQuery &q)
{
    size_t result = 0;
    vector<pair<Timestamp, Timestamp> >::iterator iter, iterBegin, iterEnd;
    RelationIdIterator iterI, iterIStart, iterIEnd;
    Timestamp a = q.start >> (this->maxBits-this->numBits); // prefix
    bool foundzero = false;
    bool foundone = false;


    for (auto l = 0; l < this->numBits; l++) {
        if (foundone && foundzero){
            // Partition totally covers lowest-level partition range that includes query range
            // all contents are guaranteed to be results
            // Handle the partition that contains a: consider both originals and replicas
            this->scanPartition_NoChecks_gOverlaps(l, a, this->pOrgsIn_ioffsets, this->pOrgsInIds, result);
            this->scanPartition_NoChecks_gOverlaps(l, a, this->pOrgsAft_ioffsets, this->pOrgsAftIds, result);
            this->scanPartition_NoChecks_gOverlaps(l, a, this->pRepsIn_ioffsets, this->pRepsInIds, result);
            this->scanPartition_NoChecks_gOverlaps(l, a, this->pRepsAft_ioffsets, this->pRepsAftIds, result);
        }
        else {
            // Comparisons needed
            // Handle the partition that contains a: consider both originals and replicas, comparisons needed
            if (!foundzero && !foundone) {
                this->scanPartition_CheckBothTimestamps_gOverlaps(l, a, this->pOrgsIn_ioffsets, this->pOrgsInTimestamps, CompareTimestampPairsByStart, q.start, q.end, result);
                this->scanPartition_CheckStart_gOverlaps(l, a, this->pOrgsAft_ioffsets, this->pOrgsAftTimestamps, CompareTimestampPairsByStart, q.end, result);
            }
            else if (foundzero) {
                this->scanPartition_CheckStart_gOverlaps(l, a, this->pOrgsIn_ioffsets, this->pOrgsInTimestamps, CompareTimestampPairsByStart, q.end, result);
                this->scanPartition_CheckStart_gOverlaps(l, a, this->pOrgsAft_ioffsets, this->pOrgsAftTimestamps, CompareTimestampPairsByStart, q.end, result);
            }
            else if (foundone) {
                this->scanPartition_CheckEnd_gOverlaps(l, a, this->pOrgsInCopy_ioffsets, this->pOrgsInCopyTimestamps, CompareTimestampPairsByEnd, q.start, result);
                this->scanPartition_NoChecks_gOverlaps(l, a, this->pOrgsAft_ioffsets, this->pOrgsAftIds, result);
            }

            if (!foundzero) {
                this->scanPartition_CheckEnd_gOverlaps(l, a, this->pRepsIn_ioffsets, this->pRepsInTimestamps, CompareTimestampPairsByEnd, q.start, result);
            }
            else {
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
        this->scanPartition_CheckEnd_gOverlaps(this->numBits, a, this->pOrgsIn_ioffsets, this->pOrgsInTimestamps, CompareTimestampPairsByEnd, q.start, result);
    }


    return result;
}

size_t HINT_IT_SubsSort_SS_CM_NoVLs::executeBottomUp_Stabbing(const RangeQuery& q, vector<hint_it_log>& candidate) {
    size_t result = 0;
    vector<pair<Timestamp, Timestamp> >::iterator iter, iterBegin, iterEnd;
    RelationIdIterator iterI, iterIStart, iterIEnd;
    Timestamp a = q.start >> (this->maxBits-this->numBits);
    bool foundzero = false;
    bool foundone = false;
    hint_it_log pOrgsIn_log, pOrgsInCopy_log, pOrgsAft_log, pRepsIn_log, pRepsAft_log, pOrgsIn_it_log;
    
    pOrgsIn_log.type  = LOG_ORIGINALS_IN;
    pOrgsInCopy_log.type = LOG_ORIGINALS_IN_COPY;
    pOrgsAft_log.type = LOG_ORIGINALS_AFT;
    pRepsIn_log.type  = LOG_REPLICAS_IN;
    pRepsAft_log.type = LOG_REPLICAS_AFT;
    pOrgsIn_it_log.type = LOG_ORIGINALS_IN_IT;

    for (auto l = 0; l < this->numBits; l++) {
        pOrgsIn_log.level = pOrgsInCopy_log.level = pOrgsAft_log.level = pRepsIn_log.level = pRepsAft_log.level = pOrgsIn_it_log.level = l;
        pOrgsIn_log.left_idx = pOrgsInCopy_log.left_idx = pOrgsAft_log.left_idx = pRepsIn_log.left_idx = pRepsAft_log.left_idx = pOrgsIn_it_log.left_idx = -1;
        
        if (foundone && foundzero) {
            this->scanPartitionNoChecks_gOverlaps(l, a, this->pOrgsIn_ioffsets, this->pOrgsInIds, pOrgsIn_log);
            this->scanPartitionNoChecks_gOverlaps(l, a, this->pOrgsAft_ioffsets, this->pOrgsAftIds, pOrgsAft_log);
            this->scanPartitionNoChecks_gOverlaps(l, a, this->pRepsIn_ioffsets, this->pRepsInIds, pRepsIn_log);
            this->scanPartitionNoChecks_gOverlaps(l, a, this->pRepsAft_ioffsets, this->pRepsAftIds, pRepsAft_log);
        }
        else {
            if (!foundzero && !foundone) {
                vector<hint_it_log> pOrgsIn_logs = this->scanPartitionCheckBothTimestamps_gOverlaps(l, a, this->pOrgsIn_ioffsets, this->pOrgsInTimestamps, CompareTimestampPairsByStart, q.start, q.end);
                for (auto& log : pOrgsIn_logs) {
                    candidate.push_back(log);
                    result += log.right_idx - log.left_idx;
                }
                this->scanPartitionCheckStart_gOverlaps(l, a, this->pOrgsAft_ioffsets, this->pOrgsAftTimestamps, CompareTimestampPairsByStart, q.end, pOrgsAft_log);
            }
            else if (foundzero) {
                this->scanPartitionCheckStart_gOverlaps(l, a, this->pOrgsIn_ioffsets, this->pOrgsInTimestamps, CompareTimestampPairsByStart, q.end, pOrgsIn_log);
                this->scanPartitionCheckStart_gOverlaps(l, a, this->pOrgsAft_ioffsets, this->pOrgsAftTimestamps, CompareTimestampPairsByStart, q.end, pOrgsAft_log);
            }
            else if (foundone) {
                this->scanPartitionCheckEnd_gOverlaps(l, a, this->pOrgsInCopy_ioffsets, this->pOrgsInCopyTimestamps, CompareTimestampPairsByEnd, q.start, pOrgsInCopy_log);
                this->scanPartitionNoChecks_gOverlaps(l, a, this->pOrgsAft_ioffsets, this->pOrgsAftIds, pOrgsAft_log);
            }

            if (!foundzero) {
                this->scanPartitionCheckEnd_gOverlaps(l, a, this->pRepsIn_ioffsets, this->pRepsInTimestamps, CompareTimestampPairsByEnd, q.start, pRepsIn_log);
            }
            else {
                this->scanPartitionNoChecks_gOverlaps(l, a, this->pRepsIn_ioffsets, this->pRepsInIds, pRepsIn_log);
            }
            this->scanPartitionNoChecks_gOverlaps(l, a, this->pRepsAft_ioffsets, this->pRepsAftIds, pRepsAft_log);

            if (a%2) 
                foundone = 1;
            if (!(a%2)) 
                foundzero = 1;
        }
        a >>= 1;
        
        if (pOrgsIn_log.left_idx != -1) {
            candidate.push_back(pOrgsIn_log);
            result += pOrgsIn_log.right_idx - pOrgsIn_log.left_idx;
        }
        if (pOrgsInCopy_log.left_idx != -1) {
            candidate.push_back(pOrgsInCopy_log);
            result += pOrgsInCopy_log.right_idx - pOrgsInCopy_log.left_idx;
        }
        if (pOrgsAft_log.left_idx != -1) {
            candidate.push_back(pOrgsAft_log);
            result += pOrgsAft_log.right_idx - pOrgsAft_log.left_idx;
        }
        if (pRepsIn_log.left_idx != -1) {
            candidate.push_back(pRepsIn_log);
            result += pRepsIn_log.right_idx - pRepsIn_log.left_idx;
        }
        if (pRepsAft_log.left_idx != -1) {
            candidate.push_back(pRepsAft_log);
            result += pRepsAft_log.right_idx - pRepsAft_log.left_idx;
        }
    }

    if (foundone && foundzero) {
        if (!this->pOrgsInIds[this->numBits].empty()) {
            pOrgsIn_log.level = this->numBits;
            pOrgsIn_log.left_idx = 0;
            pOrgsIn_log.right_idx = this->pOrgsInIds[this->numBits].size();
            candidate.push_back(pOrgsIn_log);
            result += pOrgsIn_log.right_idx - pOrgsIn_log.left_idx;
        }
    }
    else {
        pOrgsIn_log.level = this->numBits;
        pOrgsIn_log.type = LOG_ORIGINALS_IN;
        
        iterBegin = this->pOrgsInTimestamps[this->numBits].begin();
        iterEnd = lower_bound(iterBegin, this->pOrgsInTimestamps[this->numBits].end(), make_pair<Timestamp, Timestamp>(q.start+1, q.start+1), CompareTimestampPairsByStart);
        
        size_t validCount = 0;
        size_t startIdx = 0;
        
        for (iter = iterBegin; iter != iterEnd; iter++) {
            if (q.start <= iter->second) {
                if (validCount == 0)
                    startIdx = iter - this->pOrgsInTimestamps[this->numBits].begin();
                validCount++;
            }
        }
        
        if (validCount > 0) {
            pOrgsIn_log.left_idx = startIdx;
            pOrgsIn_log.right_idx = startIdx + validCount;
            candidate.push_back(pOrgsIn_log);
            result += validCount;
        }
    }
    
    return result;
}


// Independent Range Sampling
inline void HINT_IT_SubsSort_SS_CM_NoVLs::scanPartitionNoChecks_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, RelationId *ids, hint_it_log &_log)
{
//    return;
    RelationIdIterator iterIBegin, iterIEnd;

    if (this->getBounds(level, t, ioffsets, ids, iterIBegin, iterIEnd))
    {
        _log.left_idx = iterIBegin-ids[level].begin();
        _log.right_idx = _log.left_idx + (iterIEnd-iterIBegin);
    }
}

inline void HINT_IT_SubsSort_SS_CM_NoVLs::scanPartitionsNoChecks_gOverlaps(unsigned int level, Timestamp ts, Timestamp te, Offsets_SS_CM_NoVLs *ioffsets, RelationId *ids, hint_it_log &_log)
{
//    return;
    RelationIdIterator iterIBegin, iterIEnd;

    if (this->getBounds(level, ts, te, ioffsets, ids, iterIBegin, iterIEnd))
    {
        if (_log.left_idx == -1)
        {
            _log.left_idx = iterIBegin-ids[level].begin();
            _log.right_idx = _log.left_idx + (iterIEnd-iterIBegin);
        }
        else
            _log.right_idx += (iterIEnd-iterIBegin);
    }
}

inline void HINT_IT_SubsSort_SS_CM_NoVLs::scanPartitionCheckEnd_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, bool (*compare)(const pair<Timestamp, Timestamp>&, const pair<Timestamp, Timestamp>&), Timestamp qt, hint_it_log &_log)
{
    vector<pair<Timestamp, Timestamp> >::iterator pivot, iterBegin, iterEnd;
    RelationIdIterator iterI;

    if (this->getBounds(level, t, ioffsets, timestamps, iterBegin, iterEnd, iterI))
    {
        pivot = lower_bound(iterBegin, iterEnd, make_pair(qt, qt), compare);
     
        if (pivot != iterEnd)
        {
            _log.left_idx = pivot-timestamps[level].begin();
            _log.right_idx = _log.left_idx + (iterEnd-pivot);
        }
    }
}

inline void HINT_IT_SubsSort_SS_CM_NoVLs::scanPartitionCheckStart_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, bool (*compare)(const pair<Timestamp, Timestamp>&, const pair<Timestamp, Timestamp>&), Timestamp qt, hint_it_log &_log)
{
    vector<pair<Timestamp, Timestamp> >::iterator pivot, iterBegin, iterEnd;
    RelationIdIterator iterI;

    if (this->getBounds(level, t, ioffsets, timestamps, iterBegin, iterEnd, iterI))
    {
        pivot = lower_bound(iterBegin, iterEnd, make_pair(qt+1, qt+1), compare);
        
        if (pivot != iterBegin)
        {
            if (_log.left_idx == -1)
            {
                _log.left_idx = iterBegin-timestamps[level].begin();
                _log.right_idx = _log.left_idx + (pivot-iterBegin);
            }
            else
                _log.right_idx += (pivot-iterBegin);
        }
    }
}


inline vector<hint_it_log> HINT_IT_SubsSort_SS_CM_NoVLs::scanPartitionCheckBothTimestamps_gOverlaps(unsigned int level, Timestamp t, Offsets_SS_CM_NoVLs *ioffsets, vector<pair<Timestamp, Timestamp> > *timestamps, bool (*compare)(const pair<Timestamp, Timestamp>&, const pair<Timestamp, Timestamp>&), Timestamp qstart, Timestamp qend)
{
    vector<hint_it_log> logs;
    vector<pair<Timestamp, Timestamp> >::iterator iter, iterBegin, iterEnd;
    RelationIdIterator iterI;

    if (this->getBounds(level, t, ioffsets, timestamps, iterBegin, iterEnd, iterI)) {
        if (level == 0) {
            RangeQuery q(0, qstart, qend);
            vector<IntervalTree::IT_CandidateLog> candidates;
            this->pOrgsInTrees[t]->execute_Stabbing(q, candidates);
            if (!candidates.empty()) {
                for (const auto& candidate : candidates) {
                    hint_it_log log;
                    log.type = LOG_ORIGINALS_IN_IT;
                    log.node = candidate.node;
                    log.left_idx = candidate.left_idx;
                    log.right_idx = candidate.right_idx;
                    log.it_type = candidate.type;
                    log.level = level;
                    logs.push_back(log);
                }
            }
        }
        else {
            vector<pair<Timestamp, Timestamp> >::iterator pivot = lower_bound(iterBegin, iterEnd, make_pair(qend+1, qend+1), compare);        
            size_t validCount = 0, startIdx = 0;
            bool foundFirst = false;
            
            for (iter = iterBegin; iter != pivot; iter++) {
                if (qstart <= iter->second) {
                    if (!foundFirst) {
                        startIdx = iter - timestamps[level].begin();
                        foundFirst = true;
                    }
                    validCount++;
                }
            }
            
            if (validCount > 0) {
                hint_it_log log;
                log.left_idx = startIdx;
                log.right_idx = startIdx + validCount;
                logs.push_back(log);
            }
        }
    }
    return logs;
}

RecordId HINT_IT_SubsSort_SS_CM_NoVLs::get_random_sample(const hint_it_log &_log) {
    uniform_int_distribution<> rnd_idx(_log.left_idx, _log.right_idx);
    pcg32 _mt;

    const int arr_idx = rnd_idx(_mt);
    switch (_log.type)
    {
        case LOG_ORIGINALS_IN:
            return this->pOrgsInIds[_log.level][arr_idx];
        case LOG_ORIGINALS_IN_COPY:
            return this->pOrgsInCopyIds[_log.level][arr_idx];
        case LOG_ORIGINALS_AFT:
            return this->pOrgsAftIds[_log.level][arr_idx];
        case LOG_REPLICAS_IN:
            return this->pRepsInIds[_log.level][arr_idx];
        case LOG_REPLICAS_AFT:
            return this->pRepsAftIds[_log.level][arr_idx];
        case LOG_ORIGINALS_IN_IT: {
            if (_log.it_type == 0)
                return _log.node->recordsByStart[arr_idx].id;
            return _log.node->recordsByEnd[arr_idx].id;
        }
    }
}

void HINT_IT_SubsSort_SS_CM_NoVLs::executeBottomUp_gOverlaps(const RangeQuery &q, const unsigned int sample_size) {
    // Left empty on purpose
}
