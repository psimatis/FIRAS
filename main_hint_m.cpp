#include "getopt.h"
#include "def_global.h"
#include "./containers/relation.h"
#include "./indices/hint_m.h"


void usage() {
    cerr << endl;
    cerr << "PROJECT" << endl;
    cerr << "       Indexing interval data" << endl << endl;
    cerr << "USAGE" << endl;
    cerr << "       ./query_hint_m.exec [OPTION]... [DATA] [QUERIES]" << endl << endl;
    cerr << "DESCRIPTION" << endl;
    cerr << "       -s size" << endl;
    cerr << "              set the sample size" << endl;
    cerr << "       -m bits" << endl;
    cerr << "              set the number of bits" << endl;
    cerr << "       -t" << endl;
    cerr << "              evaluate query traversing the hierarchy in a top-down fashion, currently supported only by base HINT^m; by default the bottom-up strategy is used" << endl;
    cerr << "       -r runs" << endl;
    cerr << "              set the number of runs per query; by default 1" << endl << endl;
    cerr << "EXAMPLES" << endl;
    cerr << "       ./query_hint_m.exec -m 10 -q STABBING -r 10 inputs/AARHUS-BOOKS_2013_LIT_Sven.txt queries/AARHUS-BOOKS_2013_LIT_qe1%_qn10000.txt" << endl;
    cerr << "       ./query_hint_m.exec -m 10 -q gOVERLAPS -r 10 inputs/AARHUS-BOOKS_2013_LIT_Sven.txt queries/AARHUS-BOOKS_2013_LIT_qe1%_qn10000.txt" << endl;
    cerr << "       ./query_hint_m.exec -m 10 -q gOVERLAPS -v inputs/AARHUS-BOOKS_2013_LIT_Sven.txt queries/AARHUS-BOOKS_2013_LIT_qe1%_qn10000.txt" << endl;
    cerr << "       ./query_hint_m.exec -m 10 -q gOVERLAPS -s 100 inputs/AARHUS-BOOKS_2013_LIT_Sven.txt queries/AARHUS-BOOKS_2013_LIT_qe1%_qn10000.txt" << endl;
}


