#include "dit.h"
#include <algorithm>
#include <vector>
#include <iostream>
#include <iterator>
#include <stack>
#include <functional>

DITNode::DITNode(Timestamp center, unsigned int level) {
    this->center = center;
    this->level = level;
    this->left = NULL;
    this->right = NULL;
}

DITNode::~DITNode() {
    mergeBufferIfNeeded();

    if (this->left)
        delete this->left;
    
    if (this->right)
        delete this->right;
}


void DITNode::mergeBufferIfNeeded() {
    if (bufferByStart.empty())
        return;

    sort(bufferByStart.begin(), bufferByStart.end(), RecordStartCompare());

    size_t originalSize = mainSortedByStart.size();
    mainSortedByStart.insert(mainSortedByStart.end(), bufferByStart.begin(), bufferByStart.end());
    
    inplace_merge(mainSortedByStart.begin(), mainSortedByStart.begin() + originalSize, mainSortedByStart.end(), RecordStartCompare());

    bufferByStart.clear();
}

void DITNode::insert(const Record &r, size_t bufferThreshold) {
    this->recordsByEnd.insert(lower_bound(this->recordsByEnd.begin(), this->recordsByEnd.end(), RecordEnd(r.id, r.end)), RecordEnd(r.id, r.end));

    this->bufferByStart.emplace_back(r.id, r.start);
    if (this->bufferByStart.size() >= bufferThreshold)
        mergeBufferIfNeeded();
}

void DITNode::print(const char c) {
    for (unsigned int i = 0; i < this->level; ++i)
        cout << "\t";
    cout << "L" << this->level << " C=" << this->center 
         << " (S:" << this->mainSortedByStart.size() 
         << ", B:" << this->bufferByStart.size() << " recs)" << endl;

    if (this->left)
        this->left->print(c);

    if (this->right)
        this->right->print(c);
}

DIT::DIT(size_t threshold) {
    this->root = nullptr;
    this->D_dit = 0;
    this->bufferThreshold = threshold;
    this->numLevels = 0;
    this->numNodes = 0;
    this->nodesAccessed = 0;
    this->numEmptyNodes = 0;
    this->avgDataPerNode = 0;
}

DIT::~DIT() {
    delete this->root;
}

void DIT::update_domain(Timestamp D) {
    if (D <= this->D_dit) 
        return;
    Timestamp target = (D % 2 != 0) ? D : (D - 1);
    if (target < 1) 
        return;

    if (!this->root) {
        this->root = new DITNode(target, 0);
        this->numNodes = 1;
        this->D_dit = 2 * target - 1;
        return;
    }
    if (this->root->center >= target) {
        this->D_dit = 2 * target - 1;
        return;
    }
    DITNode* newRoot = new DITNode(target, 0);
    this->numNodes++;
    newRoot->left = this->root;
    this->root = newRoot;
    this->D_dit = 2 * target - 1;
}

void DIT::insert(const Record &r) {
    update_domain(r.end);
    if (!this->root) 
        return;

    auto mid_odd = [](Timestamp lo, Timestamp hi) -> Timestamp {
        size_t n = size_t((hi - lo) / 2) + 1;
        return lo + 2*(n/2);
    };

    function<void(DITNode*, Timestamp, Timestamp, unsigned)> insert_rec =
        [&](DITNode* node, Timestamp lo, Timestamp hi, unsigned level) {
            if (!node) 
                return;
            if (r.start <= node->center && node->center <= r.end) {
                node->insert(r, this->bufferThreshold);
                return;
            }
            if (r.end < node->center) {
                Timestamp nhi = node->center - 2;
                if (lo > nhi) 
                    return;
                if (!node->left) {
                    Timestamp c = mid_odd(lo, nhi);
                    node->left = new DITNode(c, level + 1);
                    this->numNodes++;
                }
                insert_rec(node->left, lo, nhi, level + 1);
            } else {
                Timestamp nlo = node->center + 2;
                if (nlo > hi) 
                    return;
                if (!node->right) {
                    Timestamp c = mid_odd(nlo, hi);
                    node->right = new DITNode(c, level + 1);
                    this->numNodes++;
                }
                insert_rec(node->right, nlo, hi, level + 1);
            }
        };

    insert_rec(this->root, 1, this->D_dit, 0);
}

