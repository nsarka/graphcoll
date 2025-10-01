#include <vector>
#include <mpi.h>
#include  "graph.hpp"

int main(int argc, const char* argv[]) {
    MPI_Init(NULL, NULL);

    int rank, comm_size;
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int root = 0;
    int my_data = rank;
    std::cout << "Building test bcast graph from root=" << root << " to " << comm_size << " other ranks in the comm" << std::endl;

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

    GraphColl::Graph bcast;
    // src, dst, sendWeight, recvWeight
    // if weight is 0 then it's a "don't-care"
    for (int i = 1; i < comm_size; i++) {
        bcast.addEdge(root, i, 0, 0);
    }

    bcast.execute(rank, comm_size, my_rank_buffer_list);
    
    // Validate
    if (my_data == root) {
        std::cout << "It worked!" << std::endl;
    } else {
        std::cout << "Expected " << root << ", got " << my_data << std::endl;
    }

    // Finalize the MPI environment.
    MPI_Finalize();

    return 0;
}

