#include <vector>
#include <mpi.h>
#include  "graph.hpp"

// To make it easy, assume the sendbuf is inplace
void test_ring_allgather(int rank, int comm_size) {
    int *recvbuf = (int*) malloc(sizeof(int) * comm_size);

    if (rank == 0) {
        std::cout << "Building test ring allgather graph with " << comm_size << " other ranks in the comm" << std::endl;
    }

    // Add my_data to a list of buffers we'll pass to the execution engine
    std::vector<GraphColl::Buffer> my_rank_buffer_list;
    GraphColl::Buffer buf;

    // Push recvbufs (one portion of the total recvbuf for each rank)
    // Buffer index i is at offset i of the recvbuf for an allgather
    for (int i = 0; i < comm_size; i++) {
        if (rank == i) {
            recvbuf[i] = rank; // Set the data inplace
        }
        buf.data = &recvbuf[i];
        buf.size = sizeof(int) * comm_size;
        buf.type = GraphColl::BufferType::Destination;
        my_rank_buffer_list.push_back(buf);
    }

    // Set up the ring allgather
    GraphColl::Graph allgather(rank, comm_size);
    for (int i = 0; i < comm_size; i++) {
        for (int j = 0; j < comm_size; j++) {
            // Add send to the next rank
            allgather.addEdge(rank, rank+1, (rank - j + comm_size) % comm_size, (rank - j - 1 + comm_size) % comm_size);
            // Add recv from the prev rank
            allgather.addEdge(rank-1, rank, (rank - j + comm_size) % comm_size, (rank - j - 1 + comm_size) % comm_size);
        }
    }

    std::cout << "Executing ring allgather graph..." << std::endl; 
    allgather.execute(my_rank_buffer_list);
    std::cout << "Done executing ring allgather graph" << std::endl; 
    
    // Validate
    for (int i = 0; i < comm_size; i++) {
        if (recvbuf[i] != i) {
            std::cout << "rank " << rank << ": recvbuf[" << i << "] incorrect: " << recvbuf[i] << std::endl;
        }
    }

    free(recvbuf);
}

void test_allgather(int rank, int comm_size) {
    int *sendbuf = (int*) malloc(sizeof(int));
    int *recvbuf = (int*) malloc(sizeof(int) * comm_size);

    *sendbuf = rank;

    if (rank == 0) {
        std::cout << "Building test allgather graph with " << comm_size << " other ranks in the comm" << std::endl;
    }

    // Add my_data to a list of buffers we'll pass to the execution engine
    // Since bcast only has one sendbuf, the list is of length 1
    std::vector<GraphColl::Buffer> my_rank_buffer_list;
    GraphColl::Buffer buf;

    // Push sendbuf
    buf.data = sendbuf;
    buf.size = sizeof(int) * 1;
    buf.type = GraphColl::BufferType::Source;
    my_rank_buffer_list.push_back(buf);

    // Push recvbufs (one portion of the total recvbuf for each rank)
    // Buffer index i is at offset i of the recvbuf for an allgather
    for (int i = 0; i < comm_size; i++) {
        buf.data = &recvbuf[i];
        buf.size = sizeof(int) * comm_size;
        buf.type = GraphColl::BufferType::Destination;
        my_rank_buffer_list.push_back(buf);
    }

    // Set up an allgather graph--there are effectively comm_size number of broadcasts
    GraphColl::Graph allgather(rank, comm_size);
    // src, dst, send/recv buf index in the buffer list
    // send buf index being 0 for each i,j means any time the sender goes to send, he'll always use my_rank_buffer_list[0]
    // recv index for each recv in an allgather is the rank of the sender (offset by 1 because the sendbuf is in the first spot)
    for (int i = 0; i < comm_size; i++) {
        for (int j = 0; j < comm_size; j++) {
            // include i=j because it must copy from send to recv buf
            allgather.addEdge(i, j, 0, i+1);
        }
    }

    std::cout << "Executing allgather graph..." << std::endl; 
    allgather.execute(my_rank_buffer_list);
    std::cout << "Done executing allgather graph" << std::endl; 
    
    // Validate
    for (int i = 0; i < comm_size; i++) {
        if (recvbuf[i] != i) {
            std::cout << "rank " << rank << ": recvbuf[" << i << "] incorrect: " << recvbuf[i] << std::endl;
        }
    }

    free(sendbuf);
    free(recvbuf);
}

void test_bcast(int rank, int comm_size) {
    int root = 0;
    int my_data = rank;

    if (rank == 0) {
        std::cout << "Building test bcast graph from root=" << root << " to " << comm_size << " other ranks in the comm" << std::endl;
    }

    // Add my_data to a list of buffers we'll pass to the execution engine
    // Since bcast only has one sendbuf, the list is of length 1
    std::vector<GraphColl::Buffer> my_rank_buffer_list;
    GraphColl::Buffer buf;
    buf.data = &my_data;
    buf.size = sizeof(int) * 1;
    if (rank == root) {
        buf.type = GraphColl::BufferType::Source;
    } else {
        buf.type = GraphColl::BufferType::Destination;
    }
    my_rank_buffer_list.push_back(buf);

    // Set up a graph with an edge from the root to every nonroot
    GraphColl::Graph bcast(rank, comm_size);
    // src, dst, weights (0 means "don't-care")
    for (int i = 1; i < comm_size; i++) {
        bcast.addEdge(root, i, 0, 0);
    }

    bcast.execute(my_rank_buffer_list);
    
    // Validate
    std::vector<int> gathered(comm_size, -1);
    MPI_Gather(&my_data, 1, MPI_INT, gathered.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank == 0) {
        std::cout << "Graph execution results:" << std::endl;
        for (int i = 0; i < comm_size; ++i) {
            if (gathered[i] != root) {
                std::cout << "  Rank " << i << ": " << gathered[i] << "  <-- INCORRECT" << std::endl;
            } else {
                std::cout << "  Rank " << i << ": " << gathered[i] << std::endl;
            }
        }
    }
}

int main(int argc, const char* argv[]) {
    MPI_Init(NULL, NULL);

    int rank, comm_size;
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    //test_bcast(rank, comm_size);
    //test_allgather(rank, comm_size);
    test_ring_allgather(rank, comm_size);

    // Finalize the MPI environment.
    MPI_Finalize();

    return 0;
}

