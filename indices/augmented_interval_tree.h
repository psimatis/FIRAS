#ifndef _AUGMENTED_INTERVAL_TREE_H_
#define _AUGMENTED_INTERVAL_TREE_H_

#include "../def_global.h"
#include "../containers/relation.h"
#include <vector>
#include <cmath>
#include <unordered_map>
#include <functional>

#define RECORDS_BY_START 0
#define RECORDS_BY_END 1
#define AUGMENTED_LIST 2
#define LIVE_INDEX 3
#define LIVE_INDEX_LAST_BUFFER 4

class AugmentedIntervalTreeNode {
public:
    Timestamp center;
    unsigned int level;
    AugmentedIntervalTreeNode *left, *right;
    RelationStart recordsByStart;
    RelationEnd recordsByEnd;
    vector<pair<RecordId, Timestamp>> al;
    bool isLeft = false;
    
    AugmentedIntervalTreeNode();
    AugmentedIntervalTreeNode(unsigned int level, const Relation &R, bool isLeft = false);
    AugmentedIntervalTreeNode(Timestamp center, unsigned int level, bool isLeft);
    inline void scan_NoChecks_gOverlaps(size_t &result);
    inline void scan_CheckStart_gOverlaps(const RangeQuery &q, size_t &result);
    inline void scan_CheckEnd_gOverlaps(const RangeQuery &q, size_t &result);
    inline void scan_augmented_list_range(const RangeQuery &q, size_t &result);
    inline void scan_NoChecks_gOverlaps(vector<RecordId> &results);
    inline void scan_CheckStart_gOverlaps(const RangeQuery &q, vector<RecordId> &results);
    inline void scan_CheckEnd_gOverlaps(const RangeQuery &q, vector<RecordId> &results);
    inline void scan_augmented_list_range(const RangeQuery &q, vector<RecordId> &results);
};

class AugmentedIntervalTree {
public:
    AugmentedIntervalTreeNode *root;
    unsigned int numLevels;
    mutable size_t nodesAccessed = 0;
    size_t numNodes = 0;
    
    Relation insertionPool;
    size_t intervals = 0;
    Timestamp D_ait = 0;

    AugmentedIntervalTree(const Relation &R);
    AugmentedIntervalTree();

    void getStats();
    size_t getSize();
    size_t getHeight();
    void print();
    
    void insert(const Record& newRecord);

    struct AIT_CandidateLog {
        AugmentedIntervalTreeNode* node;
        int type;
        int left_idx;
        int right_idx;
        void* live_data;
        
        AIT_CandidateLog(): node(nullptr), type(0), left_idx(-1), right_idx(-1) {}
        AIT_CandidateLog(AugmentedIntervalTreeNode* n, int t, int l, int r): node(n), type(t), left_idx(l), right_idx(r) {}
        AIT_CandidateLog(void* n, int t, int l, int r): node(nullptr), type(t), left_idx(l), right_idx(r), live_data(n) {}
    };

    size_t execute_gOverlaps(const RangeQuery &q);
    void execute_gOverlaps(const RangeQuery &q, vector<RecordId> &results);
    void execute_gOverlaps(const RangeQuery &q, vector<AIT_CandidateLog> &candidates);
    size_t execute_Stabbing(const RangeQuery &q);

    void update_domain(Timestamp D);
    void insert_recursive(AugmentedIntervalTreeNode* node, const Record& record, Timestamp lo, Timestamp hi);
    void insert_recursive_with_path(AugmentedIntervalTreeNode* node, const Record& record, Timestamp lo, Timestamp hi, vector<AugmentedIntervalTreeNode*>& pathNodes);
    void insert_and_rebuild(AugmentedIntervalTreeNode* node, const Record& record, Timestamp lo, Timestamp hi, vector<AugmentedIntervalTreeNode*>& path);
    void rebuildAugmentedLists(AugmentedIntervalTreeNode* node);
};

#endif