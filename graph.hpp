#pragma once

#include <iostream>
#include <vector>

namespace GraphColl {

enum BufferType {
    Source,
    Sink,
    Intermediate
};

struct Buffer {
    void *data;      // the data pointer itself
    size_t size;     // bytes
    BufferType type; // buffer type
};

struct Edge {
    int to;         // Where this edge goes
    int from;       // Where this edge comes from
    int sendIndex; // The index in the vector of buffers that this edge should use when sending
    int recvIndex; // Same but for receiving
    //int sendWeight; // The order the sending node should process the edge
    //int recvWeight; // The order the recving node should process the edge
};

// Overload << operator for easy printing of Edge
inline std::ostream& operator<<(std::ostream& os, const Edge& edge) {
    os << "Edge{from: " << edge.from 
       << ", to: " << edge.to 
       << ", sendIndex: " << edge.sendIndex 
       << ", recvIndex: " << edge.recvIndex << "}";
    return os;
}

class Graph {
public:
    Graph(int rank, int n);
    ~Graph();
    int n() const { return n_; }
    int execute(std::vector<Buffer> &buffers);
    void addEdge(int src, int dest, int sendWeight, int recvWeight);
private:
    void optimizationPass(std::vector<Buffer> &buffers);
    void postComms(std::vector<Buffer> &buffers);
    int n_;
    int rank_;
    std::vector<std::vector<Edge>> adj_;
};

} // namespace GraphColl

