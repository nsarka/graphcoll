#pragma once

#include <iostream>
#include <vector>

namespace GraphColl {

enum BufferType {
    Source,
    Destination,
    Intermediate
};

struct Buffer {
    void *data;      // the data pointer itself
    size_t size;     // bytes
    BufferType type; // buffer type
};

struct Edge {
    int to;         // Where this edge goes
    int sendWeight; // The order the sending node should process the edge
    int recvWeight; // The order the recving node should process the edge
};

class Graph {
public:
    Graph();
    ~Graph();
    int vertices() const { return vertices_; }
    int execute(int rank, int comm_size, std::vector<Buffer> &buffers);
    void addEdge(int src, int dest, int sendWeight, int recvWeight);
    int addVertex();
private:
    int vertices_;
    std::vector<std::vector<Edge>> adj_;
};

} // namespace GraphColl

