#ifndef _DIT_H_
#define _DIT_H_

#include "../def_global.h"
#include "../containers/relation.h"
#include <vector>
#include <stack>
#include <set>

#define DIT_BUFFER_BY_START -1
#define DIT_MAIN_SORTED_BY_START 0
#define DIT_RECORDS_BY_END 1
#define DIT_FIRAS_LOG 2
#define LIVE_INDEX 3
#define LIVE_INDEX_LAST_BUFFER 4

struct RecordStartCompare {
    bool operator()(const RecordStart& a, const RecordStart& b) const {
        if (a.start != b.start) {
            return a.start < b.start;
        }
        return a.id < b.id;
    }
};

class DITNode {
public:
    Timestamp center;
    unsigned int level;
    DITNode *left, *right;
    vector<RecordStart> bufferByStart;
    vector<RecordStart> mainSortedByStart;
    RelationEnd recordsByEnd;
    
    DITNode(Timestamp center, unsigned int level = 0);
    void print(const char c);
    ~DITNode();
    
    void insert(const Record &r, size_t bufferThreshold);
    void mergeBufferIfNeeded();
    
    void scan_NoChecks_gOverlaps(size_t &result);
    void scan_CheckStart_gOverlaps(const RangeQuery &q, size_t &result);
    void scan_CheckEnd_gOverlaps(const RangeQuery &q, size_t &result);
    
    void scan_NoChecks_gOverlaps(vector<RecordId> &results);
    void scan_CheckStart_gOverlaps(const RangeQuery &q, vector<RecordId> &results);
    void scan_CheckEnd_gOverlaps(const RangeQuery &q, vector<RecordId> &results);
};

class DIT {
public:
    DITNode *root;
    Timestamp D_dit;
    size_t bufferThreshold;

    unsigned int numLevels;
    size_t nodesAccessed;
    size_t numNodes;
    size_t numEmptyNodes;
    double avgDataPerNode;
    
    DIT(size_t threshold = 128);
    void print(const char c);
    void getStats();
    size_t getSize();
    ~DIT();
    
    void update_domain(Timestamp D);
    void insert(const Record &r);
    
    size_t execute_Stabbing(const RangeQuery &q);
    void execute_Stabbing(const RangeQuery &q, vector<RecordId> &results);
    size_t execute_gOverlaps(const RangeQuery &q);

    struct DIT_CandidateLog {
        DITNode* dit_node;
        void* live_data;
        int type;
        int left_idx;
        int right_idx;
    
        DIT_CandidateLog(): dit_node(nullptr), live_data(nullptr), type(0), left_idx(-1), right_idx(-1) {}
        DIT_CandidateLog(int t): dit_node(nullptr), live_data(nullptr), type(t), left_idx(-1), right_idx(-1) {}
        DIT_CandidateLog(DITNode* n, int t, int l, int r): dit_node(n), live_data(nullptr), type(t), left_idx(l), right_idx(r) {}
        DIT_CandidateLog(void* n, int t, int l, int r): dit_node(nullptr), live_data(n), type(t), left_idx(l), right_idx(r) {}
    };
    
    void execute_Stabbing(const RangeQuery &q, vector<DIT_CandidateLog> &candidates);
    void execute_gOverlaps(const RangeQuery &q, vector<DIT_CandidateLog> &candidates);
};

#endif