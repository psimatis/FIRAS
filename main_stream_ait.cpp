#include "getopt.h"
#include "def_global.h"
#include "./containers/relation.h"
#include "./indices/intervaltree.h"
#include "./indices/augmented_interval_tree.h"
#include "./utils/weighted_sampling.hpp"
#include <set>
#include <map>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <random>
#define MAX_ICDE16_CAPACITY 500000
#include "./indices/LIT/containers/buffer.cpp"
#include "./indices/LIT/indices/live_index.cpp"


void usage() {
    cerr << endl;
    cerr << "PROJECT" << endl;
    cerr << "       Streaming interval indexing with Map + Unordered Map + AIT" << endl << endl;
    cerr << "USAGE" << endl;
    cerr << "       ./stream_ait.exec [OPTION]... [STREAM_FILE]" << endl << endl;
    cerr << "DESCRIPTION" << endl;
    cerr << "       -s size" << endl;
    cerr << "              set the sample size" << endl;
    cerr << "       -r runs" << endl;
    cerr << "              set the number of runs per query; by default 1" << endl;
    cerr << "EXAMPLES" << endl;
    cerr << "       ./stream_ait.exec -s 10 -r 5 stream_data.txt" << endl;
}

int main(int argc, char **argv) {
    Timer timer;
    
    LiveIndex *liveIndex;
    liveIndex = new LiveIndexCapacityConstraintedICDE16(10000);
    AugmentedIntervalTree *ait = new AugmentedIntervalTree(); 
    
    size_t totalResults = 0;
    size_t numQueries = 0, numStarts = 0, numEnds = 0;
    double liveInsertTime = 0, aitInsertTime = 0;
    double liveRemoveTime = 0;
    double liveQueryTime = 0, aitQueryTime = 0, samplingTime = 0;
    size_t totalLogs = 0;

    RunSettings settings;
    char c;
    
    settings.init();
    while ((c = getopt(argc, argv, "vs:r:")) != -1) {
        switch (c) {
            case 's':
                settings.sampleSize = atoi(optarg);
                break;
            case 'r':
                settings.numRuns = atoi(optarg);
                break;
            default:
                cerr << "Wrong option! Use -h for help." << endl;
                return 1;
        }
    }
    
    if (argc - optind != 1) {
        usage();
        return 1;
    }
    
    string streamFile = argv[optind];
    ifstream fStream(streamFile.c_str());
    if (!fStream) {
        cerr << "Cannot open stream file " << streamFile << endl;
        return 1;
    }
    
    string line;    
    cout << "Processing stream..." << endl;
    while (getline(fStream, line)) {               
        istringstream iss(line);
        char op;
        iss >> op;

        // Start of interval: S id start_time
        if (op == 'S') {    
            RecordId id;
            Timestamp start;
            iss >> id >> start;
            numStarts++;
            
            timer.start();
            liveIndex->insert(id, start);
            liveInsertTime += timer.stop();

        } 
        // End of interval: E id end_time
        else if (op == 'E') {
            RecordId id;
            Timestamp end;
            iss >> id >> end;
            numEnds++;
            
            Timestamp start_time = 0;
            timer.start();
            start_time = liveIndex->remove(id);
            liveRemoveTime += timer.stop();
                
            timer.start();
            ait->insert(Record(id, start_time, end));
            aitInsertTime += timer.stop();
        }
        // Query: Q start_time end_time 
        else {
            Timestamp qstart, qend;
            iss >> qstart >> qend;
            numQueries++;
            size_t queryResult = 0;

            for (auto r = 0; r < settings.numRuns; r++) {
                // Sampling mode
                if (settings.sampleSize > 0) {
                    // Step 1: Get candidates
                    srand(time(NULL));
                    vector<AugmentedIntervalTree::AIT_CandidateLog> candidates;

                    timer.start();
                    if (qend >= liveIndex->getEarliestTimestamp())
                        liveIndex->execute_pureTimeTravel_candidates(RangeQuery(numQueries, qstart, qend), candidates);
                    liveQueryTime += timer.stop();

                    timer.start();
                    if (ait->root != nullptr)
                        ait->execute_gOverlaps(RangeQuery(numQueries, qstart, qend), candidates);
                    aitQueryTime += timer.stop();
                    
                    for (const auto& log : candidates)
                        totalLogs += log.right_idx - log.left_idx;
                    
                    // Step 2: Sample
                    if (!candidates.empty()) {
                        vector<unsigned int> arr;
                        vector<RecordId> samples(settings.sampleSize);
                        vector<unsigned int> candidates_counts(candidates.size(), 0);
                        
                        timer.start();
                        for (unsigned int i = 0; i < candidates.size(); ++i)
                            arr.push_back(candidates[i].right_idx - candidates[i].left_idx);
                        alias a(arr);
                        for (unsigned int i = 0; i < settings.sampleSize; ++i)
                            candidates_counts[a.get_indx()]++;

                        auto idx = 0;
                        for (unsigned int i = 0; i < candidates.size(); ++i) {
                            if (candidates_counts[i] > 0) {
                                auto &log = candidates[i];
                                if (log.type == LIVE_INDEX) {
                                    // Regular buffer - sample from values array
                                    pair<RecordId, Timestamp>* values = (pair<RecordId, Timestamp>*)log.live_data;
                                    for (auto j = 0; j < candidates_counts[i]; ++j) {
                                        auto rnd = rand() % log.right_idx;
                                        samples[idx] = values[rnd].first;
                                        idx++;
                                    }
                                } else if (log.type == LIVE_INDEX_LAST_BUFFER) {
                                    // Last buffer - sample from vector
                                    vector<RecordId>* candidateVec = (vector<RecordId>*)log.live_data;
                                    for (auto j = 0; j < candidates_counts[i]; ++j) {
                                        auto rnd = rand() % candidateVec->size();
                                        samples[idx] = (*candidateVec)[rnd];
                                        idx++;
                                    }
                                } else if (log.type == RECORDS_BY_START) {
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
                        samplingTime += timer.stop();
                    }

                }
                // Range query mode
                else {
                    vector<RecordId> results;
                    queryResult = 0;
                    
                    timer.start();
                    if (qend >= liveIndex->getEarliestTimestamp())
                    liveIndex->execute_pureTimeTravel(RangeQuery(numQueries, qstart, qend), results);
                    liveQueryTime += timer.stop();
                    
                    timer.start();
                    if (ait->root != nullptr)
                        ait->execute_gOverlaps(RangeQuery(numQueries, qstart, qend), results);
                    aitQueryTime += timer.stop();
                    
                    // XOR the results for verification
                    for (const auto& id : results) 
                        queryResult ^= id;
                }
            }
#ifdef WORKLOAD_COUNT
                totalResults += queryResult;
#else
                totalResults ^= queryResult;
#endif
        }
    }    
    fStream.close();
    ait->getStats();
    
    cout << endl;
    cout << "Stream AIT" << endl;
    cout << "==========" << endl;
    cout << "Input" << endl;
    cout << "  Stream file               : " << streamFile << endl;
    cout << "  Num of interval starts    : " << numStarts << endl;
    cout << "  Num of interval ends      : " << numEnds << endl;
    cout << "  Num of queries            : " << numQueries << endl;
    cout << endl;
    cout << "Data Structures" << endl;
    cout << "  AIT levels                : " << ait->numLevels << endl;
    cout << "  Size [Bytes]              : " << liveIndex->getMemoryUsage() + ait->getSize() << endl;
    cout << "  Number of nodes           : " << ait->numNodes << endl;
    cout << endl;
    cout << "Performance" << endl;
    cout << "  Num of runs per query     : " << settings.numRuns << endl;
    cout << "  Total update time [secs]  : " << liveInsertTime + liveRemoveTime + aitInsertTime << endl;
    if (settings.sampleSize > 0) {
        cout << "  Total querying time [secs]: " << (liveQueryTime + aitQueryTime + samplingTime)/settings.numRuns << endl;
        cout << "  Total logging time [secs] : " << (liveQueryTime + aitQueryTime)/settings.numRuns << endl;
        cout << "  Total sampling time [secs]: " << samplingTime/settings.numRuns << endl;
        cout << "  Throughput [queries/sec]  : " << numQueries/((liveQueryTime + aitQueryTime + samplingTime)/settings.numRuns) << endl;
    } else {
        cout << "  Total querying time [secs]: " << (liveQueryTime + aitQueryTime)/settings.numRuns << endl;
        cout << "  Throughput [queries/sec]  : " << numQueries/((liveQueryTime + aitQueryTime)/settings.numRuns) << endl;
    }
    cout << endl;
    cout << "Per-Structure Statistics" << endl;
    cout << "  Live insert time [secs]: " << liveInsertTime << endl;
    cout << "  Live remove time [secs]: " << liveRemoveTime << endl;
    cout << "  Live query time [secs] : " << liveQueryTime/settings.numRuns << endl;
    cout << "  AIT insert time [secs] : " << aitInsertTime << endl;
    cout << "  AIT query time [secs]  : " << aitQueryTime/settings.numRuns << endl;
    cout << endl;
    cout << "Output" << endl;
    if (settings.sampleSize != 0) {
        cout << "  Sample size               : " << settings.sampleSize << endl;
        cout << "  Total logs length     : " << totalLogs/settings.numRuns << endl;
    } else {
#ifdef WORKLOAD_COUNT
        cout << "  Total result [COUNT]      : " << totalResults << endl;
#else
        cout << "  Total result [XOR]        : " << totalResults << endl;
#endif
    }
    cout << endl;

    delete ait;
    return 0;
}