void DIT::print(const char c) {
    this->root->print(c);
}

void DIT::getStats() {   
    size_t tempNumNodes = 0;
    unsigned int maxLevel = 0;
    size_t totalDataSize = 0;
    size_t emptyNodes = 0;
    stack<DITNode*> S;
    S.push(this->root);
    
    while (!S.empty()) {
        DITNode *curr = S.top();
        S.pop();
        if (!curr) continue;
        tempNumNodes++;
        if (curr->level > maxLevel)
            maxLevel = curr->level;

        size_t nodeDataSize = curr->mainSortedByStart.size() + curr->bufferByStart.size() + curr->recordsByEnd.size();
        if (nodeDataSize == 0)
            emptyNodes++;

        totalDataSize += nodeDataSize;
            
        if (curr->left) S.push(curr->left);
        if (curr->right) S.push(curr->right);
    }
    this->numNodes = tempNumNodes;
    this->numLevels = maxLevel + 1;
    this->numEmptyNodes = emptyNodes;
    if (this->numNodes > 0)
        this->avgDataPerNode = (double)totalDataSize / this->numNodes;
    else
        this->avgDataPerNode = 0;
}

size_t DIT::getSize() {
    size_t result = 0;
    stack<DITNode*> S;
    DITNode *curr;

    S.push(this->root);
    while (!S.empty()) {
        curr = S.top();
        S.pop();
        if (!curr) continue; 
        
        if (!curr->recordsByEnd.empty() || !curr->bufferByStart.empty() || !curr->mainSortedByStart.empty()) {
            result += sizeof(DITNode);
            result += curr->mainSortedByStart.size() * sizeof(RecordStart);
            result += curr->bufferByStart.size() * sizeof(RecordStart);
            result += curr->recordsByEnd.size() * sizeof(RecordEnd);
        }
        
        if (curr->left) 
            S.push(curr->left);
        if (curr->right) 
            S.push(curr->right);
    }
    return result;
}


size_t DIT::execute_Stabbing(const RangeQuery &q){
    size_t result = 0;
    stack<DITNode*> S;
    DITNode* curr;

    S.push(this->root);
    while (!S.empty()) {
        curr = S.top();
        S.pop();
        this->nodesAccessed++;
        
        if (q.start == curr->center) {
           curr->scan_NoChecks_gOverlaps(result);
                
           if (curr->right)
               S.push(curr->right);
        }
        else if (q.start < curr->center) {
           curr->scan_CheckStart_gOverlaps(q, result);

           if (curr->left)
               S.push(curr->left);
        }
        else {
           curr->scan_CheckEnd_gOverlaps(q, result);

           if (curr->right)
               S.push(curr->right);
        }
    }
    return result;
}

void DIT::execute_Stabbing(const RangeQuery &q, vector<RecordId> &results) {
    stack<DITNode*> S;
    DITNode* curr;

    S.push(this->root);
    while (!S.empty()) {
        curr = S.top();
        S.pop();
        this->nodesAccessed++;
        
        if (q.start == curr->center) {
           curr->scan_NoChecks_gOverlaps(results);
                
           if (curr->right)
               S.push(curr->right);
        }
        else if (q.start < curr->center) {
           curr->scan_CheckStart_gOverlaps(q, results);

           if (curr->left)
               S.push(curr->left);
        }
        else {
           curr->scan_CheckEnd_gOverlaps(q, results);

           if (curr->right)
               S.push(curr->right);
        }
    }
}

