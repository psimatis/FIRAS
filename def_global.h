#pragma once
#ifndef _GLOBAL_DEF_H_
#define _GLOBAL_DEF_H_

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <unistd.h>
#include <tuple>
#include <stack>
using namespace std;

// Comment out the following for XOR workload
// #define WORKLOAD_COUNT

// Basic predicates of Allen's algebra
#define PREDICATE_EQUALS     1
#define PREDICATE_STARTS     2
#define PREDICATE_STARTED    3
#define PREDICATE_FINISHES   4
#define PREDICATE_FINISHED   5
#define PREDICATE_MEETS      6
#define PREDICATE_MET        7
#define PREDICATE_OVERLAPS   8
#define PREDICATE_OVERLAPPED 9
#define PREDICATE_CONTAINS   10
#define PREDICATE_CONTAINED  11
#define PREDICATE_PRECEDES   12
#define PREDICATE_PRECEDED   13
#define PREDICATE_STABBING   14

// Generalized predicates, ACM SIGMOD'22 gOverlaps
#define PREDICATE_GOVERLAPS  15


#define HINT_OPTIMIZATIONS_NO          0
#define HINT_OPTIMIZATIONS_SS          1

#define HINT_M_OPTIMIZATIONS_NO                    0
#define HINT_M_OPTIMIZATIONS_SUBS                  1
#define HINT_M_OPTIMIZATIONS_SUBS_SORT             2
#define HINT_M_OPTIMIZATIONS_SUBS_SOPT             3
#define HINT_M_OPTIMIZATIONS_SUBS_SORT_CM          4
#define HINT_M_OPTIMIZATIONS_SUBS_SORT_SS_CM       5
#define HINT_M_OPTIMIZATIONS_SUBS_SORT_SS_CM_NOVLS 6
#define HINT_M_OPTIMIZATIONS_SUBS_SORT_SOPT        7
#define HINT_M_OPTIMIZATIONS_SUBS_SORT_SOPT_SS     8
#define HINT_M_OPTIMIZATIONS_SUBS_SORT_SOPT_CM     9
#define HINT_M_OPTIMIZATIONS_ALL                   10

#define LOG_ORIGINALS_IN      0
#define LOG_ORIGINALS_IN_COPY 1
#define LOG_ORIGINALS_AFT     2
#define LOG_REPLICAS_IN       3
#define LOG_REPLICAS_AFT      4
#define LOG_ORIGINALS_IN_IT   5
#define LOG_ORIGINALS_IN_BOTH_COMPS_LAST   6
#define LOG_ORIGINALS_IN_BOTH_COMPS_ROOT   7

//#define SIMPLIFIED  // Uncomment this line to use the simplified query processing for gOverlaps which considers only 2/4 cases with regards to foundone and foundzero

#define SPLIT_LEAF_SUBTREE

typedef int PartitionId;
typedef int RecordId;
typedef int Timestamp;


struct RunSettings
{
	string       method;
	const char   *dataFile;
	const char   *queryFile;
	bool         verbose;
    unsigned int typeQuery;
    unsigned int typePredicate;
	unsigned int numPartitions;
	unsigned int numBits;
	unsigned int maxBits;
	unsigned int numRuns;
    unsigned int typeOptimizations;
    unsigned int sampleSize;
	
	void init()
	{
		verbose	          = false;
		numRuns           = 1;
        typeOptimizations = 0;
        sampleSize        = 0;
		numBits           = 0;
	};
};



struct StabbingQuery
{
	size_t id;
	Timestamp point;
    
    StabbingQuery()
    {
        
    };
    StabbingQuery(size_t i, Timestamp p)
    {
        id = i;
        point = p;
    };
};

struct RangeQuery
{
	size_t id;
	Timestamp start, end;

    RangeQuery()
    {
        
    };
    RangeQuery(size_t i, Timestamp s, Timestamp e)
    {
        id = i;
        start = s;
        end = e;
    };
};


struct hint_log
{
    unsigned int level;
    unsigned int type; // LOG_ORIGINALS_IN, LOG_ORIGINALS_IN_COPY, LOG_ORIGINALS_AFT, LOG_REPLICAS_IN, LOG_REPLICAS_AFT
    int left_idx;
    int right_idx;
    
    hint_log() {
        left_idx  = -1;
        right_idx = -1;
    };
};


class Timer
{
private:
	using Clock = std::chrono::high_resolution_clock;
	Clock::time_point start_time, stop_time;
	
public:
	Timer()
	{
		start();
	}
	
	void start()
	{
		start_time = Clock::now();
	}
	
	
	double getElapsedTimeInSeconds()
	{
		return std::chrono::duration<double>(stop_time - start_time).count();
	}
	
	
	double stop()
	{
		stop_time = Clock::now();
		return getElapsedTimeInSeconds();
	}
};

class Relation;

// Imports from utils
string toUpperCase(char *buf);
bool checkPredicate(string strPredicate, RunSettings &settings);
bool checkOptimizations(string strOptimizations, RunSettings &settings);
unsigned int determineOptimalNumBitsForHINT_M(const Relation &R, const float qe_precentage);
#endif // _GLOBAL_DEF_H_
