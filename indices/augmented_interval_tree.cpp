#include "augmented_interval_tree.h"

AugmentedIntervalTreeNode::AugmentedIntervalTreeNode() {
    this->left = NULL;
    this->right = NULL;
}

AugmentedIntervalTreeNode::AugmentedIntervalTreeNode(Timestamp center, unsigned int level, bool isLeft) {
    this->center = center;
    this->level = level;
    this->left = NULL;
    this->right = NULL;
    this->isLeft = isLeft;
}

AugmentedIntervalTreeNode::AugmentedIntervalTreeNode(unsigned int level, const Relation &R, bool isLeft) {
    this->isLeft = isLeft;
    if (R.empty()) {
        this->left = NULL;
        this->right = NULL;
        this->center = 0;
        this->level = level;
        return;
    }

    RelationIterator iterBegin = R.begin();
    RelationIterator iterEnd = R.end();
    Relation recordsL, recordsR;

    this->level = level;
    
    const unsigned int size = R.size();
    vector<Timestamp> endpoints;
    
    endpoints.reserve(2 * size);
    for (auto iter = iterBegin; iter != iterEnd; iter++) {
        endpoints.push_back(iter->start);
        endpoints.push_back(iter->end);
    }
    sort(endpoints.begin(), endpoints.end());
    this->center = endpoints[size];
    
    for (auto iter = iterBegin; iter != iterEnd; iter++) {
        if (iter->end < this->center)
            recordsL.emplace_back(iter->id, iter->start, iter->end);
        else if (iter->start > this->center)
            recordsR.emplace_back(iter->id, iter->start, iter->end);
        else {
            this->recordsByStart.emplace_back(iter->id, iter->start);
            this->recordsByEnd.emplace_back(iter->id, iter->end);
        }
    }

    this->recordsByEnd.sortByEnd();

    if (!recordsL.empty())
        this->left = new AugmentedIntervalTreeNode(this->level + 1, recordsL, true);
    else
        this->left = NULL;
    
    if (!recordsR.empty())
        this->right = new AugmentedIntervalTreeNode(this->level + 1, recordsR, false);
    else
        this->right = NULL;

    if (this->level > 0) {
        if (this->isLeft) {
            for (auto iter = iterBegin; iter != iterEnd; iter++)
                this->al.emplace_back(iter->id, iter->end);
            sort(this->al.begin(), this->al.end(), [](const pair<RecordId, Timestamp>& a, const pair<RecordId, Timestamp>& b) {
                return a.second < b.second;
            });
        } else {
            for (auto iter = iterBegin; iter != iterEnd; iter++)
                this->al.emplace_back(iter->id, iter->start);
        }
    }
}

AugmentedIntervalTree::AugmentedIntervalTree(const Relation &R) {
    this->numLevels = 1;
    this->root = new AugmentedIntervalTreeNode(0, R, false);
}

AugmentedIntervalTree::AugmentedIntervalTree() {
    this->root = NULL;
    this->intervals = 0;
    this->numLevels = 0;
    this->numNodes = 0;
}

void AugmentedIntervalTree::update_domain(Timestamp D) {
    if (D <= this->D_ait) 
        return;
    Timestamp target = (D % 2 != 0) ? D : (D - 1);
    if (target < 1) 
        return;

    if (!this->root) {
        this->root = new AugmentedIntervalTreeNode(target, 0, false);
        this->numNodes = 1;
        this->D_ait = 2 * target - 1;
        return;
    }

    if (this->root->center >= target) {
        this->D_ait = 2 * target - 1;
        return;
    }
    
    AugmentedIntervalTreeNode* newRoot = new AugmentedIntervalTreeNode(target, 0, false);
    this->numNodes++;
    
    AugmentedIntervalTreeNode* oldRoot = this->root;
    newRoot->left = oldRoot;
    newRoot->left->isLeft = true;
 
    this->root = newRoot;
    this->D_ait = 2 * target - 1;
}

