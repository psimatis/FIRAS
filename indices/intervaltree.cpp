#include "intervaltree.h"
#include "../utils/weighted_sampling.hpp"
 
// Construction
IntervalTreeNode::IntervalTreeNode()
{
    this->left = NULL;
    this->right = NULL;
}
 
 
IntervalTreeNode::IntervalTreeNode(const unsigned int level, const Relation &R)
{
    RelationIterator iterBegin = R.begin();
    RelationIterator iterEnd = R.end();
    Relation recordsL;

    // Set the level of the node.
    this->level = level;
//    this->isLeaf = false;
    
    // Determine center
    Timestamp gstart = min_element(iterBegin, iterEnd)->start;
    Timestamp gend = max_element(iterBegin, iterEnd, CompareRecordsByEnd)->end; // TODO by end
    this->center = gstart+ceil((gend-gstart) / 2.0);
    
    // Determine the last record in the left subtree
    RelationIterator pivot = lower_bound(iterBegin, iterEnd, Record(0, center, center));

    for (auto iter = iterBegin; iter != pivot; iter++)
    {
        if (iter->end >= this->center)
        {
            this->recordsByStart.emplace_back(iter->id, iter->start);
            this->recordsByEnd.emplace_back(iter->id, iter->end);
        }
        else
            recordsL.emplace_back(iter->id, iter->start, iter->end);
    }

    // Relation recordsByStart is by construction sorted by start, hence sort recordsByEnd
    this->recordsByEnd.sortByEnd();

    // Create subtrees
    if (!recordsL.empty())
        this->left = new IntervalTreeNode(this->level+1, recordsL);
    else
        this->left = NULL;
    
    if (pivot != iterEnd)
        this->right = new IntervalTreeNode(this->level+1, pivot, iterEnd);
    else
        this->right = NULL;
}


IntervalTreeNode::IntervalTreeNode(const unsigned int level, RelationIterator iterBegin, RelationIterator iterEnd)
{
    Relation recordsL;

    // Set the level of the node.
    this->level = level;
//    this->isLeaf = false;
    
    // Determine center
    Timestamp gstart = min_element(iterBegin, iterEnd)->start;
    Timestamp gend = max_element(iterBegin, iterEnd, CompareRecordsByEnd)->end;
    this->center = gstart+ceil((gend-gstart) / 2.0);
    
    // Determine the last record in the left subtree
    RelationIterator pivot = lower_bound(iterBegin, iterEnd, Record(0, center, center));

    for (auto iter = iterBegin; iter != pivot; iter++)
    {
        if (iter->end >= this->center)
        {
            this->recordsByStart.emplace_back(iter->id, iter->start);
            this->recordsByEnd.emplace_back(iter->id, iter->end);
        }
        else
            recordsL.emplace_back(iter->id, iter->start, iter->end);
    }

    // Relation recordsByStart is by construction sorted by start, hence sort recordsByEnd
    this->recordsByEnd.sortByEnd();

    // Create subtrees
    if (!recordsL.empty())
        this->left = new IntervalTreeNode(this->level+1, recordsL.begin(), recordsL.end());
    else
        this->left = NULL;
    
    if (pivot != iterEnd)
        this->right = new IntervalTreeNode(this->level+1, pivot, iterEnd);
    else
        this->right = NULL;
}


//IntervalTreeNode::IntervalTreeNode(const unsigned int level, const Relation &R, const unsigned int maxLevel)
//{
//    RelationIterator iterBegin = R.begin();
//    RelationIterator iterEnd = R.end();
//    Relation recordsL;
//
//    // Set the level of the node.
//    this->level = level;
//    this->isLeaf = false;
//    
//    // Determine center
//    Timestamp gstart = min_element(iterBegin, iterEnd)->start;
//    Timestamp gend = max_element(iterBegin, iterEnd, CompareRecordsByEnd)->end; // TODO by end
//    this->center = gstart+ceil((gend-gstart) / 2.0);
//    
//    // Determine the last record in the left subtree
//    RelationIterator iterBeginR = lower_bound(iterBegin, iterEnd, Record(0, center, center));
//
//    for (auto iter = iterBegin; iter != iterBeginR; iter++)
//    {
//        if (iter->end >= this->center)
//        {
//            this->recordsByStart.emplace_back(iter->id, iter->start);
//            this->recordsByEnd.emplace_back(iter->id, iter->end);
//        }
//        else
//            recordsL.emplace_back(iter->id, iter->start, iter->end);
//    }
//
//    // Relation recordsByStart is by construction sorted by start, hence sort recordsByEnd
//    this->recordsByEnd.sortByEnd();
//
//    if (this->level+1 != maxLevel)
//    {
//        // Create subtrees
//        if (!recordsL.empty())
//            this->left = new IntervalTreeNode(this->level+1, recordsL, maxLevel);
//        else
//            this->left = NULL;
//        
//        if (iterBeginR != iterEnd)
//            this->right = new IntervalTreeNode(this->level+1, Relation(iterBeginR, iterEnd), maxLevel);
//        else
//            this->right = NULL;
//    }
//    else
//    {
//        // Create leaf nodes under current node
//        if (!recordsL.empty())
//            this->left = new IntervalTreeLeafNode(this->level+1, recordsL);
//        else
//            this->left = NULL;
//        
//        if (iterBeginR != iterEnd)
//            this->right = new IntervalTreeLeafNode(this->level+1, Relation(iterBeginR, iterEnd));
//        else
//            this->right = NULL;
//    }
//}