void DIT::execute_Stabbing(const RangeQuery &q, vector<DIT_CandidateLog> &candidates) {
    stack<DITNode*> S;
    DITNode* curr;

    S.push(this->root);
    while (!S.empty()) {
        curr = S.top();
        S.pop();
        this->nodesAccessed++;

        if (q.start == curr->center) {
            candidates.emplace_back(curr, DIT_RECORDS_BY_END, 0, curr->recordsByEnd.size());

            if (curr->right)
                S.push(curr->right);
        }
        else if (q.start < curr->center) {
            // 1. Get candidates from the main sorted vector.
            auto pivot = upper_bound(curr->mainSortedByStart.begin(), curr->mainSortedByStart.end(), RecordStart(0, q.start + 1), RecordStartCompare());
            int count = distance(curr->mainSortedByStart.begin(), pivot);
            if (count > 0)
                candidates.emplace_back(curr, DIT_MAIN_SORTED_BY_START, 0, count);

            // 2. Get candidates from the buffer.
            vector<RecordId>* buffer_candidates = new vector<RecordId>();
            for (auto &record : curr->bufferByStart) {
                if (record.start <= q.end) 
                    buffer_candidates->push_back(record.id);
            }
            if (!buffer_candidates->empty()) 
                candidates.emplace_back(buffer_candidates, DIT_BUFFER_BY_START, 0, buffer_candidates->size());

            if (curr->left)
                S.push(curr->left);
        }
        else {
            auto begin = curr->recordsByEnd.cbegin();
            int l = lower_bound(begin, curr->recordsByEnd.cend(), RecordEnd(0, q.start)) - begin;
            int n = curr->recordsByEnd.size();
            if (l < n)
                candidates.emplace_back(curr, DIT_RECORDS_BY_END, l, n);

            if (curr->right)
                S.push(curr->right);
        }
    }
}

inline void DITNode::scan_NoChecks_gOverlaps(size_t &result) {
    for (const auto& record : this->recordsByEnd) {
#ifdef WORKLOAD_COUNT
        result++;
#else
        result ^= record.id;
#endif
    }
}

inline void DITNode::scan_CheckStart_gOverlaps(const RangeQuery &q, size_t &result) {
    // 1. Query the main sorted vector using binary search.
    auto pivot = upper_bound(this->mainSortedByStart.begin(), this->mainSortedByStart.end(), RecordStart(0, q.end + 1), RecordStartCompare());
    for (auto iter = this->mainSortedByStart.begin(); iter != pivot; ++iter) {
#ifdef WORKLOAD_COUNT
        result++;
#else
        result ^= iter->id;
#endif
    }

    // 2. Query the buffer using a linear scan.
    for (const auto& record : this->bufferByStart) {
        if (record.start <= q.end) {
#ifdef WORKLOAD_COUNT
            result++;
#else
            result ^= record.id;
#endif
        }
    }
}

inline void DITNode::scan_CheckEnd_gOverlaps(const RangeQuery &q, size_t &result) {
    RelationEndIterator iterBegin = this->recordsByEnd.begin();
    RelationEndIterator iterEnd = this->recordsByEnd.end();
    
    for (auto iter = lower_bound(iterBegin, iterEnd, RecordEnd(0, q.start)); iter != iterEnd; iter++) {
#ifdef WORKLOAD_COUNT
        result++;
#else
        result ^= iter->id;
#endif
    }
}

inline void DITNode::scan_NoChecks_gOverlaps(vector<RecordId> &results) {
    for (const auto& record : this->recordsByEnd) {
        results.push_back(record.id);
    }
}

inline void DITNode::scan_CheckStart_gOverlaps(const RangeQuery &q, vector<RecordId> &results) {
    // 1. Query the main sorted vector using binary search.
    auto pivot = upper_bound(this->mainSortedByStart.begin(), this->mainSortedByStart.end(), RecordStart(0, q.end + 1), RecordStartCompare());
    for (auto iter = this->mainSortedByStart.begin(); iter != pivot; ++iter) {
        results.push_back(iter->id);
    }

    // 2. Query the buffer using a linear scan.
    for (const auto& record : this->bufferByStart) {
        if (record.start <= q.end) {
            results.push_back(record.id);
        }
    }
}

inline void DITNode::scan_CheckEnd_gOverlaps(const RangeQuery &q, vector<RecordId> &results) {
    RelationEndIterator iterBegin = this->recordsByEnd.begin();
    RelationEndIterator iterEnd = this->recordsByEnd.end();
    
    for (auto iter = lower_bound(iterBegin, iterEnd, RecordEnd(0, q.start)); iter != iterEnd; iter++) {
        results.push_back(iter->id);
    }
}