void AugmentedIntervalTree::insert(const Record &r) {
    AugmentedIntervalTreeNode* oldRoot = this->root;

    update_domain(r.end);
    if (!this->root) 
        return;

    vector<AugmentedIntervalTreeNode*> path;
    AugmentedIntervalTreeNode* curr = this->root;
    Timestamp lo = 1, hi = this->D_ait;

    auto mid_odd = [](Timestamp lo, Timestamp hi) -> Timestamp {
        size_t n = size_t((hi - lo) / 2) + 1;
        return lo + 2*(n/2);
    };

    while (curr) {
        path.push_back(curr);
        if (r.start <= curr->center && curr->center <= r.end) {
            auto it_s = lower_bound(curr->recordsByStart.begin(), curr->recordsByStart.end(), RecordStart(r.id, r.start));
            curr->recordsByStart.insert(it_s, RecordStart(r.id, r.start));

            auto it_e = lower_bound(curr->recordsByEnd.begin(), curr->recordsByEnd.end(), RecordEnd(r.id, r.end));
            curr->recordsByEnd.insert(it_e, RecordEnd(r.id, r.end));
            
            break; 
        }

        if (r.end < curr->center) {
            Timestamp nhi = curr->center - 2;
            if (lo > nhi) break;
            if (!curr->left) {
                Timestamp c = mid_odd(lo, nhi);
                curr->left = new AugmentedIntervalTreeNode(c, curr->level + 1, true);
                this->numNodes++;
            }
            curr = curr->left;
            hi = nhi;
        } else {
            Timestamp nlo = curr->center + 2;
            if (nlo > hi) break;
            if (!curr->right) {
                Timestamp c = mid_odd(nlo, hi);
                curr->right = new AugmentedIntervalTreeNode(c, curr->level + 1, false);
                this->numNodes++;
            }
            curr = curr->right;
            lo = nlo;
        }
    }

    // Update augmented lists for all ancestors
    for (auto* node : path) {
        if (node->level != 0) {
            Timestamp valueToInsert = node->isLeft ? r.end : r.start;

            auto it = lower_bound(node->al.begin(), node->al.end(), make_pair(r.id, valueToInsert),
                [](const pair<RecordId, Timestamp>& a, const pair<RecordId, Timestamp>& b) {
                    return a.second < b.second;
                });
            node->al.insert(it, make_pair(r.id, valueToInsert));
        }
    }

    if (this->root != oldRoot && oldRoot != NULL) {
        AugmentedIntervalTreeNode* demotedNode = this->root->left;

        function<void(AugmentedIntervalTreeNode*, vector<pair<RecordId, Timestamp>>&)> collectAll =
            [&](AugmentedIntervalTreeNode* node, vector<pair<RecordId, Timestamp>>& all_times) {
            if (!node) return;
            
            for (auto& rec : node->recordsByEnd)
                all_times.emplace_back(rec.id, rec.end);
            
            collectAll(node->left, all_times);
            collectAll(node->right, all_times);
        };
        
        collectAll(demotedNode, demotedNode->al);
        
        sort(demotedNode->al.begin(), demotedNode->al.end(), [](const pair<RecordId, Timestamp>& a, const pair<RecordId, Timestamp>& b) {
            return a.second < b.second;
        });
    }
}

void AugmentedIntervalTree::getStats() {
    vector<unsigned int> nodesPerLevel;
    vector<size_t> recordsPerLevel;
    stack<AugmentedIntervalTreeNode*> st;
    AugmentedIntervalTreeNode *n;
    
    if (this->root)
        st.push(this->root);
    
    while (!st.empty()) {
        n = st.top();
        st.pop();
        this->numNodes++;
        
        if (nodesPerLevel.size() <= n->level) {
            nodesPerLevel.resize(n->level + 1, 0);
            recordsPerLevel.resize(n->level + 1, 0);
        }
        
        nodesPerLevel[n->level]++;
        recordsPerLevel[n->level] += n->recordsByStart.size();
        
        if (n->left)
            st.push(n->left);
        if (n->right)
            st.push(n->right);
    }
    this->numLevels = nodesPerLevel.size();
}

