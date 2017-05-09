# NetCDF Benchmark Tool

## Introduction
NetCDF Performance Benchmark Tool (NetCDF-Bench) was developed to measure NetCDF performance on devices ranging from notebooks to large HPC systems. It mimics the typical I/O behavior of scientific climate applications and captures the performance on each node/process. In the end, it aggregates the data to human readable summary. 

NetCDF-Bench supports independent I/O, collective I/O and chunked I/O modes. If necessary, it can pre-fill the variables with some value. 

## Usage
### Domain decomposition
NetCDF-Bench supports various access patterns on a 4D dataset (one time dimension and three data dimensions). The short notation of the geometry is `(t:x:y:z)`, e.g., `(10:1000:1000:500)`.

The datasize must be a multiple of the blocksize. This restriction exists due the internal working of the benchmark-tool. (Simplified data partitioning.)

The data is read/written in timesteps. Default is one timestep per iteration of each parallel process.

Furthermore, the data is partitioned equally between the processes, i.e. each process handles x/nn, y/ppn, and the complete z-axis. Consequentially, each process writes the data in blocks `(1:1/px:1/py:z)` of double values to the shared file. To be more precise, `px` and `py` satisfy the following condition: `px · py = np = nn · ppn`, where `np` is the number of processes, `nn` is the number of nodes and, `ppn` is the number of processes per node.

# Parameters
NetCDF-Bench is able to run without any parameters. This facilitates the first step to the beginners. For the advanced usage the default configuration can be changed.

```
Benchtool (datatype: int) 
Synopsis: ./benchtool [-n] [-p] [-d] [-b] [-c] [-r] [-w] [-t] [-u] [-f] [-x] [-F] [--verify]  [Optional Args]

Flags
-r, --read                    Enable read benchmark
-w, --write                   Enable write benchmark
-u, --unlimited               Enable unlimited time dimension
-F, --use-fill-value          Write a fill value
--verify                      Verify that the data read is correct (reads the data again)

Optional arguments
-n, --nn=0                    Number of nodes
-p, --ppn=0                   Number of processes
-d, --data-geometry=STRING    Data geometry `(t:x:y:z)`
-b, --block-geometry=STRING   Block geometry `(t:x:y:z)`
-c, --chunk-geometry=STRING   Chunk geometry `(t:x:y:z|auto)`
-t, --io-type=ind             Independent / Collective I/O (ind|coll)
-f, --testfile=STRING         Filename of the testfile
-x, --output-format=human     Output-Format (parser|human)
```

## On-going work
We plan to extend the tool with the following features. 
1. Drop caches - Cached data can influence the results of I/O performance, therefore it must be cleared before benchmark runs. This features must run in user space.
2. Compression - This feature is useful to benchmakr the NetCDF compression, but also for people who are working on new compression methods.
3. CSV output - Detailed information about the benchmark run for analysis with third party tools.
