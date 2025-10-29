#include "graph.hpp"
#include <mpi.h>
#include <algorithm>
#include <set>

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
    
    // Build dependency map: for each outgoing edge's sendIndex, find if there's an incoming edge with matching recvIndex
    // A dependent edge is an incoming edge where recvIndex equals an outgoing edge's sendIndex
    std::set<int> dependent_buffer_indices;
    for (const Edge& out_edge : outgoing) {
        for (const Edge& in_edge : incoming) {
            if (in_edge.recvIndex == out_edge.sendIndex) {
                dependent_buffer_indices.insert(out_edge.sendIndex);
                break;
            }
        }
    }
    
    // Custom comparator for sorting edges
    // Priority: dependent incoming edges first, then other incoming edges, then outgoing edges
    auto edge_comparator = [&dependent_buffer_indices](const std::pair<bool, const Edge*>& a, const std::pair<bool, const Edge*>& b) {
        bool a_is_send = !a.first;
        bool b_is_recv = b.first;
        const Edge* a_edge = a.second;
        const Edge* b_edge = b.second;

        // Not in order if b depends on then buffer sent by a
        if (a_is_send && b_is_recv) {
            int a_send_index = a_edge->sendIndex;
            int b_recv_index = b_edge->recvIndex;
	    if (a_send_index == b_recv_index) {
		return false; // Dependent recv comes first
	    }
        }

        // Otherwise take the (a,b) ordering
        return true;
    };
    
    // Create a sorted list of operations
    std::vector<std::pair<bool, const Edge*>> operations; // (is_recv, edge*)
    for (const Edge& edge : incoming) {
        operations.push_back({true, &edge});
    }
    for (const Edge& edge : outgoing) {
        operations.push_back({false, &edge});
    }
    
    std::stable_sort(operations.begin(), operations.end(), edge_comparator);
    
    // Post operations in sorted order
    std::vector<MPI_Request> recv_requests(incoming.size());
    std::vector<MPI_Request> send_requests(outgoing.size());
    std::vector<bool> recv_completed(incoming.size(), false);
    
    int recv_idx = 0;
    int send_idx = 0;
    
    // Post all receives first
    for (const auto& op : operations) {
        if (op.first) { // is_recv
            const Edge* edge = op.second;
            MPI_Irecv(buffers[edge->recvIndex].data, buffers[edge->recvIndex].size, 
                     MPI_BYTE, edge->from, 0, MPI_COMM_WORLD, &recv_requests[recv_idx]);
            recv_idx++;
        }
    }
    
    // Now post sends, waiting for dependent receives to complete first
    for (const auto& op : operations) {
        if (!op.first) { // is_send
            const Edge* edge = op.second;
            
            // Check if this send depends on a receive
            if (dependent_buffer_indices.count(edge->sendIndex) > 0) {
                // Find the corresponding receive and wait for it
                for (size_t i = 0; i < incoming.size(); i++) {
                    if (incoming[i].recvIndex == edge->sendIndex && !recv_completed[i]) {
                        MPI_Wait(&recv_requests[i], MPI_STATUS_IGNORE);
                        recv_completed[i] = true;
                    }
                }
            }
            
            // Post the send
            MPI_Isend(buffers[edge->sendIndex].data, buffers[edge->sendIndex].size, 
                     MPI_BYTE, edge->to, 0, MPI_COMM_WORLD, &send_requests[send_idx]);
            send_idx++;
        }
    }
    
    // Wait for all remaining operations to complete
    if (!send_requests.empty()) {
        MPI_Waitall(send_requests.size(), send_requests.data(), MPI_STATUSES_IGNORE);
    }
    for (size_t i = 0; i < recv_requests.size(); i++) {
        if (!recv_completed[i]) {
            MPI_Wait(&recv_requests[i], MPI_STATUS_IGNORE);
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

