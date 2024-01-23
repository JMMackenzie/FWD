#include "document_index.hpp"
#include "util.hpp"

void printUsage(const std::string &programName) {
  std::cerr << "Usage: " << programName
            << " <docvector> <query_file> <run_file>"
            << std::endl;
}

int main(int argc, const char **argv) {

    std::string programName = argv[0];
    if (argc != 4) {
        printUsage(programName);
        return 1;
    }

    std::string forward_index_filename = argv[1];
    std::string query_filename = argv[2];
    std::string run_filename = argv[3];

    // (1): Load the index
    document_index forward_index;
    std::cerr << "Loading forward index from " << forward_index_filename << std::endl;
    forward_index.load(forward_index_filename);


    // (2): Load the TREC run
    // trec runs is a std::unordered_map<uint32_t, std::vector<std::pair<double, uint32_t>>>
    std::ifstream intrec(run_filename);
    auto trec_runs = load_trec_runs(intrec);
    
    // (3): Load the Query
    std::ifstream inq(query_filename);
    auto queries = read_queries(inq);

    double total_micro = 0.0f;

    // (4): For each query, re-rank
    for (auto const &query: queries) {

        auto now = get_time_usecs();

        auto initial_retrieval = trec_runs[query.id];

        // Result is a std::vector<std::pair<uint32_t, double>>
        auto result = forward_index.run_query(initial_retrieval, query);

        for (size_t i = 0; i < result.size(); ++i) {
          std::cout << query.id << " Q0 " << result[i].first << " " << i+1 << " " << result[i].second << " rerank\n";
        }

        auto time = get_time_usecs() - now;
        //total_micro += time;
        std::cerr << query.id << " " << time * 0.001 << "\n";
    }

    //std::cerr << "Info: Reranking took on average " << (total_micro/queries.size() )*0.001 << " milliseconds per query... \n";
}
     
