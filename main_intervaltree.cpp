#include "getopt.h"
#include "def_global.h"
#include "./containers/relation.h"
#include "./indices/intervaltree.h"
#include "./utils/weighted_sampling.hpp"


void usage() {
    cerr << endl;
    cerr << "PROJECT" << endl;
    cerr << "       Indexing interval data" << endl;
    cerr << endl;
    cerr << "USAGE" << endl;
    cerr << "       ./query_intervaltree.exec [OPTION]... [DATA] [QUERIES]" << endl;
    cerr << endl;
    cerr << "DESCRIPTION" << endl;
    cerr << "       -q predicate" << endl;
    cerr << "              set the predicate type: currently only \"GOVERLAPS\" is supported" << endl;
    cerr << "       -s size" << endl;
    cerr << "              set the sample size" << endl;
    cerr << "       -r runs" << endl;
    cerr << "              set the number of runs per query; omit option for default 1" << endl;
    cerr << "       -v" << endl;
    cerr << "              activate verbose mode; print the trace for every query; otherwise only the final report" << endl;
    cerr << endl;
    cerr << "EXAMPLES" << endl;
    cerr << "       ./query_intervaltree.exec -q gOVERLAPS -r 10 inputs/AARHUS-BOOKS_2013_LIT_Sven.txt queries/AARHUS-BOOKS_2013_LIT_qe1%_qn10000.txt" << endl;
    cerr << "       ./query_intervaltree.exec -q gOVERLAPS inputs/AARHUS-BOOKS_2013_LIT_Sven.txt queries/AARHUS-BOOKS_2013_LIT_qe1%_qn10000.txt" << endl;
    cerr << endl;
}


