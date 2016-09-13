# Readme

The Cache-Sensitive Skip List (CSSL) is a cache-sensitive variant of the
conventional skip list that uses a different memory layout to take
maximal advantage of modern CPUs.
Please have a look at the paper `Cache-Sensitive Skip List: Efficient
Range Queries on modern CPUs` presented at ADMS-IMDM 2016 to get a
detailed description of the index structure.

# Requirements

CSSL uses Intel's AVX instructions (SIMD), so please make sure that your CPU supports them.

# Setup

Setup is easy, just use the provided `Makefile` and run the created executable `cssl`.
In the following example, range queries and lookups are executed on 16M
sparse integer keys.

1) make clean all
2) ./cssl 16000000 1

Please execute `./cssl` to see a description of needed parameters.

# Configuration

If you want to use a skip value greater than 5, please adjust the
parameter MAX\_SKIP in the file skiplist.h (line 4).
