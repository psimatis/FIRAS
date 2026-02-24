#include "getopt.h"
#include "def_global.h"
#include "./containers/relation.h"
#include "./indices/hint_m.h"
#include "./indices/live_index.cpp"
#include <random>

void usage() {
    cerr << endl;
    cerr << "PROJECT" << endl;
    cerr << "       LIT: Lightning-fast In-memory Temporal Indexing (3-column format)" << endl << endl;
    cerr << "USAGE" << endl;
    cerr << "       ./query_pureLIT_3col.exec [OPTIONS] [STREAMFILE]" << endl << endl;
    cerr << "DESCRIPTION" << endl;
    cerr << "       -e" << endl;
    cerr << "              set the leaf partition extent; it is set in seconds" << endl;
    cerr << "       -b" << endl;
    cerr << "              set the type of data structure for the LIVE INDEX" << endl;
    cerr << "       -c" << endl;
    cerr << "              set the capacity constraint number for the LIVE INDEX" << endl;
    cerr << "       -s" << endl;
    cerr << "              set the sample size" << endl;     
    cerr << "       -r runs" << endl;
    cerr << "              set the number of runs per query; by default 1" << endl; 
    cerr << "EXAMPLE" << endl;
    cerr << "       ./query_pureLIT_3col.exec -e 86400 -b ENHANCEDHASHMAP -c 10000 -r 5 streams/data.txt" << endl << endl;
}