int main(int argc, char **argv)
{
    Timer tim;
    Relation R;
    IntervalTree *idxR;
    size_t totalResult = 0, queryResult = 0, numQueries = 0;
    double totalIndexTime = 0, totalQueryTime = 0, queryTime = 0;
    double totalSamplingTime = 0, samplingTime = 0;
    size_t totalLogs = 0;
    Timestamp qstart, qend;
    RunSettings settings;
    char c;
    string strPredicate = "GOVERLAPS";
//    unsigned int maxLevel = 0;
    
    // Parse command line input
    settings.init();
    settings.method = "interval tree";
//    while ((c = getopt(argc, argv, "q:d:q:s:r:v")) != -1)
    while ((c = getopt(argc, argv, "s:r:v")) != -1)
    {
        switch (c)
        {
//            case 'd':
//                maxLevel = atoi(optarg)-1;
//                break;

            case 'q':
                strPredicate = toUpperCase((char*)optarg);
                break;

            case 's':
                settings.sampleSize = atoi(optarg);
                queryResult = settings.sampleSize;
                break;

            case 'r':
                settings.numRuns = atoi(optarg);
                break;
                
            case 'v':
                settings.verbose = true;
                break;
                
            default:
                cerr << endl << "Error - unknown option '" << c << "'" << endl << endl;
                usage();
                return 1;
        }
    }
    
    // Sanity check
    if (argc-optind != 2)
    {
        usage();
        return 1;
    }
//    if (maxLevel < 0)
//    {
//        cerr << endl << "Error - tree depth must be over 0" << endl << endl;
//        usage();
//        return 1;
//    }
    if (!checkPredicate(strPredicate, settings))
    {
        if (strPredicate == "")
            cerr << endl << "Error - predicate type not defined" << endl << endl;
        else
            cerr << endl << "Error - unknown predicate type \"" << strPredicate << "\"" << endl << endl;
        usage();
        return 1;
    }
    if (settings.sampleSize < 0)
    {
        cerr << endl << "Error - sample size must be over 0" << endl << endl;
        usage();
        return 1;
    }
    settings.dataFile = argv[optind];
    settings.queryFile = argv[optind+1];
    
    // Load data and queries
    R.load(settings.dataFile);
    settings.maxBits = int(log2(R.gend-R.gstart)+1);
    
    ifstream fQ(settings.queryFile);
    if (!fQ)
    {
        cerr << endl << "Error - cannot open query file \"" << settings.queryFile << "\"" << endl << endl;
        return 1;
    }

    // Build index
//    if (maxLevel == 0)
//    {
        tim.start();
        R.sortByStart();
        idxR = new IntervalTree(R);
        totalIndexTime = tim.stop();
//    }
//    else
//    {
//        tim.start();
//        R.sortByStart();
//        idxR = new IntervalTree(R, maxLevel);
//        totalIndexTime = tim.stop();
//    }

    idxR->getStats();

    // Execute queries
    size_t sumQ = 0;
    if (settings.verbose)
    {
        if (settings.sampleSize == 0)
            cout << "Query\tType\tPredicate\tMethod\tResult\tTime" << endl;
        else
            cout << "Query\tType\tPredicate\tMethod\tSample size\tTime" << endl;
    }
    while (fQ >> qstart >> qend) {
        sumQ += qend - qstart;
        numQueries++;
        
        for (auto r = 0; r < settings.numRuns; r++) {
            if (settings.typePredicate == PREDICATE_GOVERLAPS) {
                if (settings.sampleSize == 0) {
                    vector<RecordId> results;
                    
                    tim.start();
                    idxR->execute_gOverlaps(RangeQuery(numQueries, qstart, qend), results);
                    queryTime = tim.stop();
                    
                    // XOR the results for verification
                    queryResult = 0;
                    for (const auto& id : results) 
                        queryResult ^= id;
                }
                else {
                    // Step 1: create logs
                    vector<IntervalTree::IT_CandidateLog> candidates;
                    // candidates.reserve(idxR->numNodes * 0.3);
                    
                    tim.start();
                    idxR->execute_gOverlaps(RangeQuery(numQueries, qstart, qend), candidates);
                    queryTime = tim.stop();

                    for (const auto& log : candidates)
                        totalLogs += log.right_idx - log.left_idx;
                    
                    // Step 2: do sampling
                    samplingTime = 0;
                    if (!candidates.empty()) {
                        srand(time(NULL));
                        vector<unsigned int> arr;
                        vector<RecordId> samples(settings.sampleSize);
                        vector<unsigned int> candidates_counts(candidates.size(), 0);
                        
                        tim.start();
                        for (unsigned int i = 0; i < candidates.size(); ++i)
                        {
                            auto &log = candidates[i];
//                            if ((log.type != IT_SUBTREE_LEFT) && (log.type != IT_SUBTREE_RIGHT) && (log.type != IT_SUBTREE))
//                            {
//                                cout << "\tlog <l" << log.node->level << ", t" << log.type << ", " << log.left_idx << "," << log.right_idx << ">:\t" << (log.right_idx - log.left_idx) << endl;
                                arr.push_back(log.right_idx - log.left_idx);
//                            }
//                            else
//                            {
////                                cout << "\tlog <l" << maxLevel << ", t" << log.type << ", " << log.left_idx << "," << log.right_idx << ">:\t" << log.node->level << endl;
//                                arr.push_back(log.node->level);
//                            }
                        }
                        alias a(arr);
                        
                        for (unsigned int i = 0; i < settings.sampleSize; ++i)
                            candidates_counts[a.get_indx()]++;
                        
//                        cout<<"checkpoint"<<endl;
                        
                        auto idx = 0;
                        for (unsigned int i = 0; i < candidates.size(); ++i) {
                            if (candidates_counts[i] > 0) {
                                auto &log = candidates[i];
                                
//                                if ((log.type != IT_SUBTREE_LEFT) && (log.type != IT_SUBTREE_RIGHT) && (log.type != IT_SUBTREE))
//                                    cout << "\tlog <l" << log.node->level << ", t" << log.type << ", " << log.left_idx << "," << log.right_idx << ">:\t" << candidates_counts[i] << endl;
//                                else
//                                    cout << "\tlog <l" << maxLevel << ", t" << log.type << ", " << log.left_idx << "," << log.right_idx << ">:\t" << candidates_counts[i] << endl;
                                
                                if (log.type == IT_RECORDS_BY_START) {
                                    for (auto j = 0; j < candidates_counts[i]; ++j) {
                                        auto rnd = log.left_idx + (rand() % (log.right_idx - log.left_idx));
                                        samples[idx] = log.node->recordsByStart[rnd].id;
                                        idx++;
                                    }
                                }
                                else if (log.type == IT_RECORDS_BY_END) {
                                    for (auto j = 0; j < candidates_counts[i]; ++j) {
                                        auto rnd = log.left_idx + (rand() % (log.right_idx - log.left_idx));
                                        samples[idx] = log.node->recordsByEnd[rnd].id;
                                        idx++;
                                    }
                                }
//#ifdef SPLIT_LEAF_SUBTREE
//                                else if (log.type == IT_SUBTREE_LEFT) {
////                                    cout<<"LEFT SUBTREE LOG"<<endl;
//                                    IntervalTreeLeafNode *leaf = static_cast<IntervalTreeLeafNode*>(log.node);
//                                    
//                                    for (auto j = 0; j < candidates_counts[i]; ++j)
//                                    {
//                                        auto rnd = log.left_idx + (rand() % (log.right_idx - log.left_idx));
////                                            cout<<"\t\t"<<rnd<<endl;
//                                        while (leaf->recordsLeftSubtree[rnd].start > qend)
//                                        {
//                                            rnd = log.left_idx + (rand() % (log.right_idx - log.left_idx));
////                                                cout<<"\t\t"<<rnd<<endl;
//                                        }
//                                        samples[idx] = leaf->recordsLeftSubtree[rnd].id;
////                                        cout << "\t" << samples[idx] << ": ["<<leaf->recordsLeftSubtree[rnd].start<<".."<<leaf->recordsLeftSubtree[rnd].end<<"]"<<endl;
//                                        idx++;
//                                    }
//                                }
//                                else if (log.type == IT_SUBTREE_RIGHT) {
////                                    cout<<"RIGHT SUBTREE LOG"<<endl;
//                                    IntervalTreeLeafNode *leaf = static_cast<IntervalTreeLeafNode*>(log.node);
//                                    
//                                    for (auto j = 0; j < candidates_counts[i]; ++j)
//                                    {
//                                        auto rnd = log.left_idx + (rand() % (log.right_idx - log.left_idx));
////                                            cout<<"\t\t"<<rnd<<endl;
//                                        while (qstart > leaf->recordsRightSubtree[rnd].end)
//                                        {
//                                            rnd = log.left_idx + (rand() % (log.right_idx - log.left_idx));
////                                                cout<<"\t\t"<<rnd<<endl;
//                                        }
//                                        samples[idx] = leaf->recordsRightSubtree[rnd].id;
////                                        cout << "\t" << samples[idx] << ": ["<<leaf->recordsRightSubtree[rnd].start<<".."<<leaf->recordsRightSubtree[rnd].end<<"]"<<endl;
//                                        idx++;
//                                    }
//                                }
//#else
//                                else if (log.type == IT_SUBTREE) {
////                                    cout<<"SUBTREE LOG"<<endl;
//                                    IntervalTreeLeafNode *leaf = static_cast<IntervalTreeLeafNode*>(log.node);
//                                    
//                                    for (auto j = 0; j < candidates_counts[i]; ++j)
//                                    {
//                                        auto rnd = log.left_idx + (rand() % (log.right_idx - log.left_idx));
////                                            cout<<"\t\t"<<rnd<<endl;
//                                        while (qstart > leaf->recordsSubtree[rnd].end)
//                                        {
//                                            rnd = log.left_idx + (rand() % (log.right_idx - log.left_idx));
////                                                cout<<"\t\t"<<rnd<<endl;
//                                        }
//                                        samples[idx] = leaf->recordsSubtree[rnd].id;
////                                        cout << "\t" << samples[idx] << ": ["<<leaf->recordsSubtree[rnd].start<<".."<<leaf->recordsSubtree[rnd].end<<"]"<<endl;
//                                        idx++;
//                                    }
//                                }
//#endif
                            }
                        }
                        samplingTime = tim.stop();
                    }
                }
            } else cout << "Wrong predicate type. Use 'gOverlaps'"; 
            
            totalQueryTime += queryTime;
            totalSamplingTime += samplingTime;
            
            if (settings.verbose)
                cout << "[" << qstart << "," << qend << "]\t" << strPredicate << "\t" << settings.method << "\t" << queryResult << "\t" << queryTime << endl;
        }
#ifdef WORKLOAD_COUNT
        totalResult += queryResult;
#else
        totalResult ^= queryResult;
#endif
    }
    fQ.close();
    
    cout << endl;
    cout << "Interval tree" << endl;
    cout << "=============" << endl;
    cout << "Input" << endl;
    cout << "  Data file                 : " << settings.dataFile << endl;
    cout << "  Num of intervals          : " << R.size() << endl;
    cout << "  Domain size               : " << (R.gend-R.gstart) << endl;
    cout << "  Avg interval extent [%]   : "; printf("%f\n", R.avgRecordExtent*100/(R.gend-R.gstart));
    cout << "  Number of nodes           : " << idxR->numNodes << endl;
    cout << endl;
    cout << "Index" << endl;
    cout << "  Num of levels             : " << idxR->numLevels << endl;
    printf( "  Size [Bytes]              : %ld\n", idxR->getSize());
    printf( "  Indexing time [secs]      : %f\n", totalIndexTime);
    cout << endl;
    cout << "Queries" << endl;
    cout << "  Query file                : " << settings.queryFile << endl;
    cout << "  Predicate type            : " << strPredicate << endl;
    cout << "  Num of queries            : " << numQueries << endl;
    cout << "  Avg query extent [%]      : "; printf("%f\n", (((float)sumQ/numQueries)*100)/(R.gend-R.gstart));
    cout << "  Num of runs per query     : " << settings.numRuns << endl;
    cout << "  Avg num of nodes accessed : " << ((float)idxR->nodesAccessed/numQueries)/settings.numRuns << endl;
    cout << endl;
    cout << "Output" << endl;
    if (settings.sampleSize != 0) {
        cout << "  Sample size           : " << settings.sampleSize << endl;
        cout << "  Total Logs length     : " << totalLogs/settings.numRuns << endl;
    }
    else
    {
#ifdef WORKLOAD_COUNT
        cout << "  Total result [COUNT]      : " << totalResult << endl;
#else
        cout << "  Total result [XOR]        : " << totalResult << endl;
#endif
    }
    if (settings.sampleSize > 0) {
        printf( "  Total querying time [secs]: %f\n", (totalQueryTime + totalSamplingTime)/settings.numRuns);
        printf( "  Total logging time [secs] : %f\n", totalQueryTime/settings.numRuns);
        printf( "  Total sampling time [secs]: %f\n", totalSamplingTime/settings.numRuns);
        printf( "  Throughput [queries/sec]  : %f\n", numQueries/((totalQueryTime + totalSamplingTime)/settings.numRuns));
    } else {
        printf( "  Total querying time [secs]: %f\n", totalQueryTime/settings.numRuns);
        printf( "  Throughput [queries/sec]  : %f\n", numQueries/(totalQueryTime/settings.numRuns));
    }
    cout << endl;

    delete idxR;
    return 0;
}
