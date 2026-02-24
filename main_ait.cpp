#include "def_global.h"
#include "./containers/relation.h"
#include "./indices/augmented_interval_tree.h"
#include "./utils/weighted_sampling.hpp"


void usage() {
    cerr << endl;
    cerr << "PROJECT" << endl;
    cerr << "       Indexing interval data" << endl;
    cerr << endl;
    cerr << "USAGE" << endl;
    cerr << "       ./query_ait.exec [OPTION]... [DATA] [QUERIES]" << endl;
    cerr << endl;
    cerr << "DESCRIPTION" << endl;
    cerr << endl;
    cerr << "       -q predicate" << endl;
    cerr << "              set predicate type: \"STABBING\" or \"GOVERLAPS\"" << endl;
    cerr << endl;
    cerr << "       -s size" << endl;
    cerr << "              set the sample size" << endl;
    cerr << endl;
    cerr << "       -r runs" << endl;
    cerr << "              set the number of runs per query; by default 1" << endl;
    cerr << endl;
    cerr << "       -v" << endl;
    cerr << "              activate verbose mode; print the trace for every query; otherwise only the final report" << endl;
    cerr << endl;
    cerr << "EXAMPLES" << endl;
    cerr << "       ./query_ait.exec -q STABBING inputs/AARHUS-BOOKS_2013_LIT_Sven.txt queries/AARHUS-BOOKS_2013_LIT_qe1%_qn10000.txt" << endl;
    cerr << "       ./query_ait.exec -q GOVERLAPS -s 10 inputs/AARHUS-BOOKS_2013_LIT_Sven.txt queries/AARHUS-BOOKS_2013_LIT_qe1%_qn10000.txt" << endl;
    cerr << "       ./query_ait.exec -q GOVERLAPS -r 5 -v inputs/AARHUS-BOOKS_2013_LIT_Sven.txt queries/AARHUS-BOOKS_2013_LIT_qe1%_qn10000.txt" << endl;
    cerr << endl;
}


int main(int argc, char **argv) {
    Timer tim;
    Relation R;
    AugmentedIntervalTree *idxR;
    size_t totalResult = 0, queryResult = 0, numQueries = 0;
    double totalIndexTime = 0, totalQueryTime = 0, queryTime = 0;
    double totalSampleTime = 0, sampleTime = 0;
    size_t totalLogs = 0;
    Timestamp qstart, qend;
    RunSettings settings;
    char c;
    string strPredicate = "";
    
    // Parse command line input
    settings.init();
    settings.method = "ait";
    while ((c = getopt(argc, argv, "q:s:r:v")) != -1) {
        switch (c) {
            case 'q':
                strPredicate = toUpperCase((char*)optarg);
                break;
            case 's':
                settings.sampleSize = atoi(optarg);
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

    // Build index
    tim.start();
    R.sortByStart();
    idxR = new AugmentedIntervalTree(R);
    totalIndexTime = tim.stop();
    idxR->getStats();

    // Execute queries
    while (fQ >> qstart >> qend) {
        numQueries++;
        
        for (auto r = 0; r < settings.numRuns; r++) {
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
                vector<AugmentedIntervalTree::AIT_CandidateLog> candidates;
                // candidates.reserve(idxR->numNodes / 100);

                tim.start();
                idxR->execute_gOverlaps(RangeQuery(numQueries, qstart, qend), candidates);
                queryTime = tim.stop();

                for (const auto& log : candidates)
                    totalLogs += log.right_idx - log.left_idx;

                // Step 2: Sample
                sampleTime = 0;
                if (!candidates.empty()) {
                    srand(time(NULL));
                    vector<unsigned int> arr;
                    vector<RecordId> samples(settings.sampleSize);
                    vector<unsigned int> candidates_counts(candidates.size(), 0);

                    tim.start();
                    for (unsigned int i = 0; i < candidates.size(); ++i)
                        arr.push_back(candidates[i].right_idx - candidates[i].left_idx);
                    alias a(arr);
                    for (unsigned int i = 0; i < settings.sampleSize; ++i)
                        candidates_counts[a.get_indx()]++;

                    auto idx = 0;
                    for (unsigned int i = 0; i < candidates.size(); ++i) {
                        if (candidates_counts[i] > 0) {
                            auto &log = candidates[i];
                            if (log.type == RECORDS_BY_START) {
                                for (auto j = 0; j < candidates_counts[i]; ++j) {
                                    auto rnd = log.left_idx + (rand() % (log.right_idx - log.left_idx));
                                    samples[idx] = log.node->recordsByStart[rnd].id;
                                    idx++;
                                }
                            } else if (log.type == RECORDS_BY_END) {
                                for (auto j = 0; j < candidates_counts[i]; ++j) {
                                    auto rnd = log.left_idx + (rand() % (log.right_idx - log.left_idx));
                                    samples[idx] = log.node->recordsByEnd[rnd].id;
                                    idx++;
                                }
                            } else if (log.type == AUGMENTED_LIST) {
                                for (auto j = 0; j < candidates_counts[i]; ++j) {
                                    auto rnd = log.left_idx + (rand() % (log.right_idx - log.left_idx));
                                    samples[idx] = log.node->al[rnd].first;
                                    idx++;
                                }
                            }
                        }
                    }
                    sampleTime = tim.stop();
                }
            }
            totalQueryTime += queryTime;
            totalSampleTime += sampleTime;
        }
#ifdef WORKLOAD_COUNT
        totalResult += queryResult;
#else
        totalResult ^= queryResult;
#endif
    }
    fQ.close();
    
    cout << endl;
    cout << "Augmented Interval Tree (AIT)" << endl;
    cout << "=============================" << endl;
    cout << "Input" << endl;
    cout << "  Data file                 : " << settings.dataFile << endl;
    cout << "  Num of intervals          : " << R.size() << endl;
    cout << "  Domain size               : " << (R.gend-R.gstart) << endl;
    cout << "  Height                    : " << idxR->getHeight() << endl;
    cout << endl;
    cout << "Index" << endl;
    cout << "  Num of levels             : " << idxR->numLevels << endl;
    cout << "  Size [Bytes]              : " << idxR->getSize() << endl;
    cout << "  Indexing time [secs]      : " << totalIndexTime << endl;
    cout << "  Num of nodes              : " << idxR->numNodes << endl;
    cout << "  Num of intervals          : " << idxR->intervals << endl;
    cout << endl;
    cout << "Queries" << endl;
    cout << "  Query file                : " << settings.queryFile << endl;
    cout << "  Predicate type            : " << strPredicate << endl;
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
        cout << "  Total querying time [secs]: " << (totalQueryTime + totalSampleTime)/settings.numRuns << endl;
        cout << "  Total logging time [secs] : " << totalQueryTime/settings.numRuns << endl;
        cout << "  Total sampling time [secs]: " << totalSampleTime/settings.numRuns << endl;
        cout << "  Throughput [queries/sec]  : " << numQueries/((totalQueryTime + totalSampleTime)/settings.numRuns) << endl;
    } else {
        cout << "  Total querying time [secs]: " << totalQueryTime/settings.numRuns << endl;
        cout << "  Throughput [queries/sec]  : " << numQueries/(totalQueryTime/settings.numRuns) << endl;
    }
    cout << endl;

    delete idxR;
    return 0;
}