//IntervalTreeNode::IntervalTreeNode(const unsigned int level, RelationIterator iterBegin, RelationIterator iterEnd, const unsigned int maxLevel)
//{
//    Relation recordsL;
//
//    // Set the level of the node.
//    this->level = level;
//    this->isLeaf = false;
//    
//    // Determine center
//    Timestamp gstart = min_element(iterBegin, iterEnd)->start;
//    Timestamp gend = max_element(iterBegin, iterEnd, CompareRecordsByEnd)->end;
//    this->center = gstart+ceil((gend-gstart) / 2.0);
//    
//    // Determine the last record in the left subtree
//    RelationIterator pivot = lower_bound(iterBegin, iterEnd, Record(0, center, center));
//
//    for (auto iter = iterBegin; iter != pivot; iter++)
//    {
//        if (iter->end >= this->center)
//        {
//            this->recordsByStart.emplace_back(iter->id, iter->start);
//            this->recordsByEnd.emplace_back(iter->id, iter->end);
//        }
//        else
//            recordsL.emplace_back(iter->id, iter->start, iter->end);
//    }
//
//    // Relation recordsByStart is by construction sorted by start, hence sort recordsByEnd
//    this->recordsByEnd.sortByEnd();
//
//    if (this->level+1 != maxLevel)
//    {
//        // Create subtrees
//        if (!recordsL.empty())
//            this->left = new IntervalTreeNode(this->level+1, recordsL.begin(), recordsL.end());
//        else
//            this->left = NULL;
//        
//        if (pivot != iterEnd)
//            this->right = new IntervalTreeNode(this->level+1, pivot, iterEnd);
//        else
//            this->right = NULL;
//    }
//    else
//    {
//        // Create leaf nodes under current node
//        if (!recordsL.empty())
//            this->left = new IntervalTreeLeafNode(this->level+1, recordsL);
//        else
//            this->left = NULL;
//        
//        if (pivot != iterEnd)
//            this->right = new IntervalTreeLeafNode(this->level+1, pivot, iterEnd);
//        else
//            this->right = NULL;
//    }
//
//}

void IntervalTreeNode::print(const char c)
{
    cout << "Level = " << this->level << "\tcenter = " << this->center << endl;
    
    cout << "\tByStart (" << this->recordsByStart.size() << "): ";
    for (const RecordStart &r: this->recordsByStart)
    {
        cout << "<r" << r.id << "," << r.start << "> ";
    }
    cout << endl;
    
    cout << "\tByEnd (" << this->recordsByEnd.size() << "): ";
    for (const RecordEnd &r: this->recordsByEnd)
    {
        cout << "<r" << r.id << "," << r.end << "> ";
    }
    cout << endl;

    if (this->left)
    {
        cout << "left"<<endl;
        this->left->print(c);
    }

    if (this->right)
    {
        cout << "right"<<endl;
        this->right->print(c);
    }
}


size_t IntervalTreeNode::getSize()
{
//    return sizeof(IntervalTreeNode) + this->recordsByStart.size()*sizeof(RecordStart) + this->recordsByEnd.size()*sizeof(RecordEnd);
    return sizeof(IntervalTreeNode)-sizeof(unsigned int) + this->recordsByStart.size()*sizeof(RecordStart) + this->recordsByEnd.size()*sizeof(RecordEnd);
}
 
 
IntervalTreeNode::~IntervalTreeNode()
{
    if (this->left)
        delete this->left;
    
    if (this->right)
        delete this->right;
}


// Updating
void IntervalTreeNode::insert(const Record &r)
{
    // Update record lists
    RecordStart rs(r.id, r.start);
    RecordEnd re(r.id, r.end);

    auto iterS = lower_bound(this->recordsByStart.begin(), this->recordsByStart.end(), rs);
    this->recordsByStart.insert(iterS, rs);
    auto iterE = lower_bound(this->recordsByEnd.begin(), this->recordsByEnd.end(), re);
    this->recordsByEnd.insert(iterE, re);
}

 
void IntervalTreeNode::remove(const Record &r)
{
    // Update record lists
    RecordStart rs(r.id, r.start);
    pair<RelationStartIterator, RelationStartIterator> itersS = equal_range(this->recordsByStart.begin(), this->recordsByStart.end(), rs);
    auto iters = itersS.first;
    while (iters != itersS.second)
    {
        if (iters->id == rs.id)
        {
            this->recordsByStart.erase(iters);
            break;
        }
        else
            iters++;
    }
    RecordEnd re(r.id, r.end);
    pair<RelationEndIterator, RelationEndIterator>  itersE = equal_range(this->recordsByEnd.begin(), this->recordsByEnd.end(), re);
    auto itere = itersE.first;
    while (itere != itersE.second)
    {
        if (itere->id == re.id)
        {
            this->recordsByEnd.erase(itere);
            break;
        }
        else
            itere++;
    }
}

// Querying
inline void IntervalTreeNode::scan_NoChecks_gOverlaps(size_t &result)
{
//    cout << "IntervalTreeNode::scan_NoChecks_gOverlaps" << endl;
    RelationStartIterator iterBegin = this->recordsByStart.begin();
    RelationStartIterator iterEnd = this->recordsByStart.end();
    
    for (auto iter = iterBegin; iter != iterEnd; iter++)
    {
#ifdef WORKLOAD_COUNT
        result++;
#else
        result ^= iter->id;
#endif
    }
}