int main(int argc, char **argv) {
    Timer tim;
    Relation R;
    HINT_M_SubsSort_SS_CM_NoVLs *idxR;
    size_t totalResult = 0, queryResult = 0, numQueries = 0;
    double totalIndexTime = 0, totalQueryTime = 0, queryTime = 0, totalSamplingTime = 0, samplingTime = 0;
    Timestamp qstart, qend;
    size_t totalLogs = 0;
    RunSettings settings;
    char c;
    string strOptimizations = "SUBS+SORT+SS+CM-NOVLS";

    settings.init();
    settings.method = "hint_m";
    while ((c = getopt(argc, argv, "?hvq:s:m:r:")) != -1) {
        switch (c) {               
            case 's':
                settings.sampleSize = atoi(optarg);
                queryResult = settings.sampleSize;
                break;
            case 'm':
                settings.numBits = atoi(optarg);
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
    if (!checkOptimizations(strOptimizations, settings)) {
        cerr << endl << "Error - unknown optimizations type \"" << strOptimizations << "\"" << endl << endl;
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
    if (settings.numBits == 0)
        settings.numBits = determineOptimalNumBitsForHINT_M(R, 0.1);    // Use 0.1% of the domain as the query extent

    tim.start();
    idxR = new HINT_M_SubsSort_SS_CM_NoVLs(R, settings.numBits, settings.maxBits);
    totalIndexTime = tim.stop();

    // Execute queries
    size_t sumQ = 0;
    if (settings.verbose) {
        if (settings.sampleSize == 0)
            cout << "Query\tPredicate\tMethod\tBits\tOptimizations\tResult\tTime" << endl;
        else
            cout << "Query\tPredicate\tMethod\tBits\tOptimizations\tSample size\tTime" << endl;
    }
    while (fQ >> qstart >> qend) {
        sumQ += qend-qstart;
        numQueries++;
        
        for (auto r = 0; r < settings.numRuns; r++) {
            if (settings.sampleSize == 0) {
                vector<RecordId> results;
                
                tim.start();
                idxR->executeBottomUp_gOverlaps(RangeQuery(numQueries, qstart, qend), results);
                queryTime = tim.stop();
                
                // XOR the results for verification
                queryResult = 0;
                for (const auto& id : results) 
                    queryResult ^= id;
            }
            else {
                vector<RecordId> results;
                vector<RecordId> samples(settings.sampleSize);
                
                // Step 1: Query                      
                tim.start();
                idxR->executeBottomUp_gOverlaps(RangeQuery(numQueries, qstart, qend), results);
                queryTime = tim.stop();
                
                totalLogs += results.size();

                // Step 2: Sample                          
                tim.start();
                if (!results.empty()) {
                    for (int i = 0; i < settings.sampleSize; ++i) {
                        auto rnd = rand() % results.size();
                        samples[i] = results[rnd];
                    }
                }
                samplingTime = tim.stop();
            }
            totalQueryTime += queryTime;
            totalSamplingTime += samplingTime;
        }
#ifdef WORKLOAD_COUNT
        totalResult += queryResult;
#else
        totalResult ^= queryResult;
#endif
    }
    fQ.close();
    
    idxR->getStats();
    cout << endl;
    cout << "HINT^m" << endl;
    cout << "======" << endl;
    cout << "Input" << endl;
    cout << "  Data file                 : " << settings.dataFile << endl;
    cout << "  Num of intervals          : " << R.size() << endl;
    cout << "  Domain size               : " << (R.gend-R.gstart) << endl;
    cout << "  Avg interval extent [%]   : "; printf("%f\n", R.avgRecordExtent*100/(R.gend-R.gstart));
    cout << endl;
    cout << endl;
    cout << "Index" << endl;
    cout << "  Optimizations             : " << ((settings.typeOptimizations == HINT_M_OPTIMIZATIONS_NO)? "no": strOptimizations) << endl;
    cout << "  Num of bits               : " << settings.numBits << endl;
    cout << "  Max bits                  : " << settings.maxBits << endl;
    cout << "  Num of partitions         : " << idxR->numPartitions << endl;
    if (settings.typeOptimizations == HINT_M_OPTIMIZATIONS_NO) {
        cout << "  Num of Originals          : " << idxR->numOriginals << endl;
        cout << "  Num of replicas           : " << idxR->numReplicas << endl;
    }
    else {
        cout << "  Num of Originals (In)     : " << idxR->numOriginalsIn << endl;
        cout << "  Num of Originals (Aft)    : " << idxR->numOriginalsAft << endl;
        cout << "  Num of replicas (In)      : " << idxR->numReplicasIn << endl;
        cout << "  Num of replicas (Aft)     : " << idxR->numReplicasAft << endl;
    }
    cout << "  Num of empty partitions   : " << idxR->numEmptyPartitions << endl;
    printf( "  Avg partition size        : %f\n", idxR->avgPartitionSize);
    cout << "  Size [Bytes]              : " << idxR->getSize() << endl;
    printf( "  Indexing time [secs]      : %f\n", totalIndexTime);
    cout << endl;
    cout << "Queries" << endl;
    cout << "  Query file                : " << settings.queryFile << endl;
    cout << "  Num of queries            : " << numQueries << endl;
    cout << "  Avg query extent [%]      : "; printf("%f\n", (((float)sumQ/numQueries)*100)/(R.gend-R.gstart));
    cout << "  Num of runs per query     : " << settings.numRuns << endl;
    cout << endl;
    cout << "Output" << endl;
    if (settings.sampleSize != 0) {
        cout << "  Sample size               : " << settings.sampleSize << endl;
        cout << "  Total logs length         : " << totalLogs/settings.numRuns << endl;
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
