#include "document_index.hpp"
#include "util.hpp"

void printUsage(const std::string &programName) {
  std::cerr << "Usage: " << programName
            << " <docvector>"
            << std::endl;
}

int main(int argc, const char **argv) {

    std::string programName = argv[0];
    if (argc != 2) {
        printUsage(programName);
        return 1;
    }

    std::string forward_index_filename = argv[1];

    // (1): Load the index
    document_index forward_index;
    std::cerr << "Loading forward index from " << forward_index_filename << std::endl;
    forward_index.load(forward_index_filename);

    forward_index.dump_to_plaintext();

}
     