inline void IntervalTreeNode::scan_CheckStart_gOverlaps(const RangeQuery &q, size_t &result)
{
    RelationStartIterator iterBegin = this->recordsByStart.begin();
    RelationStartIterator iterEnd = this->recordsByStart.end();
    RelationStartIterator pivot = upper_bound(iterBegin, iterEnd, RecordStart(0, q.end+1));
    
//    if (this->center == 8573400)
//    {
//        cout<<"\t";
//        for (auto iter = iterBegin; iter != iterEnd; iter++)
//            cout << iter->id << " ";
//        cout<<endl;
//    }
    
    for (auto iter = iterBegin; iter != pivot; iter++)
    {
#ifdef WORKLOAD_COUNT
        result++;
#else
        result ^= iter->id;
#endif
    }
}
 
 
inline void IntervalTreeNode::scan_CheckEnd_gOverlaps(const RangeQuery &q, size_t &result)
{
    RelationEndIterator iterBegin = this->recordsByEnd.begin();
    RelationEndIterator iterEnd = this->recordsByEnd.end();
    RelationEndIterator pivot = lower_bound(iterBegin, iterEnd, RecordEnd(0, q.start));
    
    for (auto iter = pivot; iter != iterEnd; iter++)
    {
#ifdef WORKLOAD_COUNT
        result++;
#else
        result ^= iter->id;
#endif
    }
}

inline void IntervalTreeNode::scan_NoChecks_gOverlaps(vector<RecordId> &results)
{
    RelationStartIterator iterBegin = this->recordsByStart.begin();
    RelationStartIterator iterEnd = this->recordsByStart.end();
    
    for (auto iter = iterBegin; iter != iterEnd; iter++)
    {
        results.push_back(iter->id);
    }
}

inline void IntervalTreeNode::scan_CheckStart_gOverlaps(const RangeQuery &q, vector<RecordId> &results)
{
    RelationStartIterator iterBegin = this->recordsByStart.begin();
    RelationStartIterator iterEnd = this->recordsByStart.end();
    RelationStartIterator pivot = upper_bound(iterBegin, iterEnd, RecordStart(0, q.end+1));
    
    for (auto iter = iterBegin; iter != pivot; iter++)
    {
        results.push_back(iter->id);
    }
}

inline void IntervalTreeNode::scan_CheckEnd_gOverlaps(const RangeQuery &q, vector<RecordId> &results)
{
    RelationEndIterator iterBegin = this->recordsByEnd.begin();
    RelationEndIterator iterEnd = this->recordsByEnd.end();
    RelationEndIterator pivot = lower_bound(iterBegin, iterEnd, RecordEnd(0, q.start));
    
    for (auto iter = pivot; iter != iterEnd; iter++)
    {
        results.push_back(iter->id);
    }
}



