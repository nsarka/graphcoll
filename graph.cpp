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
/*
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
    
    // Sort the edges so that sends come before receives, except for when a send depends on a receive
    auto edge_comparator = [&buffers](const std::pair<bool, const Edge*>& a, const std::pair<bool, const Edge*>& b) {
        bool a_is_recv = a.first;
        bool a_is_send = !a.first;
        bool b_is_recv = b.first;
        bool b_is_send = !b.first;
        const Edge* a_edge = a.second;
        const Edge* b_edge = b.second;

        // Not in order if b depends on the buffer sent by a
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
            if (a_type != GraphColl::BufferType::Source && b_type == GraphColl::BufferType::Source) {
                // Sources should come before intermediates and sinks
                return false;
            }
            if (a_type == GraphColl::BufferType::Sink && b_type != GraphColl::BufferType::Sink) {
                // Sinks should come after non-sinks
                return false;
            }
        }

        // If not a dependent edge, make sends come first
        if (a_is_recv && b_is_send) {
            return false;
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
    
    //std::stable_sort(operations.begin(), operations.end(), edge_comparator);
    std::sort(operations.begin(), operations.end(), edge_comparator);

    //std::cout << "Sorted " << operations.size() << " edges. Incoming=" << incoming.size() << " , outgoing=" << outgoing.size() << std::endl;
 
    // Post operations in sorted order, using blocking sends and recvs as per the ordering in operations vector
    for (const auto& op : operations) {
        const Edge* edge = op.second;
        if (op.first) { // is_recv
            //std::cout << "Recving from " << edge->from << " recvIndex " << edge->recvIndex << std::endl;
            MPI_Recv(buffers[edge->recvIndex].data, buffers[edge->recvIndex].size,
                     MPI_BYTE, edge->from, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } else { // is_send
            //std::cout << "Sending to " << edge->to << " sendIndex " << edge->sendIndex << std::endl;
            MPI_Send(buffers[edge->sendIndex].data, buffers[edge->sendIndex].size,
                     MPI_BYTE, edge->to, 0, MPI_COMM_WORLD);
        }
    }
}
*/

void Graph::postComms(std::vector<Buffer> &buffers) {
    // Step 1: Find incoming edges (edges where this rank_ is the destination)
    std::vector<Edge> incoming;
    for (int i = 0; i < n_; i++) {
        for (const Edge& edge : adj_[i]) {
	    if (edge.to == rank_) {
	        incoming.push_back(edge);
            }
        }
    }
    
    // Step 2: Post MPI_Recv for each incoming edge TODO: assert recvIndex is in bound of buffers
    std::vector<MPI_Request> recv_requests;
    for (const Edge& edge : incoming) {
        MPI_Request request;
        MPI_Irecv(buffers[edge.recvIndex].data, buffers[edge.recvIndex].size, MPI_BYTE, edge.from, 0, MPI_COMM_WORLD, &request);
        recv_requests.push_back(request);
    }
    
    
    std::vector<Edge> &outgoing = adj_[rank_];
    
    // Step 5: Post MPI_Send for each outgoing edge TODO: assert sendIndex is in bounds of buffers
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
    for (const Edge& edge : adj_[src]) {
        if (edge.to == dest && edge.from == src && edge.sendIndex == sendIndex && edge.recvIndex == recvIndex) {
            //std::cout << "Skipping adding duplicate edge" << std::endl;
            return;
        }
    }
    adj_[src].push_back({dest, src, sendIndex, recvIndex});
}

} // namespace GraphColl

