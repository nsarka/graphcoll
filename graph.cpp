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
    // Step 1: Find incoming edges (edges where this rank_ is the destination)
    std::vector<Edge> incoming;
    for (int i = 0; i < n_; i++) {
        if (i < adj_.size()) {
            for (const Edge& edge : adj_[i]) {
                if (edge.to == rank_) {
                    incoming.push_back(edge);
                }
            }
        }
    }
    
    // Step 2: Post MPI_Recv for each incoming edge
    std::vector<MPI_Request> recv_requests;
    for (const Edge& edge : incoming) {
        MPI_Request request;
        // Find the source rank_ for this edge
        int source_rank = -1;
        for (int i = 0; i < n_; i++) {
            if (i < adj_.size()) {
                for (const Edge& e : adj_[i]) {
                    if (e.to == rank_ && e.sendWeight == edge.sendWeight && e.recvWeight == edge.recvWeight) {
                        source_rank = i;
                        break;
                    }
                }
            }
            if (source_rank != -1) break;
        }
        
        if (source_rank != -1) {
            // Use the first buffer for receiving (assuming single buffer per rank_)
            MPI_Irecv(buffers[0].data, buffers[0].size, MPI_BYTE, source_rank, 0, MPI_COMM_WORLD, &request);
            recv_requests.push_back(request);
        }
    }
    
    // Step 3: Wait for all receives to complete
    if (!recv_requests.empty()) {
        MPI_Waitall(recv_requests.size(), recv_requests.data(), MPI_STATUSES_IGNORE);
    }
    
    // Step 4: Find outgoing edges (edges where this rank_ is the source)
    std::vector<Edge> outgoing;
    if (rank_ < adj_.size()) {
        for (const Edge& edge : adj_[rank_]) {
            outgoing.push_back(edge);
        }
    }
    
    // Step 5: Post MPI_Send for each outgoing edge
    std::vector<MPI_Request> send_requests;
    for (const Edge& edge : outgoing) {
        MPI_Request request;
        // Use the first buffer for sending (assuming single buffer per rank_)
        MPI_Isend(buffers[0].data, buffers[0].size, MPI_BYTE, edge.to, 0, MPI_COMM_WORLD, &request);
        send_requests.push_back(request);
    }
    
    // Wait for all sends to complete
    if (!send_requests.empty()) {
        MPI_Waitall(send_requests.size(), send_requests.data(), MPI_STATUSES_IGNORE);
    }
}

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

void Graph::addEdge(int src, int dest, int sendWeight, int recvWeight) {
    if (src >= adj_.size()) {
        while (src >= adj_.size()) {
            adj_.push_back({});
        }
    }
    adj_[src].push_back({dest, sendWeight, recvWeight});
}

} // namespace GraphColl

