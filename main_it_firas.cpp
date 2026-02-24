#include "getopt.h"
#include "def_global.h"
#include "./containers/relation.h"
#include "./indices/intervaltree.h"
#include "./utils/weighted_sampling.hpp"
#include <random>
#include <numeric>

void usage() {
    cerr << endl;
    cerr << "PROJECT" << endl;
    cerr << "       Indexing interval data" << endl << endl;
    cerr << "USAGE" << endl;
    cerr << "       ./query_it_firas.exec [OPTION]... [DATA] [QUERIES]" << endl << endl;
    cerr << "DESCRIPTION" << endl;
    cerr << "       -v" << endl;
    cerr << "              activate verbose mode; print the trace for every query; otherwise only the final report" << endl;
    cerr << "       -s size" << endl;
    cerr << "              set the sample size" << endl;
    cerr << "       -r runs" << endl;
    cerr << "              set the number of runs per query; by default 1" << endl << endl;
    cerr << "EXAMPLES" << endl;
    cerr << "       ./query_it_firas.exec -r 10 inputs/AARHUS-BOOKS_2013_LIT_Sven.txt queries/AARHUS-BOOKS_2013_LIT_qe1%_qn10000.txt" << endl;
    cerr << "       ./query_it_firas.exec inputs/AARHUS-BOOKS_2013_LIT_Sven.txt queries/AARHUS-BOOKS_2013_LIT_qe1%_qn10000.txt" << endl;
}

