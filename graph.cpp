#include "graph.hpp"
#include <mpi.h>

namespace GraphColl {

Graph::Graph(int rank, int n) : rank_(rank), n_(n) {
    // Initialize with empty adjacency list
    adj_.clear();
    adj_.resize(n);
}
Graph::~Graph() {}

void Graph::optimizationPass(std::vector<Buffer> &buffers) {
    // TODO: Implement
}

void Graph::postComms(std::vector<Buffer> &buffers) {
    // Find incoming edges (edges where this rank_ is the destination)
    std::vector<Edge> incoming;
    for (int i = 0; i < n_; i++) {
        for (const Edge& edge : adj_[i]) {
	    if (edge.to == rank_) {
	        incoming.push_back(edge);
            }
        }
    }
    
    // Post MPI_Recv for each incoming edge TODO: assert recvIndex is in bound of buffers
    std::vector<MPI_Request> recv_requests;
    for (const Edge& edge : incoming) {
        MPI_Request request;
        MPI_Irecv(buffers[edge.recvIndex].data, buffers[edge.recvIndex].size, MPI_BYTE, edge.from, 0, MPI_COMM_WORLD, &request);
        recv_requests.push_back(request);
    }
    
    std::vector<Edge> &outgoing = adj_[rank_];
    
    // Post MPI_Send for each outgoing edge TODO: assert sendIndex is in bounds of buffers
    std::vector<MPI_Request> send_requests;
    for (const Edge& edge : outgoing) {
        MPI_Request request;
        MPI_Isend(buffers[edge.sendIndex].data, buffers[edge.sendIndex].size, MPI_BYTE, edge.to, 0, MPI_COMM_WORLD, &request);
        send_requests.push_back(request);
    }
    
    // Wait for all comms to complete
    if (!send_requests.empty()) {
        MPI_Waitall(send_requests.size(), send_requests.data(), MPI_STATUSES_IGNORE);
    }
    if (!recv_requests.empty()) {
        MPI_Waitall(recv_requests.size(), recv_requests.data(), MPI_STATUSES_IGNORE);
    }
}

/* Run the heuristics to optimize the graph, then post the comms */
int Graph::execute(std::vector<Buffer> &buffers) {
    // Validate inputs
    if (buffers.empty()) {
        std::cerr << "Error: No buffers provided" << std::endl;
        return -1;
    }
    
    optimizationPass(buffers);
    postComms(buffers);
    
    return 0;
}

/* Represents a send from src's sendIndex and into dest's recvIndex */
void Graph::addEdge(int src, int dest, int sendIndex, int recvIndex) {
    adj_[src].push_back({dest, src, sendIndex, recvIndex});
}

} // namespace GraphColl