//IntervalTreeLeafNode::IntervalTreeLeafNode(const unsigned int level, const Relation &R) : IntervalTreeNode()
//{
//    RelationIterator iterBegin = R.begin();
//    RelationIterator iterEnd = R.end();
//
//    // Set the level of the node.
//    this->level = level;
//    this->isLeaf = true;
//    
//    // Determine center
//    Timestamp gstart = min_element(iterBegin, iterEnd)->start;
//    Timestamp gend = max_element(iterBegin, iterEnd, CompareRecordsByEnd)->end; // TODO by end
//    this->center = gstart+ceil((gend-gstart) / 2.0);
//    
//    // Determine the last record in the left subtree
//    RelationIterator pivot = lower_bound(iterBegin, iterEnd, Record(0, center, center));
//
//    for (auto iter = iterBegin; iter != pivot; iter++)
//    {
//        if (iter->end >= this->center)
//        {
//            this->recordsByStart.emplace_back(iter->id, iter->start);
//            this->recordsByEnd.emplace_back(iter->id, iter->end);
//        }
//        else
//        {
//#ifdef SPLIT_LEAF_SUBTREE
//            this->recordsLeftSubtree.emplace_back(iter->id, iter->start, iter->end);
//#else
//            this->recordsSubtree.emplace_back(iter->id, iter->start, iter->end);
//#endif
//        }
//    }
//
//#ifdef SPLIT_LEAF_SUBTREE
//    this->recordsRightSubtree.insert(this->recordsRightSubtree.end(), pivot, iterEnd);
//#else
//    this->recordsSubtree.insert(this->recordsSubtree.end(), pivot, iterEnd);
//#endif
//
//    // Relation recordsByStart is by construction sorted by start, hence sort recordsByEnd
//    this->recordsByEnd.sortByEnd();
//
//#ifdef SPLIT_LEAF_SUBTREE
//    this->recordsLeftSubtree.sortByEnd();
//#endif
//}
//
//
//IntervalTreeLeafNode::IntervalTreeLeafNode(const unsigned int level, RelationIterator iterBegin, RelationIterator iterEnd) : IntervalTreeNode()
//{
//    // Set the level of the node.
//    this->level = level;
//    this->isLeaf = true;
//    
//    // Determine center
//    Timestamp gstart = min_element(iterBegin, iterEnd)->start;
//    Timestamp gend = max_element(iterBegin, iterEnd, CompareRecordsByEnd)->end; // TODO by end
//    this->center = gstart+ceil((gend-gstart) / 2.0);
//    
//    // Determine the last record in the left subtree
//    RelationIterator pivot = lower_bound(iterBegin, iterEnd, Record(0, center, center));
//
//    for (auto iter = iterBegin; iter != pivot; iter++)
//    {
//        if (iter->end >= this->center)
//        {
//            this->recordsByStart.emplace_back(iter->id, iter->start);
//            this->recordsByEnd.emplace_back(iter->id, iter->end);
//        }
//        else
//        {
//#ifdef SPLIT_LEAF_SUBTREE
//            this->recordsLeftSubtree.emplace_back(iter->id, iter->start, iter->end);
//#else
//            this->recordsSubtree.emplace_back(iter->id, iter->start, iter->end);
//#endif
//        }
//    }
//
//#ifdef SPLIT_LEAF_SUBTREE
//    this->recordsRightSubtree.insert(this->recordsRightSubtree.end(), pivot, iterEnd);
//#else
//    this->recordsSubtree.insert(this->recordsSubtree.end(), pivot, iterEnd);
//#endif
//
//    // Relation recordsByStart is by construction sorted by start, hence sort recordsByEnd
//    this->recordsByEnd.sortByEnd();
//
//#ifdef SPLIT_LEAF_SUBTREE
//    this->recordsLeftSubtree.sortByEnd();
//#endif
//}
//
//
//void IntervalTreeLeafNode::print(const char c)
//{
//    cout << "Level = " << this->level << "\tcenter = " << this->center << endl;
//    
//    cout << "\tByStart (" << this->recordsByStart.size() << "): ";
//    for (const RecordStart &r: this->recordsByStart)
//    {
//        cout << "<r" << r.id << "," << r.start << "> ";
//    }
//    cout << endl;
//    
//    cout << "\tByEnd (" << this->recordsByEnd.size() << "): ";
//    for (const RecordEnd &r: this->recordsByEnd)
//    {
//        cout << "<r" << r.id << "," << r.end << "> ";
//    }
//    cout << endl;
//
//#ifdef SPLIT_LEAF_SUBTREE
//    cout << "\tLeftSubtree (" << this->recordsLeftSubtree.size() << "): ";
//    for (const Record &r: this->recordsLeftSubtree)
//    {
//        cout << "<r" << r.id << "," << r.start << "," << r.end << "> ";
//    }
//    cout << endl;
//    
//    cout << "\tRightsubtree (" << this->recordsRightSubtree.size() << "): ";
//    for (const Record &r: this->recordsRightSubtree)
//    {
//        cout << "<r" << r.id << "," << r.start << "," << r.end << "> ";
//    }
//    cout << endl;
//#else
//    cout << "\tSubtree (" << this->recordsSubtree.size() << "): ";
//    for (const Record &r: this->recordsSubtree)
//    {
//        cout << "<r" << r.id << "," << r.end << "> ";
//    }
//    cout << endl;
//#endif
//}
//
//
//size_t IntervalTreeLeafNode::getSize()
//{
//#ifdef SPLIT_LEAF_SUBTREE
//    return sizeof(IntervalTreeNode) + this->recordsByStart.size()*sizeof(RecordStart) + this->recordsByEnd.size()*sizeof(Record) + this->recordsLeftSubtree.size()*sizeof(Record) + this->recordsRightSubtree.size()*sizeof(Record);
//#else
//    return sizeof(IntervalTreeNode) + this->recordsByStart.size()*sizeof(RecordStart) + this->recordsByEnd.size()*sizeof(RecordEnd) + this->recordsSubtree.size()*sizeof(Record);
//#endif
//}
//
//IntervalTreeLeafNode::~IntervalTreeLeafNode()
//{
//}
//
//#ifdef SPLIT_LEAF_SUBTREE
//inline void IntervalTreeLeafNode::scanLeftSubtree_CheckBothTimestamps_gOverlaps(const RangeQuery &q, size_t &result)
//{
////    cout << "\tIntervalTreeLeafNode::scanLeftSubtree_CheckBothTimestamps_gOverlaps" << endl;
////    RelationIterator iterBegin = this->recordsLeftSubtree.begin();
////    RelationIterator iterEnd = this->recordsLeftSubtree.end();
////    
////    for (auto iter = iterBegin; iter != iterEnd; iter++)
////    {
////        if ((iter->start <= q.end) && (q.start <= iter->end))
////        {
////#ifdef WORKLOAD_COUNT
////            result++;
////#else
////            result ^= iter->id;
////#endif
////        }
////    }
////    return;
//    RelationIterator iterBegin = this->recordsLeftSubtree.begin();
//    RelationIterator iterEnd = this->recordsLeftSubtree.end();
//    RelationIterator pivot = lower_bound(iterBegin, iterEnd, Record(0, q.start, q.start), CompareRecordsByEnd);
//
//    for (auto iter = pivot; iter != iterEnd; iter++)
//    {
//        if (iter->start <= q.end)
//        {
//#ifdef WORKLOAD_COUNT
//            result++;
//#else
//            result ^= iter->id;
//#endif
//        }
//    }
//}
//
//inline void IntervalTreeLeafNode::scanRightSubtree_CheckBothTimestamps_gOverlaps(const RangeQuery &q, size_t &result)
//{
////    cout << "\tIntervalTreeLeafNode::scanRightSubtree_CheckBothTimestamps_gOverlaps" << endl;
//    RelationIterator iterBegin = this->recordsRightSubtree.begin();
//    RelationIterator iterEnd = this->recordsRightSubtree.end();
//    RelationIterator pivot = lower_bound(iterBegin, iterEnd, Record(0, q.end+1, q.end+1));
//    
//    for (auto iter = iterBegin; iter != pivot; iter++)
//    {
//        if (q.start <= iter->end)
//        {
//#ifdef WORKLOAD_COUNT
//            result++;
//#else
//            result ^= iter->id;
//#endif
//        }
//    }
//}
//
//inline void IntervalTreeLeafNode::scanLeftSubtree_CheckEnd_gOverlaps(const RangeQuery &q, size_t &result)
//{
//    // TODO
////    cout << "\tIntervalTreeLeafNode::scanLeftSubtree_CheckEnd_gOverlaps" << endl;
////    RelationIterator iterBegin = this->recordsLeftSubtree.begin();
////    RelationIterator iterEnd = this->recordsLeftSubtree.end();
////
////    for (auto iter = iterBegin; iter != iterEnd; iter++)
////    {
////        if ((iter->start <= q.end) && (q.start <= iter->end))
////        {
////#ifdef WORKLOAD_COUNT
////            result++;
////#else
////            result ^= iter->id;
////#endif
////        }
////    }
////    return;
//    RelationIterator iterBegin = this->recordsLeftSubtree.begin();
//    RelationIterator iterEnd = this->recordsLeftSubtree.end();
//    RelationIterator pivot = lower_bound(iterBegin, iterEnd, Record(0, q.start, q.start), CompareRecordsByEnd);
//
//    for (auto iter = pivot; iter != iterEnd; iter++)
//    {
//#ifdef WORKLOAD_COUNT
//        result++;
//#else
//        result ^= iter->id;
//#endif
//    }
//}
//
//inline void IntervalTreeLeafNode::scanRightSubtree_CheckStart_gOverlaps(const RangeQuery &q, size_t &result)
//{
////    cout << "\tIntervalTreeLeafNode::scanRightSubtree_CheckStart_gOverlaps" << endl;
//    RelationIterator iterBegin = this->recordsRightSubtree.begin();
//    RelationIterator iterEnd = this->recordsRightSubtree.end();
//    RelationIterator pivot = lower_bound(iterBegin, iterEnd, Record(0, q.end+1, q.end+1));
//    
//    for (auto iter = iterBegin; iter != pivot; iter++)
//    {
//#ifdef WORKLOAD_COUNT
//        result++;
//#else
//        result ^= iter->id;
//#endif
//    }
//}
//#else
//inline void IntervalTreeLeafNode::scanSubtree_CheckBothTimestamps_gOverlaps(const RangeQuery &q, size_t &result)
//{
////    cout << "\tIntervalTreeLeafNode::scan_RecordsLeftSubtree_gOverlaps" << endl;
//    RelationIterator iterBegin = this->recordsSubtree.begin();
//    RelationIterator iterEnd = this->recordsSubtree.end();
//    RelationIterator pivot = lower_bound(iterBegin, iterEnd, Record(0, q.end+1, q.end+1));
//    for (auto iter = iterBegin; iter != pivot; iter++)
//    {
//        if (q.start <= iter->end)
//        {
//#ifdef WORKLOAD_COUNT
//            result++;
//#else
//// std::cout << "Hit ID: " << *iterI << std::endl;
//            result ^= iter->id;
//#endif
//        }
//    }
//}
//#endif


