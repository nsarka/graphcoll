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

    GraphColl::Graph bcast;
    // src, dst, sendWeight, recvWeight
    // if weight is 0 then it's a "don't-care"
    for (int i = 1; i < comm_size; i++) {
        bcast.addEdge(root, i, 0, 0);
    }

    bcast.execute(rank, comm_size, my_rank_buffer_list);
    
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

    // Finalize the MPI environment.
    MPI_Finalize();

    return 0;
}

