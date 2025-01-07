# CompactLTJ

CompactLTJ is a C++ library that provides an implementation of the Compact Leapfrog Triejoin (CLTJ) algorithm. This algorithm uses a compact representation of the tries to support the Leapfrog Triejoin algorithm, which is a well-known algorithm for evaluating SPARQL queries. The CompactLTJ library is based on the Succinct Data Structure Library (SDSL) and provides a fast and memory-efficient way to evaluate SPARQL queries on RDF datasets.

# Table of contents
- [Building the code](#building-the-code)
- [Usage of the library](#usage-of-the-library)
- [Command line interface](#command-line-interface)
- [Benchmark](#Benchmark)


## Building the code

In order to build the code, you need to have an extended version of the SDSL library installed. You can find the code to install the SDSL library [here](https://github.com/adriangbrandon/sdsl-lite.git) and follow these steps
```Bash
git clone https://github.com/adriangbrandon/sdsl-lite.git
cd sdsl-lite
./install.sh
```
After installing the SDSL library, you can clone this repository and build the code:

```Bash
git clone https://github.com/adriangbrandon/cltj.git
cd cltj
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

For a debug environment you can compile the code with the following command:
```Bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
```
Once the code is compiled, you should have three folder (`example`, `cmd`, and `bench`). The binaries of each folder are detailed in the following sections.
## Usage of the library
The CompactLTJ library supports different input data formats:
- **IDs**: This format is a text file where each IRI or Literal form the RDF dataset are encoded with an integer identifier.
- **RDF**: This format is a text file in RDF format, that is, each IRI and Literal is a string. 

Depending on the format of the input data, the library provides different classes to build the index. The classes can be found in `include/api` and are the following:
- **cltj_ids.hpp**: This class builds the index from a file with the IDs format.
- **cltj_rdf.hpp**: This class builds the index from a file with the RDF format.

Both of the classes have the same methods to use the index. The methods are the following:
- `constructor(dataset)`: given the path to the dataset, it builds the index.
- `query(query, res, limit, timeout)`: given a BGP query, this method solves the query and each result obtained is treated with the object res. The limit parameter is the maximum number of results that the user wants to obtain. The timeout parameter is the maximum time that the user wants to wait for the results. If the timeout is reached, the method returns the results obtained until that moment.
  Examples of the object res can be found in `include/results`. By default, we are using the `result_collector` object, which produces all the results but does not store all of them. It stores the number of results and the last 2<sup>20</sup> ones.
- `insert(triple)`: given a triple, it inserts it into the index.
- `remove(triple)`: given a triple, it removes it from the index.
- `sdsl::load_from_file(index, file)`: given an index and a file, it loads the index from the file.
- `sdsl::store_to_file(index, file)`: given an index and a file, it stores the index in the file.

On these classes there are several configurations of the indices that can be set. We recommend to use the following:
- `xcltj_ids_dyn` or `xcltj_rdf_dyn`: the best optimized version of the *xCLTJ* structure of the index. It uses the dynamic meta-tries and the queries are solved with the adaptive VEO considering the number of descendants.
- `cltj_ids_dyn` or `cltj_rdf_dyn`: the best optimized version of the *xCLTJ* structure of the index. It uses the dynamic meta-tries and the queries are solved with the adaptive VEO considering the number of descendants.

Basic examples of how to use the library can be found in the `example` folder, which source code is in `src/example`.

## Command line interface

The command line interface is a tool that allows the user to build the index and solve queries from the command line. The source code is in `src/cmd`. Again, there are two versions of the command line interface depending on the format of the input data. Both binaries are available in the `cmd`folder:
- **cmd-cltj**: this version works with the IDs format.
- **cmd-cltj-rdf**: this version works with the RDF format.

In both command line interface we can specify as first parameter two options:

- `build <file> <index> [version]`: to build the index. The parameters are the path to the dataset file, the path to the index file, and the kind of version to use: *xcltj* or *cltj*. By default, it is *xcltj*. The index is stored as `<index>.xcltj` or `<index>.cltj`, depending on the chosen version

  **Example**: creates the index data-ids.xcltj from the file data.txt.
  ```Bash
  ./cmd-cltj build data.txt data-ids xcltj 
  ```
- `run <index> [print] [limit] [timeout] [veo]`: to load the index and interact with it. The parameters are the path to the index file, a flag to print the results, the limit of the results, the timeout in seconds, and the VEO to use in the queries. The print argument admits *none* and *print* values, the second one shows the results obtained by the queries. Regarding the veo parameter, the options are *adaptive* or *global*. By default, the print flag is *none*, the limit is 1000, the timeout is 600 seconds, and the VEO is *adaptive*.

  **Example**: runs the interactive command line interface on the index data-ids.xcltj, the results are limited to 1000, and they are not printed.
  ```Bash
  ./cmd-cltj run data-ids.xcltj
  ```
  The command line interface supports the following commands:
  - `query <n>`: specifies that in the following *n* lines there are *n* queries to solve. We show the time required for each query in nanoseconds.
  - `insert <n>`: specifies that in the following *n* lines there are *n* triples to insert. We show the time required for the block of insertions in nanoseconds.
  - `delete <n>`: specifies that in the following *n* lines there are *n* triples to delete. We show the time required for the block of deletions in nanoseconds.
  - `commit`: commits the changes made with the insert and remove commands into the index file.
  - `quit`: exits the command line interface.

  **Example**: running two queries, inserting and removing the triple `1 1 1`, and committing the changes.
  ```Bash
  [CLTJ]> query 2
  [CLTJ]> [1/2] ?x ?y ?z . ?z ?y ?k
  [CLTJ]> [2/2] ?x ?y ?x
          [1/2] 1000 results in 131112625 ns.
          [2/2] 1000 results in 133458 ns.
  [CLTJ]> insert 1
  [CLTJ]> [1/1] 1 1 1
          [1/1] 1 triples inserted in 2389605126 ns.
  [CLTJ]> delete 1
  [CLTJ]> [1/1] 1 1 1
          [1/1] 1 triples deleted in 1280416 ns.
  [CLTJ]> commit
          Commit updates... done.
  [CLTJ]> quit
  ```
## Benchmark

In order to replicate the results obtained on the experimental evaluation, we provide the benchmark code in the `bench` folder. The benchmark code is in `src/bench`. The benchmark code is divided into two parts:
- **build-**: the binaries prefixed with *build-* are used to builds the index from the dataset. They just need the path of the dataset and they generate the index in the same folder as the dataset. The binaries are:
  - **build-cltj**: builds the static version of *CLTJ* from the dataset with IDs format.
  - **build-xcltj**: builds the static version of *xCLTJ* from the dataset with IDs format.
  - **build-uncltj**: builds the static version of *UnCLTJ* from the dataset with IDs format.
  - **build-cltj-dyn**: builds the dynamic version of *CLTJ* from the dataset with IDs format.
  - **build-xcltj-dyn**: builds the dynamic version of *xCLTJ* from the dataset with IDs format.
  - **build-cltj-rdf**: builds the dynamic version of *CLTJ* from the dataset with RDF format.
  - **build-xcltj-rdf**: builds the dynamic version of *xCLTJ* from the dataset with RDF format.
- **bench-query-\<index>**: those binaries are used to solve the queries in the static indices built from the dataset with IDs format. They need the path of the index, the path of the queries file, the limit of their results and 
the type of index *star* or *normal* version. Similar to the build phase there is a binary for each kind of index in the experimental evaluation. Note that some of them are suffixed with *-global*, those are the binaries that use de global VEO, the remaining ones use the adaptive VEO. The output of each binary follows the format `<query number>;<number of results>;<elapsed time>`, where the elapsed time is in nanoseconds.
- **bench-query-\<index>-rdf**: those binaries are used to solve the queries in the dynamic indices built from the dataset with RDF format. The input parameters are the same as before, but the output changes to `<query number>;<number of results>;<string to id time>;<query elapsed time>; <id to string time>`, where the times are measured in nanoseconds. The new fields are the time required to convert the strings of the query to the IDs and the time required to convert the results from IDs to strings, in that order.
- **bench-update-\<index>**: those binaries are used to solve the queries in the dynamic indices built from the dataset with IDs format. They need the path of the index, the path of the queries file, the path of the updates file, the ratio of updates per query, the limit of their results and the type of index *star* or *normal* version. The output of each binary follows the format `<query number>;<number of results>;<elapsed time>`, where the elapsed time is in nanoseconds.

The dataset used on our benchmark can be found at [TODO] [here]() and the script `bench.sh` to replicate our experiments can be found wihtin the folder `src\bench`.