int main(int argc, char **argv) {
    Timer tim;
    Record r;
    HINT_M_Dynamic *deadIndex;
    LiveIndex *liveIndex;
    size_t totalResults = 0, numStarts = 0, numEnds = 0, numQueries = 0;
    double liveStartTime = 0, liveEndTime = 0, liveQueryTime = 0;
    double deadEndTime = 0, deadQueryTime = 0, samplingTime = 0;
    Timestamp first, second, startEndpoint;
    size_t total_interval_length = 0, total_query_length = 0;
    size_t totalLogs = 0;
    RunSettings settings;
    char c, operation;
    Timestamp leafPartitionExtent = 0;
    string typeBuffer;
    int maxCapacity = -1;
    int sampleSize = 0;

    settings.init();
    settings.method = "pureLIT_3col";
    while ((c = getopt(argc, argv, "?q:e:c:b:s:r:")) != -1) {
        switch (c) {
            case 'e':
                leafPartitionExtent = atoi(optarg);
                break;
            case 'b':
                typeBuffer = toUpperCase((char*)optarg);
                break;
            case 'c':
                maxCapacity = atoi(optarg);
                break;
            case 's':
                sampleSize = atoi(optarg);
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
    
    if (argc-optind != 1) {
        usage();
        return 1;
    }

    if (leafPartitionExtent <= 0) {
        usage();
        return 1;
    }

    settings.dataFile = argv[optind];
    
    // Initialize indices
    deadIndex = new HINT_M_Dynamic(leafPartitionExtent);
    liveIndex = new LiveIndexCapacityConstraintedICDE16(maxCapacity);

    ifstream fQ(settings.dataFile);
    if (!fQ) {
        cerr << endl << "Error - cannot open query file \"" << settings.dataFile << "\"" << endl << endl;
        return 1;
    }
    
    cout << "pureLIT_3col" << endl;
    cout << "====================" << endl << endl;
    cout << "Buffer info" << endl;
    cout << "Type                               : " << typeBuffer << endl;
    // cout << "Buffer capacity                    : " << maxCapacity << endl;
    cout << endl;
    
    cout << "Index info" << endl;

    while (fQ >> operation >> first >> second) {
        switch (operation) {
            case 'S':
                numStarts++;

                tim.start();
                liveIndex->insert(first, second);
                liveStartTime += tim.stop();
                break;
            case 'E':
                numEnds++;

                tim.start();
                startEndpoint = liveIndex->remove(first);
                liveEndTime += tim.stop();
                
                tim.start();
                deadIndex->insert(Record(first, startEndpoint, second));
                deadEndTime += tim.stop();

                total_interval_length += (second - startEndpoint);
                break;
            case 'Q':
                {
                    total_query_length += (second - first);
                    size_t queryResult = 0;

                    for (auto r = 0; r < settings.numRuns; r++) {
                        // Sampling mode
                        if (sampleSize > 0) {
                            srand(time(NULL));
                            vector<RecordId> results;
                            vector<RecordId> samples(sampleSize);
                            
                            tim.start();
                            if (second >= liveIndex->getEarliestTimestamp())
                                liveIndex->execute_pureTimeTravel(RangeQuery(numQueries, first, second), results);
                            liveQueryTime += tim.stop();
                            
                            tim.start();
                            if (first <= deadIndex->gend) 
                                deadIndex->execute_pureTimeTravel(RangeQuery(numQueries, first, second), results);
                            deadQueryTime += tim.stop();

                            totalLogs += results.size();
                            
                            tim.start();
                            if (!results.empty()) {
                                for (int i = 0; i < sampleSize; ++i) {
                                    auto rnd = rand() % results.size();
                                    samples[i] = results[rnd];
                                }
                            }
                            samplingTime += tim.stop();
                        }
                        // Non-sampling mode
                        else {
                            vector<RecordId> results;
                            queryResult = 0;
                            
                            tim.start();
                            if (second >= liveIndex->getEarliestTimestamp())
                                liveIndex->execute_pureTimeTravel(RangeQuery(numQueries, first, second), results);
                            liveQueryTime += tim.stop();
                            
                            tim.start();
                            if (first <= deadIndex->gend) {
                                Timestamp clampedSecond = min(second, deadIndex->gend);
                                deadIndex->execute_pureTimeTravel(RangeQuery(numQueries, first, clampedSecond), results);
                            }
                            deadQueryTime += tim.stop();
                            
                            // XOR the results for verification
                            for (const auto& id : results) 
                                queryResult ^= id;
                            deadQueryTime += tim.stop();
                        }
                    }
#ifdef WORKLOAD_COUNT
                    totalResults += queryResult;
#else
                    totalResults ^= queryResult;
#endif
                    numQueries++;
                }
                break;
        }
    }
    fQ.close();
    
    deadIndex->getStats();
    cout << endl;
    cout << "Stream pureLIT 3-Column" << endl;
    cout << "======================" << endl;
    cout << "Input" << endl;
    cout << "  Stream file               : " << settings.dataFile << endl;
    cout << "  Num of interval starts    : " << numStarts << endl;
    cout << "  Num of interval ends      : " << numEnds << endl;
    cout << "  Num of queries            : " << numQueries << endl;
    cout << endl;
    cout << "Data Structures" << endl;
    cout << "  Buffer type               : " << typeBuffer << endl;
    printf( "  Size [Bytes]              : %ld\n", liveIndex->getMemoryUsage() + deadIndex->getSize());
    cout << endl;
    cout << "  Num of bits               : " << deadIndex->numBits << endl;
    cout << "  Num of partitions         : " << deadIndex->numPartitions << endl;
    cout << "  Num of Originals (In)     : " << deadIndex->numOriginalsIn << endl;
    cout << "  Num of Originals (Aft)    : " << deadIndex->numOriginalsAft << endl;
    cout << "  Num of replicas (In)      : " << deadIndex->numReplicasIn << endl;
    cout << "  Num of replicas (Aft)     : " << deadIndex->numReplicasAft << endl;
    cout << endl;
    cout << "Performance" << endl;
    cout << "  Num of runs per query     : " << settings.numRuns << endl;
    cout << "  Total update time [secs]  : " << liveStartTime + liveEndTime + deadEndTime << endl;
    if (sampleSize > 0) {
        printf( "  Total querying time [secs]: %f\n", (liveQueryTime + deadQueryTime + samplingTime)/settings.numRuns);
        printf( "  Total logging time [secs] : %f\n", (liveQueryTime + deadQueryTime)/settings.numRuns);
        printf( "  Total sampling time [secs]: %f\n", samplingTime/settings.numRuns);
        printf( "  Throughput [queries/sec]  : %f\n", numQueries/((liveQueryTime + deadQueryTime + samplingTime)/settings.numRuns));
    } else {
        printf( "  Total querying time [secs]: %f\n", (liveQueryTime + deadQueryTime)/settings.numRuns);
        printf( "  Throughput [queries/sec]  : %f\n", numQueries/((liveQueryTime + deadQueryTime)/settings.numRuns));
    }
    cout << endl;
    cout << "Per-Structure Statistics" << endl;
    printf( "  Live insert time [secs]   : %f\n", liveStartTime);
    printf( "  Live remove time [secs]   : %f\n", liveEndTime);
    printf( "  Live query time [secs]    : %f\n", liveQueryTime/settings.numRuns);
    printf( "  Dead insert time [secs]   : %f\n", deadEndTime);
    printf( "  Dead query time [secs]    : %f\n", deadQueryTime/settings.numRuns);
    cout << endl;
    cout << "Output" << endl;
    if (sampleSize != 0) {
        cout << "  Sample size               : " << sampleSize << endl;
        cout << "  Total logs length         : " << totalLogs/settings.numRuns << endl;
    } else {
#ifdef WORKLOAD_COUNT
        cout << "  Total result [COUNT]      : " << totalResults << endl;
#else
        cout << "  Total result [XOR]        : " << totalResults << endl;
#endif
    }
    cout << endl;
        
    delete deadIndex;
    delete liveIndex;
    
    return 0;
}
