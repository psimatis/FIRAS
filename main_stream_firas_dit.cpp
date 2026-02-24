#include "getopt.h"
#include "def_global.h"
#include "./containers/relation.h"
#include "./indices/dit.h"
#include "./utils/weighted_sampling.hpp"
#define MAX_ICDE16_CAPACITY 500000
#include "./indices/LIT/containers/buffer.cpp"
#include "./indices/LIT/indices/live_index.cpp"
#include <set>
#include <map>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <random>

void usage() {
    cerr << endl;
    cerr << "PROJECT" << endl;
    cerr << "       Streaming interval indexing with Live Index + FIRAS" << endl << endl;
    cerr << "USAGE" << endl;
    cerr << "       ./stream_firas_fit.exec [OPTION]... [STREAM_FILE]" << endl << endl;
    cerr << "DESCRIPTION" << endl;
    cerr << "       -b threshold" << endl;
    cerr << "              set the buffer threshold (default: 128)" << endl;
    cerr << "       -s size" << endl;
    cerr << "              set the sample size" << endl;
    cerr << "       -r runs" << endl;
    cerr << "              set the number of runs per query; by default 1" << endl;
    cerr << "EXAMPLES" << endl;
    cerr << "       ./stream_firas_fit.exec -s 10 -r 5 stream_data.txt" << endl;
}


