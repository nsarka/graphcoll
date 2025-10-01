#include "graph.hpp"
#include <mpi.h>

namespace GraphColl {

Graph::Graph() {}
Graph::~Graph() {}

int Graph::execute(int rank, int comm_size, std::vector<Buffer> &buffers) {
    std::vector<Edge> incoming;

    for (int i = 0; i < comm_size; i++) {
        
    }
    
    return 0;
}

void Graph::addEdge(int src, int dest, int sendWeight, int recvWeight) {
    adj_[src].push_back({dest, sendWeight, recvWeight});
}

int Graph::addVertex() {
    return vertices_;
}

} // namespace GraphColl