size_t AugmentedIntervalTree::getHeight() {   
    function<size_t(AugmentedIntervalTreeNode*)> height = 
        [&](AugmentedIntervalTreeNode* node) -> size_t {
            if (!node) return 0;
            size_t hl = height(node->left);
            size_t hr = height(node->right);
            return 1 + max(hl, hr);
        };
    
    return height(this->root);
}

size_t AugmentedIntervalTree::getSize() {
    size_t size = 0;
    stack<AugmentedIntervalTreeNode*> st;
    AugmentedIntervalTreeNode *curr;
    
    st.push(this->root);
    while (!st.empty()) {
        curr = st.top();
        st.pop();
        
        size += sizeof(AugmentedIntervalTreeNode);
        size += curr->recordsByStart.size() * sizeof(RecordStart);
        size += curr->recordsByEnd.size() * sizeof(RecordEnd);
        
        if (curr->level > 0) 
            size += curr->al.size() * sizeof(pair<RecordId, Timestamp>);
        
        if (curr->left)
            st.push(curr->left);
        if (curr->right)
            st.push(curr->right);
    }
    return size;
}

void AugmentedIntervalTree::print() {
    // cout << "Augmented Interval Tree:" << endl;
    // cout << "Number of levels: " << numLevels << endl;
    // cout << "Number of nodes: " << numNodes << endl;
    // cout << "Domain: " << D_ait << endl;
    
    // function<void(AugmentedIntervalTreeNode*, int)> printNode = [&](AugmentedIntervalTreeNode* node, int depth) {
    //     if (!node) return;
        
    //     for (int i = 0; i < depth; i++) cout << "  ";
    //     cout << "Level " << node->level << ", Center: " << node->center << endl;
        
    //     for (int i = 0; i < depth; i++) cout << "  ";
    //     cout << "Records by start: ";
    //     for (auto& r : node->recordsByStart) {
    //         cout << "(" << r.id << "," << r.start << ") ";
    //     }
    //     cout << endl;
        
    //     for (int i = 0; i < depth; i++) cout << "  ";
    //     cout << "Records by end: ";
    //     for (auto& r : node->recordsByEnd) {
    //         cout << "(" << r.id << "," << r.end << ") ";
    //     }
    //     cout << endl;
        
    //     for (int i = 0; i < depth; i++) cout << "  ";
    //     cout << "ALl (" << node->ALl.size() << " intervals): ";
    //     for (auto& r : node->ALl) {
    //         cout << "(" << r.first << "," << r.second << ") ";
    //     }
    //     cout << endl;
        
    //     for (int i = 0; i < depth; i++) cout << "  ";
    //     cout << "ALr (" << node->ALr.size() << " intervals): ";
    //     for (auto& r : node->ALr) {
    //         cout << "(" << r.first << "," << r.second << ") ";
    //     }
    //     cout << endl << endl;
        
    //     printNode(node->left, depth + 1);
    //     printNode(node->right, depth + 1);
    // };
    
    // printNode(root, 0);
}

inline void AugmentedIntervalTreeNode::scan_CheckStart_gOverlaps(const RangeQuery &q, size_t &result) {
    RelationStartIterator pivot = upper_bound(this->recordsByStart.begin(), this->recordsByStart.end(), RecordStart(0, q.end+1));
    
    for (auto iter = this->recordsByStart.begin(); iter != pivot; iter++) {
#ifdef WORKLOAD_COUNT
        result++;
#else
        result ^= iter->id;
#endif
    }
}

inline void AugmentedIntervalTreeNode::scan_CheckEnd_gOverlaps(const RangeQuery &q, size_t &result) {
    RelationEndIterator pivot = lower_bound(this->recordsByEnd.begin(), this->recordsByEnd.end(), RecordEnd(0, q.start));
    
    for (auto iter = pivot; iter != this->recordsByEnd.end(); iter++) {
#ifdef WORKLOAD_COUNT
        result++;
#else
        result ^= iter->id;
#endif
    }
}

inline void AugmentedIntervalTreeNode::scan_NoChecks_gOverlaps(size_t &result) {
    for (auto iter = this->recordsByStart.begin(); iter != this->recordsByStart.end(); iter++)
#ifdef WORKLOAD_COUNT
        result ++;
#else
        result ^= iter->id;
#endif
}