int main(int argc, char **argv) {
    Timer timer;
       
    size_t totalResults = 0;
    size_t numQueries = 0, numStarts = 0, numEnds = 0;
    double liveInsertTime = 0,  aInsertTime = 0, ditInsertTime = 0;
    double liveRemoveTime = 0;
    double liveQueryTime = 0, aQueryTime = 0, ditQueryTime = 0, samplingTime = 0;
    size_t totalLogs = 0;

    RunSettings settings;
    char c;
    size_t bufferThreshold = 512;
    
    settings.init();
    while ((c = getopt(argc, argv, "vs:r:b:")) != -1) {
        switch (c) {
            case 'b':
                bufferThreshold = atoi(optarg);
                break;
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
    
    LiveIndex *liveIndex;
    liveIndex = new LiveIndexCapacityConstraintedICDE16(10000);
    vector<Timestamp> A_timestamps;
    vector<RecordId> A_ids;
    DIT *dit = new DIT(bufferThreshold);
    
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
    

    auto comp = [](const Timestamp& a, const Timestamp& b) {return a < b;};
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
            numEnds++;
            
            iss >> id >> end;
            
            Timestamp start_time = 0;
            timer.start();
            start_time = liveIndex->remove(id);
            liveRemoveTime += timer.stop();

            timer.start();
            Record r(id, start_time, end);
            A_timestamps.push_back(end);
            A_ids.push_back(id);
            aInsertTime += timer.stop();
            
            timer.start();
            dit->insert(r);
            ditInsertTime += timer.stop();                
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
                    vector<DIT::DIT_CandidateLog> candidates;
                    // candidates.reserve(1000);

                    timer.start();
                    if (qend >= liveIndex->getEarliestTimestamp())
                        liveIndex->execute_pureTimeTravel_candidates(RangeQuery(numQueries, qstart, qend), candidates);
                    liveQueryTime += timer.stop();

                    timer.start();
                    auto low = lower_bound(A_timestamps.begin(), A_timestamps.end(), qstart, comp);
                    auto high = lower_bound(low, A_timestamps.end(), qend, comp);
                    if (high > low)
                        candidates.push_back(DIT::DIT_CandidateLog((DITNode*)nullptr, DIT_FIRAS_LOG, low - A_timestamps.begin(), high - A_timestamps.begin()));
                    aQueryTime += timer.stop();

                    timer.start();
                    if (dit->root != nullptr)
                        dit->execute_Stabbing(RangeQuery(numQueries, qend, qend), candidates);
                    ditQueryTime += timer.stop();

                    for (const auto& log : candidates)
                        totalLogs += log.right_idx - log.left_idx;

                    // Step 2: Sampling
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
                                } else if (log.type == DIT_FIRAS_LOG) {
                                    for (auto j = 0; j < candidates_counts[i]; ++j) {
                                        auto rnd = log.left_idx + (rand() % (log.right_idx - log.left_idx));
                                        samples[idx] = A_ids[rnd];
                                        idx++;
                                    }
                                } else if (log.type == DIT_MAIN_SORTED_BY_START) {
                                    for (auto j = 0; j < candidates_counts[i]; ++j) {
                                        auto rnd = log.left_idx + (rand() % (log.right_idx - log.left_idx));
                                        samples[idx] = log.dit_node->mainSortedByStart[rnd].id;
                                        idx++;
                                    }
                                } else if (log.type == DIT_BUFFER_BY_START) {
                                    vector<RecordId>* buffer_candidates = (vector<RecordId>*)log.live_data;
                                    for (auto j = 0; j < candidates_counts[i]; ++j) {
                                        auto rnd = rand() % buffer_candidates->size();
                                        samples[idx] = (*buffer_candidates)[rnd];
                                        idx++;
                                    }
                                } else if (log.type == DIT_RECORDS_BY_END) {
                                    for (auto j = 0; j < candidates_counts[i]; ++j) {
                                        auto rnd = log.left_idx + (rand() % (log.right_idx - log.left_idx));
                                        auto dit_node = log.dit_node;
                                        samples[idx] = dit_node->recordsByEnd[rnd].id;
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
                    auto low = lower_bound(A_timestamps.begin(), A_timestamps.end(), qstart, comp);
                    auto high = lower_bound(low, A_timestamps.end(), qend, comp);
                    auto ilow = A_ids.begin() + (low-A_timestamps.begin());
                    auto ihigh = A_ids.begin() + (high-A_timestamps.begin());
                    for (auto it = ilow; it != ihigh; ++it)
                        results.push_back(*it);
                    aQueryTime += timer.stop();
                    
                    if (dit->root != nullptr) {
                        timer.start();
                        dit->execute_Stabbing(RangeQuery(numQueries, qend, qend), results);
                        ditQueryTime += timer.stop();                
                    }
                    
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

    dit->getStats();
    
    cout << endl;
    cout << "Stream FIRAS with DIT" << endl;
    cout << "====================" << endl;
    cout << "Input" << endl;
    cout << "  Stream file               : " << streamFile << endl;
    cout << "  Num of interval starts    : " << numStarts << endl;
    cout << "  Num of interval ends      : " << numEnds << endl;
    cout << "  Num of queries            : " << numQueries << endl;
    cout << endl;
    cout << "Data Structures" << endl;
    cout << "  FIRAS Array size          : " << A_timestamps.size() << " completed intervals" << endl;
    cout << "  DIT levels                : " << dit->numLevels << endl;
    cout << "  Size [Bytes]              : " << liveIndex->getMemoryUsage() + A_timestamps.size() * sizeof(Timestamp) + A_ids.size() * sizeof(RecordId) + dit->getSize() << endl;
    cout << "  Number of nodes           : " << dit->numNodes << endl;
    cout << "  Number of empty nodes     : " << dit->numEmptyNodes << endl;
    cout << "  Average data per node     : " << dit->avgDataPerNode << endl;
    cout << endl;
    cout << "Performance" << endl;
    cout << "  Num of runs per query     : " << settings.numRuns << endl;
    cout << "  Total update time [secs]  : " << liveInsertTime + liveRemoveTime + aInsertTime + ditInsertTime << endl;
    if (settings.sampleSize > 0) {
        cout << "  Total querying time [secs]: " << (liveQueryTime + aQueryTime + ditQueryTime + samplingTime)/settings.numRuns << endl;
        cout << "  Total logging time [secs] : " << (liveQueryTime + aQueryTime + ditQueryTime)/settings.numRuns << endl;
        cout << "  Total sampling time [secs]: " << samplingTime/settings.numRuns << endl;
        cout << "  Throughput [queries/sec]  : " << numQueries/((liveQueryTime + aQueryTime + ditQueryTime + samplingTime)/settings.numRuns) << endl;
    } else {
        cout << "  Total querying time [secs]: " << (liveQueryTime + aQueryTime + ditQueryTime)/settings.numRuns << endl;
        cout << "  Throughput [queries/sec]  : " << numQueries/((liveQueryTime + aQueryTime + ditQueryTime)/settings.numRuns) << endl;
    }
    cout << endl;
    cout << "Per-Structure Statistics" << endl;
    cout << "  Live insert time [secs]: " << liveInsertTime << endl;
    cout << "  Live remove time [secs]: " << liveRemoveTime << endl;
    cout << "  Live query time [secs] : " << liveQueryTime/settings.numRuns << endl;
    cout << "  Array A insert time [secs]: " << aInsertTime << endl;
    cout << "  Array A query time [secs] : " << aQueryTime/settings.numRuns << endl;
    cout << "  DIT insert time [secs]    : " << ditInsertTime << endl;
    cout << "  DIT query time [secs]     : " << ditQueryTime/settings.numRuns << endl;
    cout << endl;
    cout << "Output" << endl;
    if (settings.sampleSize != 0) {
        cout << "  Sample size               : " << settings.sampleSize << endl;
        cout << "  Total logs length         : " << totalLogs/settings.numRuns << endl;
    } else {
#ifdef WORKLOAD_COUNT
        cout << "  Total result [COUNT]      : " << totalResults << endl;
#else
        cout << "  Total result [XOR]        : " << totalResults << endl;
#endif
    }
    cout << endl;
    
    delete dit;
    delete liveIndex;
    return 0;
}
