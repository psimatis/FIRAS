#ifndef _INTERVAL_TREE_H_
#define _INTERVAL_TREE_H_

#define IT_RECORDS_BY_START 0
#define IT_RECORDS_BY_END   1
#define IT_SUBTREE          3
#define IT_SUBTREE_LEFT     4
#define IT_SUBTREE_RIGHT    5
#define IT_FIRAS_LOG        2

#include "../def_global.h"
#include "../containers/relation.h"
#include <vector>


class IntervalTreeNode
{
public:
    Timestamp center;
    unsigned int level;
    IntervalTreeNode *left, *right;
    RelationStart recordsByStart;
    RelationEnd recordsByEnd;
//    bool isLeaf;
    
    // Construction
    IntervalTreeNode();
    IntervalTreeNode(const unsigned int level, const Relation &R);
    IntervalTreeNode(const unsigned int level, RelationIterator iterBegin, RelationIterator iterEnd);
//    IntervalTreeNode(const unsigned int level, const Relation &R, const unsigned int maxLevel);
//    IntervalTreeNode(const unsigned int level, RelationIterator iterBegin, RelationIterator iterEnd, const unsigned int maxLevel);
    void print(const char c);
    size_t getSize();
    ~IntervalTreeNode();
    
    // Updating
    void insert(const Record &r);
    void remove(const Record &r);

    // Querying
    inline void scan_NoChecks_gOverlaps(size_t &result);
    inline void scan_CheckStart_gOverlaps(const RangeQuery &q, size_t &result);
    inline void scan_CheckEnd_gOverlaps(const RangeQuery &q, size_t &result);
    
    inline void scan_NoChecks_gOverlaps(vector<RecordId> &results);
    inline void scan_CheckStart_gOverlaps(const RangeQuery &q, vector<RecordId> &results);
    inline void scan_CheckEnd_gOverlaps(const RangeQuery &q, vector<RecordId> &results);
};


//class IntervalTreeLeafNode : public IntervalTreeNode
//{
//public:
//#ifdef SPLIT_LEAF_SUBTREE
//    Relation recordsLeftSubtree;
//    Relation recordsRightSubtree;
//#else
//    Relation recordsSubtree;
//#endif
//    
//    // Construction
//    IntervalTreeLeafNode(const unsigned int level, const Relation &R);
//    IntervalTreeLeafNode(const unsigned int level, RelationIterator iterBegin, RelationIterator iterEnd);
//    void print(const char c);
//    size_t getSize();
//    ~IntervalTreeLeafNode();
//    
//    // Updating
//    void insert(const Record &r) {};
//    void remove(const Record &r) {};
//
//    // Querying
//#ifdef SPLIT_LEAF_SUBTREE
//    inline void scanLeftSubtree_CheckBothTimestamps_gOverlaps(const RangeQuery &q, size_t &result);
//    inline void scanLeftSubtree_CheckEnd_gOverlaps(const RangeQuery &q, size_t &result);
//    inline void scanRightSubtree_CheckBothTimestamps_gOverlaps(const RangeQuery &q, size_t &result);
//    inline void scanRightSubtree_CheckStart_gOverlaps(const RangeQuery &q, size_t &result);
//#else
//    inline void scanSubtree_CheckBothTimestamps_gOverlaps(const RangeQuery &q, size_t &result);
//#endif
//};


class IntervalTree
{
private:
    IntervalTreeNode *root;
    
public:
    unsigned int numLevels;
    size_t nodesAccessed = 0;
    size_t numNodes = 0;
    
    // Construction
    IntervalTree(const Relation &R);
    IntervalTree(Relation::iterator begin, Relation::iterator end);
//    IntervalTree(const Relation &R, const unsigned int maxLevel);
    void print(const char c);
    void getStats();
    size_t getSize();
    ~IntervalTree();
    
    // Updating
    void insert(const Record &r);
    void remove(const Record &r);
    
    // Querying
    size_t execute_Stabbing(const RangeQuery &q);
    size_t execute_gOverlaps(const RangeQuery &q);
    void execute_Stabbing(const RangeQuery &q, vector<RecordId> &results);
    void execute_gOverlaps(const RangeQuery &q, vector<RecordId> &results);
    
    struct IT_CandidateLog {
    IntervalTreeNode* node;
    int type;
    int left_idx;
    int right_idx;

    IT_CandidateLog(): node(nullptr), type(0), left_idx(-1), right_idx(-1) {}
    IT_CandidateLog(IntervalTreeNode* n, int t, int l, int r): node(n), type(t), left_idx(l), right_idx(r) {}
    };

    // Sampling
    void execute_Stabbing(const RangeQuery &q, vector<IT_CandidateLog> &candidates);
    void execute_gOverlaps(const RangeQuery &q, vector<IT_CandidateLog> &candidates);
};

#endif //_INTERVAL_TREE_H_