inline void AugmentedIntervalTreeNode::scan_augmented_list_range(const RangeQuery &q, size_t &result) {
    if (this->isLeft) {
        auto lb = lower_bound(this->al.begin(), this->al.end(), make_pair(0, q.start), 
            [](const pair<RecordId, Timestamp>& rec, const pair<RecordId, Timestamp>& val) { 
                return rec.second < val.second; 
            });

        for (auto it = lb; it != this->al.end(); ++it) {
#ifdef WORKLOAD_COUNT
            result++;
#else
            result ^= it->first;
#endif
        }
    }
    else {
        auto ub = upper_bound(this->al.begin(), this->al.end(), make_pair(0, q.end), 
            [](const pair<RecordId, Timestamp>& val, const pair<RecordId, Timestamp>& rec) { 
                return val.second < rec.second; 
            });

        for (auto it = this->al.begin(); it != ub; ++it) {
#ifdef WORKLOAD_COUNT
            result++;
#else
            result ^= it->first;
#endif
        }
    }
}

inline void AugmentedIntervalTreeNode::scan_NoChecks_gOverlaps(vector<RecordId> &results) {
    for (auto iter = this->recordsByStart.begin(); iter != this->recordsByStart.end(); iter++)
        results.push_back(iter->id);
}

inline void AugmentedIntervalTreeNode::scan_CheckStart_gOverlaps(const RangeQuery &q, vector<RecordId> &results) {
    RelationStartIterator iterBegin = this->recordsByStart.begin();
    RelationStartIterator iterEnd = this->recordsByStart.end();
    RelationStartIterator pivot = upper_bound(iterBegin, iterEnd, RecordStart(0, q.end+1));
    
    for (auto iter = iterBegin; iter != pivot; iter++)
        results.push_back(iter->id);
}

inline void AugmentedIntervalTreeNode::scan_CheckEnd_gOverlaps(const RangeQuery &q, vector<RecordId> &results) {
    RelationEndIterator iterBegin = this->recordsByEnd.begin();
    RelationEndIterator iterEnd = this->recordsByEnd.end();
    RelationEndIterator pivot = lower_bound(iterBegin, iterEnd, RecordEnd(0, q.start));
    
    for (auto iter = pivot; iter != iterEnd; iter++)
        results.push_back(iter->id);
}

inline void AugmentedIntervalTreeNode::scan_augmented_list_range(const RangeQuery &q, vector<RecordId> &results) {
    if (this->isLeft) {
        auto lb = lower_bound(this->al.begin(), this->al.end(), make_pair(0, q.start), 
            [](const pair<RecordId, Timestamp>& rec, const pair<RecordId, Timestamp>& val) { 
                return rec.second < val.second; 
            });

        for (auto it = lb; it != this->al.end(); ++it) {
            results.push_back(it->first);
        }
    }
    else {
        auto ub = upper_bound(this->al.begin(), this->al.end(), make_pair(0, q.end), 
            [](const pair<RecordId, Timestamp>& val, const pair<RecordId, Timestamp>& rec) { 
                return val.second < rec.second; 
            });

        for (auto it = this->al.begin(); it != ub; ++it) {
            results.push_back(it->first);
        }
    }
}

size_t AugmentedIntervalTree::execute_gOverlaps(const RangeQuery &q) {
    size_t result = 0;
    
    if (this->root) {
        stack<AugmentedIntervalTreeNode*> S;
        S.push(this->root);

        while (!S.empty()) {
            AugmentedIntervalTreeNode* curr = S.top();
            S.pop();
            this->nodesAccessed++;
            
            if (q.end < curr->center) {
                curr->scan_CheckStart_gOverlaps(q, result);
                if (curr->left)
                    S.push(curr->left);
            }
            else if (curr->center < q.start) {
                curr->scan_CheckEnd_gOverlaps(q, result);
                if (curr->right)
                    S.push(curr->right);
            }
            else {
                curr->scan_NoChecks_gOverlaps(result);            
                if (curr->left) 
                    curr->left->scan_augmented_list_range(q, result);
                if (curr->right) 
                    curr->right->scan_augmented_list_range(q, result);
                break;
            }
        }
    }   
    return result;
}