int main(int argc, char **argv) {
    Timer tim;
    Relation R;
    IntervalTree *idxR;
    size_t totalResult = 0, queryResult = 0, numQueries = 0;
    double totalIndexTime = 0, queryTime = 0;
    double totalATime = 0, totalStabbingTime = 0, totalSamplingTime = 0, aTime = 0, stabbingTime = 0, samplingTime = 0;
    size_t totalLogs = 0;
    Timestamp qstart, qend;
    RunSettings settings;
    char c;

    // Parse command line input
    settings.init();
    while ((c = getopt(argc, argv, "q:s:r:")) != -1) {
        switch (c) {
            case 'v':
                settings.verbose = true;
                break;
            case 's':
                settings.sampleSize = atoi(optarg);
                queryResult = settings.sampleSize;
                break;
            case 'r':
                settings.numRuns = atoi(optarg);
                break;
            default:
                cerr << endl << "Error - unknown option '" << c << "'" << endl << endl;
                usage();
                return 1;
        }
    }
    // Sanity check
    if (argc-optind != 2) {
        usage();
        return 1;
    }
    settings.dataFile = argv[optind];
    settings.queryFile = argv[optind+1];

    // Load data and queries
    R.load(settings.dataFile);
    settings.maxBits = int(log2(R.gend-R.gstart)+1);

    ifstream fQ(settings.queryFile);
    if (!fQ) {
        cerr << endl << "Error - cannot open query file \"" << settings.queryFile << "\"" << endl << endl;
        return 1;
    }

    // Build FIRAS
    tim.start();
    R.sortByStart();
    vector<Timestamp> A_timestamps;
    vector<RecordId> A_ids;
    A_timestamps.reserve(R.size());
    A_ids.reserve(R.size());
    for (const auto& rec : R) {
        A_timestamps.push_back(rec.end);
        A_ids.push_back(rec.id);
    }
    // Sort both vectors together
    vector<size_t> indices(R.size());
    iota(indices.begin(), indices.end(), 0);
    sort(indices.begin(), indices.end(), [&](size_t i, size_t j) {
        return A_timestamps[i] < A_timestamps[j];
    });
    vector<Timestamp> sorted_timestamps(R.size());
    vector<RecordId> sorted_ids(R.size());
    for (size_t i = 0; i < R.size(); ++i) {
        sorted_timestamps[i] = A_timestamps[indices[i]];
        sorted_ids[i] = A_ids[indices[i]];
    }
    A_timestamps = move(sorted_timestamps);
    A_ids = move(sorted_ids);
    totalIndexTime = tim.stop();

    tim.start();
    R.sortByStart();
    idxR = new IntervalTree(R);
    totalIndexTime += tim.stop();

    idxR->getStats();

    // Execute queries
    auto comp = [](const Timestamp& a, const Timestamp& b) {return a < b;};

    random_device rd;
    mt19937 rng(rd());

    while (fQ >> qstart >> qend){
        numQueries++;
        for (auto r = 0; r < settings.numRuns; r++){
            if (settings.sampleSize == 0) {
                vector<RecordId> results;

                // Q1: Search right endpoints in array A using two binary searches
                tim.start();
                auto low = lower_bound(A_timestamps.begin(), A_timestamps.end(), qstart, comp);
                auto high = lower_bound(low, A_timestamps.end(), qend, comp);
                auto ilow = A_ids.begin() + (low-A_timestamps.begin());
                auto ihigh = A_ids.begin() + (high-A_timestamps.begin());
                for (auto it = ilow; it != ihigh; ++it)
                    results.push_back(*it);
                aTime = tim.stop();

                // Q2: Stabbing query on interval tree at qend
                tim.start();
                idxR->execute_Stabbing(RangeQuery(numQueries, qend, qend), results);
                stabbingTime = tim.stop();
                
                // XOR for verification
                queryResult = 0;
                for (const auto& id : results) 
                    queryResult ^= id;
            } 
            else {
                // Step 1: create logs for both Q1 and Q2
                vector<IntervalTree::IT_CandidateLog> candidates;
                // candidates.reserve(idxR->numLevels + 1);
                
                // Q1: Add array A as IT_FIRAS_LOG
                tim.start();
                auto low = lower_bound(A_timestamps.begin(), A_timestamps.end(), qstart, comp);
                auto high = lower_bound(low, A_timestamps.end(), qend, comp);
                if (high > low)
                    candidates.push_back(IntervalTree::IT_CandidateLog(nullptr, IT_FIRAS_LOG, low - A_timestamps.begin(), high - A_timestamps.begin()));
                aTime = tim.stop();
                
                // Q2: Get stabbing query candidates
                tim.start();
                idxR->execute_Stabbing(RangeQuery(numQueries, qend, qend), candidates);
                stabbingTime = tim.stop();

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
                    for (unsigned int i = 0; i < candidates.size(); ++i) {
                        auto &log = candidates[i];
                        if ((log.type != IT_SUBTREE_LEFT) && (log.type != IT_SUBTREE_RIGHT) && (log.type != IT_SUBTREE))
                            arr.push_back(log.right_idx - log.left_idx);
                        else
                            arr.push_back(log.node->level);
                    }
                    alias a(arr);
                    for (unsigned int i = 0; i < settings.sampleSize; ++i)
                        candidates_counts[a.get_indx()]++;
                    
                    auto idx = 0;
                    for (unsigned int i = 0; i < candidates.size(); ++i) {
                        if (candidates_counts[i] > 0) {
                            auto &log = candidates[i];
                            if (log.type == IT_FIRAS_LOG) {
                                for (auto j = 0; j < candidates_counts[i]; ++j) {
                                    auto rnd = log.left_idx + (rand() % (log.right_idx - log.left_idx));
                                    samples[idx] = A_ids[rnd];
                                    idx++;
                                }
                            } else if (log.type == IT_RECORDS_BY_START) {
                                for (auto j = 0; j < candidates_counts[i]; ++j) {
                                    auto rnd = log.left_idx + (rand() % (log.right_idx - log.left_idx));
                                    samples[idx] = log.node->recordsByStart[rnd].id;
                                    idx++;
                                }
                            } else if (log.type == IT_RECORDS_BY_END) {
                                for (auto j = 0; j < candidates_counts[i]; ++j) {
                                    auto rnd = log.left_idx + (rand() % (log.right_idx - log.left_idx));
                                    samples[idx] = log.node->recordsByEnd[rnd].id;
                                    idx++;
                                }
                            }
//#ifdef SPLIT_LEAF_SUBTREE
//                            else if (log.type == IT_SUBTREE_LEFT) {
//                                IntervalTreeLeafNode *leaf = static_cast<IntervalTreeLeafNode*>(log.node);
//                                
//                                for (auto j = 0; j < candidates_counts[i]; ++j)
//                                {
//                                    auto rnd = log.left_idx + (rand() % (log.right_idx - log.left_idx));
//                                    while (leaf->recordsLeftSubtree[rnd].start > qend)
//                                    {
//                                        rnd = log.left_idx + (rand() % (log.right_idx - log.left_idx));
//                                    }
//                                    samples[idx] = leaf->recordsLeftSubtree[rnd].id;
//                                    idx++;
//                                }
//                            }
//                            else if (log.type == IT_SUBTREE_RIGHT) {
//                                IntervalTreeLeafNode *leaf = static_cast<IntervalTreeLeafNode*>(log.node);
//                                
//                                for (auto j = 0; j < candidates_counts[i]; ++j)
//                                {
//                                    auto rnd = log.left_idx + (rand() % (log.right_idx - log.left_idx));
//                                    while (qstart > leaf->recordsRightSubtree[rnd].end)
//                                    {
//                                        rnd = log.left_idx + (rand() % (log.right_idx - log.left_idx));
//                                    }
//                                    samples[idx] = leaf->recordsRightSubtree[rnd].id;
//                                    idx++;
//                                }
//                            }
//#else
//                            else if (log.type == IT_SUBTREE) {
//                                IntervalTreeLeafNode *leaf = static_cast<IntervalTreeLeafNode*>(log.node);
//                                
//                                for (auto j = 0; j < candidates_counts[i]; ++j) {
//                                    auto rnd = log.left_idx + (rand() % (log.right_idx - log.left_idx));
//                                    while (qstart > leaf->recordsSubtree[rnd].end)
//                                        rnd = log.left_idx + (rand() % (log.right_idx - log.left_idx));
//                                    samples[idx] = leaf->recordsSubtree[rnd].id;
//                                    idx++;
//                                }
//                            }
//#endif
                        }
                    }
                    samplingTime = tim.stop();
                }
            }
            totalATime += aTime;
            totalStabbingTime += stabbingTime;
            totalSamplingTime += samplingTime;
        }
#ifdef WORKLOAD_COUNT
        totalResult += queryResult;
#else
        totalResult ^= queryResult;
#endif
    }
    fQ.close();

    cout << endl;
    cout << "FIRAS with Interval Tree" << endl;
    cout << "=======================" << endl;
    cout << "Input" << endl;
    cout << "  Data file                 : " << settings.dataFile << endl;
    cout << "  Num of intervals          : " << R.size() << endl;
    cout << "  Domain size               : " << (R.gend-R.gstart) << endl;
    cout << "  Avg interval extent [%]   : " << R.avgRecordExtent*100/(R.gend-R.gstart) << endl;
    cout << endl;
    cout << "Index" << endl;
    cout << "  Num of levels             : " << idxR->numLevels << endl;
    cout << "  Size [Bytes]              : " << idxR->getSize() + A_timestamps.size() * sizeof(Timestamp) + A_ids.size() * sizeof(RecordId) << endl;
    cout << "  Indexing time [secs]      : " << totalIndexTime << endl;
    cout << endl;
    cout << "Queries" << endl;
    cout << "  Query file                : " << settings.queryFile << endl;
    cout << "  Num of queries            : " << numQueries << endl;
    cout << "  Num of runs per query     : " << settings.numRuns << endl;
    cout << "  Avg num of nodes accessed : " << ((float)idxR->nodesAccessed/numQueries)/settings.numRuns << endl;
    cout << endl;
    cout << "Output" << endl;
    if (settings.sampleSize != 0) {
        cout << "  Sample size           : " << settings.sampleSize << endl;
        cout << "  Total logs length     : " << totalLogs/settings.numRuns << endl;
    }
    else {
#ifdef WORKLOAD_COUNT
        cout << "  Total result [COUNT]      : " << totalResult << endl;
#else
        cout << "  Total result [XOR]        : " << totalResult << endl;
#endif
    }
    if (settings.sampleSize > 0) {
        double totalQueryTime = totalATime + totalStabbingTime + totalSamplingTime;
        printf( "  Total querying time [secs]: %f\n", totalQueryTime/settings.numRuns);
        printf("   Total logging time [secs] : %f\n", (totalATime + totalStabbingTime)/settings.numRuns);
        printf( "  Total sampling time [secs]: %f\n", totalSamplingTime/settings.numRuns);
        printf( "  Throughput [queries/sec]  : %f\n", numQueries/(totalQueryTime/settings.numRuns));
    } else {
        double totalQueryTime = totalATime + totalStabbingTime;
        printf( "  Total querying time [secs]: %f\n", totalQueryTime/settings.numRuns);
        printf( "  Throughput [queries/sec]  : %f\n", numQueries/(totalQueryTime/settings.numRuns));
    }
    cout << endl;
    cout << "Per-Structure Statistics" << endl;
    cout << "  Array A size              : " << A_timestamps.size() * sizeof(Timestamp) + A_ids.size() * sizeof(RecordId) << endl;
    printf( "  Total Array A time [secs] : %f\n", totalATime/settings.numRuns);
    cout << "  IT size                   : " << idxR->getSize() << endl;
    printf( "  Total stabbing time [secs]: %f\n", totalStabbingTime/settings.numRuns);
    cout << endl;

    delete idxR;
    return 0;
}
