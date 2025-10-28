#include "graph.hpp"
#include <mpi.h>
#include <iostream>
#include <vector>

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
    
    std::vector<Edge> &outgoing = adj_[rank_];
    
    // Build dependency map: which sends depend on which receives
    // send_dependencies[send_idx] = set of recv_idx that must complete before this send
    std::vector<std::vector<size_t>> send_dependencies(outgoing.size());
    
    for (size_t send_idx = 0; send_idx < outgoing.size(); send_idx++) {
        int send_buffer = outgoing[send_idx].sendIndex;
        
        // Check if any receive writes to this buffer
        for (size_t recv_idx = 0; recv_idx < incoming.size(); recv_idx++) {
            if (incoming[recv_idx].recvIndex == send_buffer) {
                send_dependencies[send_idx].push_back(recv_idx);
            }
        }
    }
    
    // Allocate request arrays
    std::vector<MPI_Request> recv_requests(incoming.size(), MPI_REQUEST_NULL);
    std::vector<MPI_Request> send_requests(outgoing.size(), MPI_REQUEST_NULL);
    std::vector<bool> recv_completed(incoming.size(), false);
    std::vector<bool> send_posted(outgoing.size(), false);
    
    // Post ALL receives first (standard MPI pattern to avoid deadlocks)
    for (size_t recv_idx = 0; recv_idx < incoming.size(); recv_idx++) {
        const Edge& edge = incoming[recv_idx];
        MPI_Irecv(buffers[edge.recvIndex].data, buffers[edge.recvIndex].size,
                 MPI_BYTE, edge.from, 0, MPI_COMM_WORLD, &recv_requests[recv_idx]);
    }
    
    // Post all independent sends immediately (sends that don't depend on any receive)
    for (size_t send_idx = 0; send_idx < outgoing.size(); send_idx++) {
        if (send_dependencies[send_idx].empty()) {
            const Edge& edge = outgoing[send_idx];
            MPI_Isend(buffers[edge.sendIndex].data, buffers[edge.sendIndex].size,
                     MPI_BYTE, edge.to, 0, MPI_COMM_WORLD, &send_requests[send_idx]);
            send_posted[send_idx] = true;
        }
    }
    
    // Now iteratively make progress: test receives, and post sends when dependencies are satisfied
    // This avoids blocking waits that could cause deadlock
    size_t sends_remaining = 0;
    for (size_t send_idx = 0; send_idx < outgoing.size(); send_idx++) {
        if (!send_posted[send_idx]) {
            sends_remaining++;
        }
    }
    
    while (sends_remaining > 0) {
        // Test all incomplete receives (non-blocking check)
        for (size_t recv_idx = 0; recv_idx < incoming.size(); recv_idx++) {
            if (!recv_completed[recv_idx]) {
                int flag;
                MPI_Test(&recv_requests[recv_idx], &flag, MPI_STATUS_IGNORE);
                if (flag) {
                    recv_completed[recv_idx] = true;
                }
            }
        }
        
        // Try to post sends whose dependencies are now satisfied
        for (size_t send_idx = 0; send_idx < outgoing.size(); send_idx++) {
            if (send_posted[send_idx]) {
                continue;  // Already posted
            }
            
            // Check if all dependencies are satisfied
            bool can_post = true;
            for (size_t recv_idx : send_dependencies[send_idx]) {
                if (!recv_completed[recv_idx]) {
                    can_post = false;
                    break;
                }
            }
            
            if (can_post) {
                const Edge& edge = outgoing[send_idx];
                MPI_Isend(buffers[edge.sendIndex].data, buffers[edge.sendIndex].size,
                         MPI_BYTE, edge.to, 0, MPI_COMM_WORLD, &send_requests[send_idx]);
                send_posted[send_idx] = true;
                sends_remaining--;
            }
        }
    }
    
    // Wait for all remaining receives to complete
    for (size_t recv_idx = 0; recv_idx < incoming.size(); recv_idx++) {
        if (!recv_completed[recv_idx]) {
            MPI_Wait(&recv_requests[recv_idx], MPI_STATUS_IGNORE);
        }
    }
    
    // Wait for all sends to complete
    for (size_t send_idx = 0; send_idx < outgoing.size(); send_idx++) {
        if (send_posted[send_idx]) {
            MPI_Wait(&send_requests[send_idx], MPI_STATUS_IGNORE);
        }
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

