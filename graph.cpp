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
    
    // Sort the edges so that sends come before receives, except for when a send depends on a receive
    auto edge_comparator = [&dependent_buffer_indices, &buffers](const std::pair<bool, const Edge*>& a, const std::pair<bool, const Edge*>& b) {
        bool a_is_recv = a.first;
        bool a_is_send = !a.first;
        bool b_is_recv = b.first;
        bool b_is_send = !b.first;
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

        // If both are sends and one is a source, make the source come first
        if (a_is_send && b_is_send) {
            BufferType a_type = buffers[a_edge->sendIndex].type;
            BufferType b_type = buffers[b_edge->sendIndex].type;
            if (a_type == GraphColl::BufferType::Source && b_type != GraphColl::BufferType::Source) {
                return true;
            }
            if (a_type != GraphColl::BufferType::Source && b_type == GraphColl::BufferType::Source) {
                return false;
            }
        }

        // If not a dependent edge, make sends come first
        if (a_is_recv && b_is_send)
            return false;

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
    
    // Post operations in sorted order, using blocking sends and recvs as per the ordering in operations vector
    for (const auto& op : operations) {
        const Edge* edge = op.second;
        if (op.first) { // is_recv
            MPI_Recv(buffers[edge->recvIndex].data, buffers[edge->recvIndex].size,
                     MPI_BYTE, edge->from, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } else { // is_send
            MPI_Send(buffers[edge->sendIndex].data, buffers[edge->sendIndex].size,
                     MPI_BYTE, edge->to, 0, MPI_COMM_WORLD);
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