// Construction
IntervalTree::IntervalTree(const Relation &R)
{
    this->root = new IntervalTreeNode(0, R);
}

IntervalTree::IntervalTree(Relation::iterator begin, Relation::iterator end)
{
    this->root = new IntervalTreeNode(0, begin, end);
}

//IntervalTree::IntervalTree(const Relation &R, const unsigned int maxLevel)
//{
//    this->root = new IntervalTreeNode(0, R, maxLevel);
//}

void IntervalTree::print(const char c)
{
     this->root->print(c);
}

 
 void IntervalTree::getStats()
 {
     //TODO
     size_t numIndexedRecords = 0;
     stack<IntervalTreeNode*> S;
     IntervalTreeNode *curr;
     
     this->numLevels = 0;
     S.push(this->root);
     while (!S.empty())
     {
         curr = S.top();
         S.pop();
         this->numNodes++;
 
         if (curr->level > this->numLevels)
             this->numLevels = curr->level;
         if (!curr->recordsByStart.empty())
             numIndexedRecords += curr->recordsByStart.size();
         
//         if (!curr->isLeaf)
//         {
             if (curr->left)
                 S.push(curr->left);
             if (curr->right)
                 S.push(curr->right);
//         }
     }
     this->numLevels++;  // count root
 }
 
 
 size_t IntervalTree::getSize()
 {
     size_t result = 0;
     stack<IntervalTreeNode*> S;
     IntervalTreeNode *curr;
     
     S.push(this->root);
     while (!S.empty())
     {
         curr = S.top();
         S.pop();
          
         result += curr->getSize();
//         if (!curr->isLeaf)
//         {
             if (curr->left)
                 S.push(curr->left);
             if (curr->right)
                 S.push(curr->right);
//         }
     }
 
     return result;
 }
 
 
 IntervalTree::~IntervalTree()
 {
     delete this->root;
 }
 
 // Updating
 void IntervalTree::insert(const Record &r)
 {
     stack<IntervalTreeNode*> S;
     IntervalTreeNode* curr;
     
     S.push(this->root);
     while (!S.empty())
     {
         curr = S.top();
         S.pop();

         if (r.start <= curr->center)
         {
             if (curr->center <= r.end)
             {
                 // Found the node to store r
                 curr->insert(r);
                 return;
             }
             else if (curr->left)
             {
                 // Check left subtree
                 S.push(curr->left);
             }
         }
         else if (curr->right)
         {
             // Check right subtree
             S.push(curr->right);
         }
     }
     curr->insert(r);
 }
 
 
 void IntervalTree::remove(const Record &r)
 {
     stack<IntervalTreeNode*> S;
     IntervalTreeNode* curr;
     
     S.push(this->root);
     while (!S.empty())
     {
         curr = S.top();
         S.pop();
         
         if (r.start < curr->center)
         {
             if ((curr->center <= r.end) && (!curr->recordsByStart.empty()))
             {
                 // Found the node to store r
                 curr->remove(r);
                 return;
             }
             else if (curr->left)
             {
                 // Check left subtree
                 S.push(curr->left);
             }
         }
         else if (curr->right)
         {
             // Check right subtree
             S.push(curr->right);
         }
     }
     
     curr->remove(r);
 }
 
 // Querying
 size_t IntervalTree::execute_Stabbing(const RangeQuery &q){
     size_t result = 0;
     stack<IntervalTreeNode*> S;
     IntervalTreeNode* curr;

     S.push(this->root);
     while (!S.empty()) {
         curr = S.top();
         S.pop();
         this->nodesAccessed++;
         
//         if (!curr->isLeaf)
//         {
             // Internal node
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
//         }
//         else
//         {
//             // Leaf node
//             IntervalTreeLeafNode *leaf = static_cast<IntervalTreeLeafNode*>(curr);
//             
//             if (q.start == leaf->center)
//             {
//                 leaf->scan_NoChecks_gOverlaps(result);
//
//#ifdef SPLIT_LEAF_SUBTREE
//                 leaf->scanRightSubtree_CheckBothTimestamps_gOverlaps(q, result);
//#endif
//             }
//             else if (q.start < leaf->center)
//             {
//                 leaf->scan_CheckStart_gOverlaps(q, result);
//
//#ifdef SPLIT_LEAF_SUBTREE
//                 leaf->scanLeftSubtree_CheckBothTimestamps_gOverlaps(q, result);
//#endif
//             }
//             else
//             {
//                 leaf->scan_CheckEnd_gOverlaps(q, result);
//                 
//#ifdef SPLIT_LEAF_SUBTREE
//                 leaf->scanRightSubtree_CheckBothTimestamps_gOverlaps(q, result);
//#endif
//             }
//
//#ifndef SPLIT_LEAF_SUBTREE
//             leaf->scanSubtree_CheckBothTimestamps_gOverlaps(q, result);
//#endif
//         }
     }
     return result;
 }