void AugmentedIntervalTree::execute_gOverlaps(const RangeQuery &q, vector<RecordId> &results) {
    if (this->root) {
        stack<AugmentedIntervalTreeNode*> S;
        S.push(this->root);

        while (!S.empty()) {
            AugmentedIntervalTreeNode* curr = S.top();
            S.pop();
            this->nodesAccessed++;
            
            if (q.end < curr->center) {
                curr->scan_CheckStart_gOverlaps(q, results);
                if (curr->left)
                    S.push(curr->left);
            }
            else if (curr->center < q.start) {
                curr->scan_CheckEnd_gOverlaps(q, results);
                if (curr->right)
                    S.push(curr->right);
            }
            else {
                curr->scan_NoChecks_gOverlaps(results);            
                if (curr->left) 
                    curr->left->scan_augmented_list_range(q, results);
                if (curr->right) 
                    curr->right->scan_augmented_list_range(q, results);
                break;
            }
        }
    }   
}

void AugmentedIntervalTree::execute_gOverlaps(const RangeQuery &q, vector<AIT_CandidateLog> &candidates) {
    stack<AugmentedIntervalTreeNode*> S;
    AugmentedIntervalTreeNode* curr;

    S.push(this->root);
    while (!S.empty()) {
        curr = S.top();
        S.pop();
        this->nodesAccessed++;
        
        // Case 1: q.r < ci (query is left of center)
        if (q.end < curr->center) {
            auto pivot = upper_bound(curr->recordsByStart.cbegin(), curr->recordsByStart.cend(), RecordStart(0, q.end + 1));
            int count = pivot - curr->recordsByStart.cbegin();
            if (count > 0)
                candidates.emplace_back(AugmentedIntervalTree::AIT_CandidateLog(curr, RECORDS_BY_START, 0, count));
            
            if (curr->left)
                S.push(curr->left);
        }
        // Case 2: ci < q.l (query is right of center)
        else if (curr->center < q.start) {
            auto pivot = lower_bound(curr->recordsByEnd.cbegin(), curr->recordsByEnd.cend(), RecordEnd(0, q.start));
            int l = pivot - curr->recordsByEnd.cbegin();
            int n = curr->recordsByEnd.size();
            if (l < n)
                candidates.emplace_back(AugmentedIntervalTree::AIT_CandidateLog(curr, RECORDS_BY_END, l, n));
            
            if (curr->right)
                S.push(curr->right);
        }
        // Case 3: q.l <= ci <= q.r (query includes center)
        else {
            if (curr->recordsByStart.size() > 0)
                candidates.emplace_back(AugmentedIntervalTree::AIT_CandidateLog(curr, RECORDS_BY_START, 0, curr->recordsByStart.size()));
            
            if (curr->left) {
                auto pivotL = lower_bound(curr->left->al.cbegin(), curr->left->al.cend(), make_pair(0, q.start), 
                    [](const pair<RecordId, Timestamp>& rec, const pair<RecordId, Timestamp>& val) { 
                        return rec.second < val.second; 
                    });
                int l = pivotL - curr->left->al.cbegin();
                int n = curr->left->al.size();
                if (l < n)
                    candidates.emplace_back(AugmentedIntervalTree::AIT_CandidateLog(curr->left, AUGMENTED_LIST, l, n));
            }
            
            if (curr->right) {
                auto pivotR = upper_bound(curr->right->al.cbegin(), curr->right->al.cend(), make_pair(0, q.end), 
                [](const pair<RecordId, Timestamp>& val, const pair<RecordId, Timestamp>& rec) { 
                    return val.second < rec.second; 
                });
                int count = pivotR - curr->right->al.cbegin();
                if (count > 0)
                    candidates.emplace_back(AugmentedIntervalTree::AIT_CandidateLog(curr->right, AUGMENTED_LIST, 0, count));
            }
            break;
        }
    }
}
