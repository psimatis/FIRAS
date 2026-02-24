# FIRAS: Framework for Interval Range Search and Sampling

Source code from the publication at SIGMOD 2026.

<p align="justify"> 
Intervals are ubiquitous in many applications, including temporal and uncertain databases. Range search, which retrieves all intervals that overlap a given query interval, is a key operation in such applications. As data sizes grow, range search results can become large, overwhelming users and resulting in long search times. Obtaining random samples from a large search result is a promising approach that alleviates the above issues. While for some applications, sampling range query results is adequate, others may require the complete query result. Hence, a challenging question arises: can we design a framework that efficiently handles both range search and range sampling? This work provides a positive answer. Based on a novel query decomposition idea, we propose FIRAS, a framework that supports range search and sampling in $\tilde{O}(1)+O(k)$ time and $\tilde{O}(1)+O(s)$ time, respectively, with $O(n)$ space, where $\tilde{O}(\cdot)$ hides any logarithmic factors, $k$ ($s$) is the range search result (sample) size, and $n$ is the data size. In other words, the runtime of FIRAS is sensitive only to the output size ($k$ or $s$), and not to $n$. FIRAS can also be used to know the result size $k$ of a range query in $\tilde{O}(1)$ time; subsequently, the issuer can decide whether to retrieve all results or random samples thereof in $O(k)$ or $O(s)$ time, respectively. Finally, we extend FIRAS to apply to evolving interval data, where queries interleave with updates and both have to be supported efficiently. Our extensive experiments on real-world datasets demonstrate the efficiency of FIRAS.
</p>

## Dependencies

- g++/gcc compiler
- Standard C++ libraries (no external dependencies)

## Quick Start

First unzip the archives in the data directory.

```bash
# Compile and run all experiments
./run.sh
```

This will build all executables and run the complete experimental suite on both static and dynamic data.
Results saved to `benchmark.log`.

## Manual Compilation & Execution

If you want to compile and run specific components:

```bash
# Build all executables
make clean && make -j$(nproc 2>/dev/null || echo 4)
cd indices/LIT && make clean && make pureLIT_3col && cd ../..

# Static range queries
./query_it_firas.exec -r 5 data/Static/BTC.txt data/Static/BTC_queries.txt

# Static sampling (100 samples)
./query_it_firas.exec -s 100 -r 5 data/Static/BTC.txt data/Static/BTC_queries.txt

# Dynamic range queries
./query_stream_firas_dit.exec -r 5 data/Dynamic/BTC_stream.txt

# Dynamic sampling
./query_stream_firas_dit.exec -s 100 -r 5 data/Dynamic/BTC_stream.txt
```

**Parameters:**
- `-r N`: Number of runs per query
- `-s N`: Sample size

If `-s` is omitted, the method performs range queries (returns all overlapping intervals). 
If `-s` is specified, the method performs sampling queries (returns N random samples).

## Methods

- Interval Tree
- HINT  
- FIRAS-IT (our method)
- AIT
- PURELIT
- Stream AIT
- Stream FIRAS (our method)


## Dataset

Example data and queries are provided (compressed) in the `data/` directory:
- `data/Static/BTC.txt` - Static interval data
- `data/Static/BTC_queries.txt` - Query intervals for static data
- `data/Dynamic/BTC_stream.txt` - Streaming data
