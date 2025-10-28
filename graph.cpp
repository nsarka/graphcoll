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
    
    // Build operation dependency graph using topological sort
    // Operation indices: [0, incoming.size()) are receives, [incoming.size(), incoming.size() + outgoing.size()) are sends
    size_t num_ops = incoming.size() + outgoing.size();
    std::vector<std::vector<size_t>> op_graph(num_ops);  // op_graph[i] = list of operations that depend on operation i
    std::vector<int> in_degree(num_ops, 0);  // Number of dependencies for each operation
    
    // Build dependency edges: recv_i -> send_j if send_j uses buffer that recv_i writes to
    for (size_t send_idx = 0; send_idx < outgoing.size(); send_idx++) {
        int send_buffer = outgoing[send_idx].sendIndex;
        
        // Check if any receive writes to this buffer
        for (size_t recv_idx = 0; recv_idx < incoming.size(); recv_idx++) {
            if (incoming[recv_idx].recvIndex == send_buffer) {
                // This send depends on this receive
                op_graph[recv_idx].push_back(incoming.size() + send_idx);
                in_degree[incoming.size() + send_idx]++;
            }
        }
    }
    
    // Topological sort using Kahn's algorithm
    std::vector<size_t> topo_order;
    std::vector<size_t> ready_queue;
    
    // Start with all operations that have no dependencies
    for (size_t i = 0; i < num_ops; i++) {
        if (in_degree[i] == 0) {
            ready_queue.push_back(i);
        }
    }
    
    while (!ready_queue.empty()) {
        size_t op_idx = ready_queue.back();
        ready_queue.pop_back();
        topo_order.push_back(op_idx);
        
        // Reduce in-degree for dependent operations
        for (size_t dependent_op : op_graph[op_idx]) {
            in_degree[dependent_op]--;
            if (in_degree[dependent_op] == 0) {
                ready_queue.push_back(dependent_op);
            }
        }
    }
    
    // Check for cycles (should not happen in valid communication patterns)
    if (topo_order.size() != num_ops) {
        std::cerr << "Error: Cyclic dependencies detected in communication pattern!" << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    // Allocate request arrays
    std::vector<MPI_Request> recv_requests(incoming.size(), MPI_REQUEST_NULL);
    std::vector<MPI_Request> send_requests(outgoing.size(), MPI_REQUEST_NULL);
    std::vector<bool> recv_posted(incoming.size(), false);
    std::vector<bool> send_posted(outgoing.size(), false);
    
    // Strategy: Post ALL receives first to avoid deadlocks, then handle sends in dependency order
    // This is the standard MPI pattern: receivers must be ready before senders can proceed
    for (size_t recv_idx = 0; recv_idx < incoming.size(); recv_idx++) {
        const Edge& edge = incoming[recv_idx];
        MPI_Irecv(buffers[edge.recvIndex].data, buffers[edge.recvIndex].size,
                 MPI_BYTE, edge.from, 0, MPI_COMM_WORLD, &recv_requests[recv_idx]);
        recv_posted[recv_idx] = true;
    }
    
    // Now process sends in topological order, waiting for dependencies as needed
    for (size_t op_idx : topo_order) {
        if (op_idx < incoming.size()) {
            // This is a receive - already posted, skip
            continue;
        }
        
        // This is a send
        size_t send_idx = op_idx - incoming.size();
        const Edge& edge = outgoing[send_idx];
        
        // Check if this send depends on any receive
        int send_buffer = edge.sendIndex;
        for (size_t recv_idx = 0; recv_idx < incoming.size(); recv_idx++) {
            if (incoming[recv_idx].recvIndex == send_buffer && recv_posted[recv_idx]) {
                // Wait for the receive to complete before sending
                MPI_Wait(&recv_requests[recv_idx], MPI_STATUS_IGNORE);
                recv_posted[recv_idx] = false;  // Mark as completed
            }
        }
        
        // Post the send
        MPI_Isend(buffers[edge.sendIndex].data, buffers[edge.sendIndex].size,
                 MPI_BYTE, edge.to, 0, MPI_COMM_WORLD, &send_requests[send_idx]);
        send_posted[send_idx] = true;
    }
    
    // Wait for all remaining operations to complete
    for (size_t recv_idx = 0; recv_idx < incoming.size(); recv_idx++) {
        if (recv_posted[recv_idx]) {
            MPI_Wait(&recv_requests[recv_idx], MPI_STATUS_IGNORE);
        }
    }
    
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