void IntervalTree::execute_Stabbing(const RangeQuery &q, vector<RecordId> &results)
{
    stack<IntervalTreeNode*> S;
    IntervalTreeNode* curr;

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
  
void IntervalTree::execute_Stabbing(const RangeQuery &q, vector<IT_CandidateLog> &candidates) {
    stack<IntervalTreeNode*> S;
    IntervalTreeNode* curr;

    S.push(this->root);
    while (!S.empty()) {
        curr = S.top();
        S.pop();
        this->nodesAccessed++;

//        if (!curr->isLeaf)
//        {
            // Internal node
            if (q.start == curr->center) {
                if (curr->recordsByStart.size() > 0)
                    candidates.emplace_back(curr, IT_RECORDS_BY_START, 0, curr->recordsByStart.size());
                
                if (curr->right)
                    S.push(curr->right);
            }
            else if (q.start < curr->center) {
                auto begin = curr->recordsByStart.cbegin();
                int count = upper_bound(begin, curr->recordsByStart.cend(), RecordStart(0, q.start + 1)) - begin;
                if (count > 0)
                    candidates.emplace_back(curr, IT_RECORDS_BY_START, 0, count);
                
                if (curr->left)
                    S.push(curr->left);
            }
            else {
                auto begin = curr->recordsByEnd.cbegin();
                int l = lower_bound(begin, curr->recordsByEnd.cend(), RecordEnd(0, q.start)) - begin;
                int n = curr->recordsByEnd.size();
                if (l < n)
                    candidates.emplace_back(curr, IT_RECORDS_BY_END, l, n);
                
                if (curr->right)
                    S.push(curr->right);
            }
//        }
//        else
//        {
//            // Leaf node
//            IntervalTreeLeafNode *leaf = static_cast<IntervalTreeLeafNode*>(curr);
//            size_t count = 0;
//            
//            if (q.start == leaf->center) {
//                if (leaf->recordsByStart.size() > 0)
//                    candidates.emplace_back(leaf, IT_RECORDS_BY_START, 0, leaf->recordsByStart.size());
//                
//#ifdef SPLIT_LEAF_SUBTREE
//                // Handle right subtree
//                RelationIterator iterBegin = leaf->recordsRightSubtree.begin();
//                RelationIterator iterEnd = leaf->recordsRightSubtree.end();
//                RelationIterator pivot = lower_bound(iterBegin, iterEnd, Record(0, q.end+1, q.end+1));
//                
//                for (auto iter = iterBegin; iter != pivot; iter++)
//                {
//                    if (q.start <= iter->end)
//                        count++;
//                }
//                
//                if (count > 0)
//                {
//                    leaf->level = count;    // hide number of results inside the level field
//                    candidates.emplace_back(leaf, IT_SUBTREE_RIGHT, 0, (pivot-iterBegin));
//                }
//#endif
//            }
//            else if (q.start < leaf->center) {
//                auto begin = leaf->recordsByStart.cbegin();
//                int count = upper_bound(begin, leaf->recordsByStart.cend(), RecordStart(0, q.start + 1)) - begin;
//                if (count > 0)
//                    candidates.emplace_back(leaf, IT_RECORDS_BY_START, 0, count);
//                
//#ifdef SPLIT_LEAF_SUBTREE
//                // Handle left subtree
//                RelationIterator iterBegin = leaf->recordsLeftSubtree.begin();
//                RelationIterator iterEnd = leaf->recordsLeftSubtree.end();
//                RelationIterator pivot = lower_bound(iterBegin, iterEnd, Record(0, q.start, q.start), CompareRecordsByEnd);
//
//                count = 0;
//                for (auto iter = pivot; iter != iterEnd; iter++)
//                {
//                    if (iter->start <= q.end)
//                        count++;
//                }
//
//                if (count > 0)
//                {
//                    leaf->level = count;    // hide number of results inside the level field
//                    candidates.emplace_back(leaf, IT_SUBTREE_LEFT, 0, (iterEnd-pivot));
//                }
//#endif
//            }
//            else {
//                auto begin = leaf->recordsByEnd.cbegin();
//                int l = lower_bound(begin, leaf->recordsByEnd.cend(), RecordEnd(0, q.start)) - begin;
//                int n = leaf->recordsByEnd.size();
//                if (l < n)
//                    candidates.emplace_back(leaf, IT_RECORDS_BY_END, l, n);
//                
//#ifdef SPLIT_LEAF_SUBTREE
//                // Handle right subtree
//                RelationIterator iterBegin = leaf->recordsRightSubtree.begin();
//                RelationIterator iterEnd = leaf->recordsRightSubtree.end();
//                RelationIterator pivot = lower_bound(iterBegin, iterEnd, Record(0, q.end+1, q.end+1));
//                
//                for (auto iter = iterBegin; iter != pivot; iter++)
//                {
//                    if (q.start <= iter->end)
//                        count++;
//                }
//                
//                if (count > 0)
//                {
//                    leaf->level = count;    // hide number of results inside the level field
//                    candidates.emplace_back(leaf, IT_SUBTREE_RIGHT, 0, (pivot-iterBegin));
//                }
//#endif
//            }
//            
//#ifndef SPLIT_LEAF_SUBTREE
//            // Handle subtree
//            RelationIterator iterBegin = leaf->recordsSubtree.begin();
//            RelationIterator iterEnd = leaf->recordsSubtree.end();
//            RelationIterator pivot = lower_bound(iterBegin, iterEnd, Record(0, q.end+1, q.end+1));
//            for (auto iter = iterBegin; iter != pivot; iter++)
//            {
//                if (q.start <= iter->end)
//                {
//                    count++;
////                        cout << "\t\tfound: [" << iter->first << ".." << iter->second << "]" << endl;
//                }
//            }
//
//            if (count > 0)
//            {
//                leaf->level = count;    // hide number of results inside the level field
//                candidates.emplace_back(leaf, IT_SUBTREE, 0, (pivot-iterBegin));
//            }
//#endif
//        }
    }
}


size_t IntervalTree::execute_gOverlaps(const RangeQuery &q)
{
    size_t result = 0;
    stack<IntervalTreeNode*> S;
    IntervalTreeNode* curr;

    S.push(this->root);
    while (!S.empty())
    {
        curr = S.top();
        S.pop();
        this->nodesAccessed++;
        
//        if (!curr->isLeaf)
//        {
            if (q.start <= curr->center)
            {
                if (curr->center <= q.end)
                {
                    // No comparisons needed, all contents overlap query; doesn't matter which of recordsByStart or recordsByEnd is scanned.
                    curr->scan_NoChecks_gOverlaps(result);
                    
                    if (curr->right)
                        S.push(curr->right);
                }
                else
                    curr->scan_CheckStart_gOverlaps(q, result);

                if (curr->left)
                    S.push(curr->left);
            }
            else
            {
                curr->scan_CheckEnd_gOverlaps(q, result);

                if (curr->right)
                    S.push(curr->right);
            }
//        }
//        else
//        {
//            IntervalTreeLeafNode *leaf = static_cast<IntervalTreeLeafNode*>(curr);
////
////            if (q.end < leaf->center)
////            {
////                leaf->scan_CheckStart_gOverlaps(q, result);
////                leaf->scanLeftSubtree_CheckBothTimestamps_gOverlaps(q, result);
////            }
////            else if (leaf->center < q.start)
////            {
////                leaf->scan_CheckEnd_gOverlaps(q, result);
////                leaf->scanRightSubtree_CheckBothTimestamps_gOverlaps(q, result);
////            }
////            else
////            {
////                leaf->scan_NoChecks_gOverlaps(result);
////                leaf->scanLeftSubtree_CheckBothTimestamps_gOverlaps(q, result);
////                leaf->scanRightSubtree_CheckBothTimestamps_gOverlaps(q, result);
////            }
////
////
//            if (q.start <= leaf->center)
//            {
//                if (leaf->center <= q.end)
//                {
//                    // No comparisons needed, all contents overlap query; doesn't matter which of recordsByStart or recordsByEnd is scanned.
//                    leaf->scan_NoChecks_gOverlaps(result);
//                    
//#ifdef SPLIT_LEAF_SUBTREE
//                    leaf->scanRightSubtree_CheckBothTimestamps_gOverlaps(q, result);
//#endif
//                }
//                else
//                    leaf->scan_CheckStart_gOverlaps(q, result);
//
//#ifdef SPLIT_LEAF_SUBTREE
//                leaf->scanLeftSubtree_CheckBothTimestamps_gOverlaps(q, result);
//#else
//                leaf->scanSubtree_CheckBothTimestamps_gOverlaps(q, result);
//#endif
//            }
//            else
//            {
//                leaf->scan_CheckEnd_gOverlaps(q, result);
//
//#ifdef SPLIT_LEAF_SUBTREE
//                leaf->scanRightSubtree_CheckBothTimestamps_gOverlaps(q, result);
//#else
//                leaf->scanSubtree_CheckBothTimestamps_gOverlaps(q, result);
//#endif
//            }
//        }
    }

    return result;
}

void IntervalTree::execute_gOverlaps(const RangeQuery &q, vector<RecordId> &results)
{
    stack<IntervalTreeNode*> S;
    IntervalTreeNode* curr;

    S.push(this->root);
    while (!S.empty())
    {
        curr = S.top();
        S.pop();
        this->nodesAccessed++;
        
        if (q.start <= curr->center)
        {
            if (curr->center <= q.end)
            {
                // No comparisons needed, all contents overlap query
                curr->scan_NoChecks_gOverlaps(results);
                
                if (curr->right)
                    S.push(curr->right);
            }
            else
                curr->scan_CheckStart_gOverlaps(q, results);

            if (curr->left)
                S.push(curr->left);
        }
        else
        {
            curr->scan_CheckEnd_gOverlaps(q, results);

            if (curr->right)
                S.push(curr->right);
        }
    }
}

void IntervalTree::execute_gOverlaps(const RangeQuery &q, vector<IT_CandidateLog> &candidates) {
    stack<IntervalTreeNode*> S;
    IntervalTreeNode* curr;

    S.push(this->root);
    while (!S.empty()) {
        curr = S.top();
        S.pop();
        this->nodesAccessed++;

//        if (!curr->isLeaf)
//        {
            // Internal node
            if (q.start <= curr->center) {
                if (curr->center <= q.end) {
                    // Case: query straddles center
                    if (curr->recordsByStart.size() > 0)
                        candidates.emplace_back(curr, IT_RECORDS_BY_START, 0, curr->recordsByStart.size());
                    
                    if (curr->right)
                        S.push(curr->right);
                }
                else {
                    // Entirely to the left of the center
                    RelationStartIterator pivot = upper_bound(curr->recordsByStart.begin(), curr->recordsByStart.end(), RecordStart(0, q.end + 1));
                    int count = distance(curr->recordsByStart.cbegin(), pivot);
                    if (count > 0) {
                        candidates.emplace_back(curr, IT_RECORDS_BY_START, 0, count);
                    }
                }
                
                if (curr->left)
                    S.push(curr->left);
            }
            else {
                // Entirely to the right of center
                RelationEndIterator pivot = lower_bound(curr->recordsByEnd.cbegin(), curr->recordsByEnd.cend(), RecordEnd(0, q.start));
                int l = distance(curr->recordsByEnd.cbegin(), pivot);
                int n = curr->recordsByEnd.size();
                if (l < n) {
                    candidates.emplace_back(curr, IT_RECORDS_BY_END, l, n);
                }
                
                if (curr->right)
                    S.push(curr->right);
            }
//        }
//        else
//        {
//            // Leaf node
//            IntervalTreeLeafNode *leaf = static_cast<IntervalTreeLeafNode*>(curr);
//            size_t count = 0;
//            
//            if (q.start <= leaf->center) {
//                if (leaf->center <= q.end) {
//                    // Case: query straddles center
//                    if (leaf->recordsByStart.size() > 0)
//                        candidates.emplace_back(leaf, IT_RECORDS_BY_START, 0, leaf->recordsByStart.size());
//                    
//#ifdef SPLIT_LEAF_SUBTREE
//                    // Handle right subtree
//                    RelationIterator iterBegin = leaf->recordsRightSubtree.begin();
//                    RelationIterator iterEnd = leaf->recordsRightSubtree.end();
//                    RelationIterator pivot = lower_bound(iterBegin, iterEnd, Record(0, q.end+1, q.end+1));
//                    
//                    for (auto iter = iterBegin; iter != pivot; iter++)
//                    {
//                        if (q.start <= iter->end)
//                            count++;
//                    }
//                    
//                    if (count > 0)
//                    {
//                        leaf->level = count;    // hide number of results inside the level field
//                        candidates.emplace_back(leaf, IT_SUBTREE_RIGHT, 0, (pivot-iterBegin));
//                    }
//#endif
//                }
//                else {
//                    // Entirely to the left of the center
//                    RelationStartIterator pivot = upper_bound(leaf->recordsByStart.begin(), leaf->recordsByStart.end(), RecordStart(0, q.end + 1));
//                    int count = distance(leaf->recordsByStart.cbegin(), pivot);
//                    if (count > 0) {
//                        candidates.emplace_back(leaf, IT_RECORDS_BY_START, 0, count);
//                    }
//                }
//
//#ifdef SPLIT_LEAF_SUBTREE
//                // Handle left subtree
//                RelationIterator iterBegin = leaf->recordsLeftSubtree.begin();
//                RelationIterator iterEnd = leaf->recordsLeftSubtree.end();
//                RelationIterator pivot = lower_bound(iterBegin, iterEnd, Record(0, q.start, q.start), CompareRecordsByEnd);
//
//                count = 0;
//                for (auto iter = pivot; iter != iterEnd; iter++)
//                {
//                    if (iter->start <= q.end)
//                        count++;
//                }
//
//                if (count > 0)
//                {
//                    leaf->level = count;    // hide number of results inside the level field
//                    candidates.emplace_back(leaf, IT_SUBTREE_LEFT, 0, (iterEnd-pivot));
//                }
//#endif
//            }
//            else {
//                // Entirely to the right of center
//                RelationEndIterator pivot = lower_bound(leaf->recordsByEnd.cbegin(), leaf->recordsByEnd.cend(), RecordEnd(0, q.start));
//                int l = distance(leaf->recordsByEnd.cbegin(), pivot);
//                int n = leaf->recordsByEnd.size();
//                if (l < n) {
//                    candidates.emplace_back(leaf, IT_RECORDS_BY_END, l, n);
//                }
//                
//#ifdef SPLIT_LEAF_SUBTREE
//                // Handle right subtree
//                RelationIterator iterBegin = leaf->recordsRightSubtree.begin();
//                RelationIterator iterEnd = leaf->recordsRightSubtree.end();
//                RelationIterator spivot = lower_bound(iterBegin, iterEnd, Record(0, q.end+1, q.end+1));
//                
//                for (auto iter = iterBegin; iter != spivot; iter++)
//                {
//                    if (q.start <= iter->end)
//                        count++;
//                }
//
//                if (count > 0)
//                {
//                    leaf->level = count;    // hide number of results inside the level field
//                    candidates.emplace_back(leaf, IT_SUBTREE_RIGHT, 0, (spivot-iterBegin));
//                }
//#endif
//            }
//            
//#ifndef SPLIT_LEAF_SUBTREE
//            // Handle subtree
//            RelationIterator iterBegin = leaf->recordsSubtree.begin();
//            RelationIterator iterEnd = leaf->recordsSubtree.end();
//            RelationIterator pivot = lower_bound(iterBegin, iterEnd, Record(0, q.end+1, q.end+1));
//            for (auto iter = iterBegin; iter != pivot; iter++)
//            {
//                if (q.start <= iter->end)
//                {
//                    count++;
////                        cout << "\t\tfound: [" << iter->first << ".." << iter->second << "]" << endl;
//                }
//            }
//
//            if (count > 0)
//            {
//                leaf->level = count;    // hide number of results inside the level field
//                candidates.emplace_back(leaf, IT_SUBTREE, 0, (pivot-iterBegin));
//            }
//#endif
//        }
    }
}

