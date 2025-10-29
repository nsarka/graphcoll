# GraphColl: Graph-Based Collective Communication Library

GraphColl is a C++ library for expressing and executing custom collective communication patterns using graph-based representations. Built on top of MPI, it allows you to define complex communication patterns as directed graphs and execute them efficiently.

## Overview

GraphColl represents collective communication operations (like broadcast, allgather, reduce-scatter, etc.) as directed graphs where:
- **Nodes** represent MPI ranks
- **Edges** represent point-to-point data transfers between ranks
- **Buffers** can be sources, sinks, or intermediates in the communication flow

The library handles the scheduling and execution of MPI send/receive operations based on the graph topology.

## Why not NCCL or UCC?

Because communication patterns are defined as graphs, you can easily express and execute any custom pattern you need. Before execution, an optimization pass improves the graph by fixing inefficiencies such as unnecessary duplicate edges, suboptimal use of the system topology, or imbalanced communication across ranks.

## Features

- **Graph-based abstraction**: Define any custom collective operation as a directed graph
- **Automatic scheduling**: The library handles posting and completing communication operations
- **Non-blocking communication**: Uses MPI_Isend/Irecv for asynchronous execution (this will likely change to direclty call UCX)
- **Buffer type classification**: Distinguish between source, intermediate, and sink buffers
- **Flexible edge specification**: Control which buffers are used for sending and receiving

## Project Structure

```
graphcoll/
├── graph.hpp          # Header file with Graph, Buffer, and Edge definitions
├── graph.cpp          # Implementation of Graph class with MPI logic
├── main.cpp           # Test suite with broadcast, allgather examples
├── Makefile           # Build system
├── run.sh             # Script to run the testbench with MPI
├── README.md          # This file
└── LICENSE            # License information
```

## Dependencies

- **MPI implementation** (OpenMPI, MPICH, or similar)
- **C++ compiler** with C++11 support (g++, clang++)
- **mpicxx** wrapper for compiling MPI programs

## Building

```bash
make
```

This generates:
- `libgraphcoll.so` - Shared library containing the Graph class implementation
- `testbench` - Executable with test cases

To clean build artifacts:
```bash
make clean
```

## Running Tests

The project includes a test suite with examples of broadcast and allgather implementations:

```bash
mpirun -n 4 ./testbench
```

Or use the provided script:
```bash
./run.sh
```

### Example Output

```
Building test bcast graph from root=1 to 4 other ranks in the comm
Executing allgather graph...
Done executing allgather graph
Graph execution results:
  Rank 0: 1
  Rank 1: 1
  Rank 2: 1
  Rank 3: 1
```

## API Reference

### Core Classes

#### `GraphColl::Graph`

Main class for defining and executing collective operations.

```cpp
Graph(int rank, int n);
```
- `rank`: MPI rank of the calling process
- `n`: Total number of processes in the communicator

```cpp
void addEdge(int src, int dest, int sendIndex, int recvIndex);
```
- `src`: Source rank
- `dest`: Destination rank
- `sendIndex`: Index in the buffer list for the send operation
- `recvIndex`: Index in the buffer list for the receive operation

```cpp
int execute(std::vector<Buffer> &buffers);
```
Executes the communication pattern defined by the graph edges.

#### `GraphColl::Buffer`

Represents a communication buffer.

```cpp
struct Buffer {
    void *data;         // Pointer to the data
    size_t size;        // Size in bytes
    BufferType type;    // Source, Sink, or Intermediate
};
```

#### `GraphColl::BufferType`

```cpp
enum BufferType {
    Source,         // Buffer contains initial data to send
    Sink,           // Buffer receives final data
    Intermediate    // Buffer is used for both receiving and sending
};
```

#### `GraphColl::Edge`

Represents a data transfer between two ranks.

```cpp
struct Edge {
    int to;         // Destination rank
    int from;       // Source rank
    int sendIndex;  // Buffer index for sending
    int recvIndex;  // Buffer index for receiving
};
```

## Usage Examples

### Broadcast Example

Broadcasting a value from root rank to all other ranks:

```cpp
int root = 1;
int my_data = rank;

std::vector<GraphColl::Buffer> buffers;
GraphColl::Buffer buf;
buf.data = &my_data;
buf.size = sizeof(int);
buf.type = (rank == root) ? GraphColl::BufferType::Source 
                          : GraphColl::BufferType::Sink;
buffers.push_back(buf);

GraphColl::Graph bcast(rank, comm_size);
for (int i = 0; i < comm_size; i++) {
    if (i != root) {
        bcast.addEdge(root, i, 0, 0);
    }
}

bcast.execute(buffers);
```

### Allgather Example

Gathering data from all ranks to all ranks:

```cpp
int *sendbuf = (int*) malloc(sizeof(int));
int *recvbuf = (int*) malloc(sizeof(int) * comm_size);
*sendbuf = rank;

std::vector<GraphColl::Buffer> buffers;

// Add send buffer
GraphColl::Buffer buf;
buf.data = sendbuf;
buf.size = sizeof(int);
buf.type = GraphColl::BufferType::Source;
buffers.push_back(buf);

// Add receive buffers
for (int i = 0; i < comm_size; i++) {
    buf.data = &recvbuf[i];
    buf.size = sizeof(int);
    buf.type = GraphColl::BufferType::Sink;
    buffers.push_back(buf);
}

GraphColl::Graph allgather(rank, comm_size);
for (int i = 0; i < comm_size; i++) {
    for (int j = 0; j < comm_size; j++) {
        allgather.addEdge(i, j, 0, i+1);
    }
}

allgather.execute(buffers);
```

## Implementation Details

### Edge Management

- Edges are stored in an adjacency list structure
- Duplicate edges are automatically filtered out
- Each rank only stores outgoing edges in its adjacency list

## Roadmap

- [ ] Implement optimization pass to reorder operations for better performance
- [ ] Implement system topology aware optimizations

## Contributing

Contributions are welcome! Please feel free to submit issues or pull requests.

## License

See the LICENSE file for details.