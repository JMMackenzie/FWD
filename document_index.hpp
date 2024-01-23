#pragma once

#include "util.hpp"
#include "document_vector.hpp"

//#define INTERP

class document_index {
 
  private: 
    // m_doc_vectors[i] returns the document_vector for document i
    std::vector<document_vector> m_doc_vectors;
    uint32_t m_size;
    uint32_t no_terms;
    std::vector<uint32_t> m_external_to_internal;
    std::vector<uint32_t> m_internal_to_external;

   // Helper struct for our RM calculations
    struct vector_wrapper {
        typename document_vector::const_iterator cur;
        typename document_vector::const_iterator end;
        double doc_score;
        uint32_t doc_len;
        vector_wrapper() = default;
        vector_wrapper(document_vector& dv, double score = 0.0f) 
                      : doc_score(score) {
            cur = dv.begin();
            end = dv.end();
            doc_len = dv.doclen();
        } 
    };

  public:
    document_index() : m_size(0) {}

    // Build a document index from ds2i files
    document_index(std::string ds2i_basename, std::unordered_set<uint32_t>& stoplist) {
        // Temporary 'plain' index structures
        std::vector<std::vector<uint32_t>> plain_terms;
        std::vector<std::vector<uint32_t>> plain_freqs;

       
        // Read DS2i document file prefix
        std::ifstream docs (ds2i_basename + ".docs", std::ios::binary);
        std::ifstream freqs (ds2i_basename + ".freqs", std::ios::binary);

        // Read first sequence from docs
        uint32_t one;
        docs.read(reinterpret_cast<char *>(&one), sizeof(uint32_t));
        docs.read(reinterpret_cast<char *>(&m_size), sizeof(uint32_t));
        m_doc_vectors.reserve(m_size); // Note: not yet constructed
        plain_terms.resize(m_size); 
        plain_freqs.resize(m_size);

        // Read the document map
        std::ifstream docids(ds2i_basename + ".documents");
        uint32_t docname;
        uint32_t internal_id = 0;
        m_external_to_internal.resize(m_size);
        m_internal_to_external.resize(m_size);
        while (docids >> docname) {
          m_external_to_internal[docname] = internal_id;
          m_internal_to_external[internal_id] = docname;
          internal_id++;
        }

        std::cerr << "Read " << internal_id << " document names...\n";
 
        uint32_t d_seq_len = 0;
        uint32_t f_seq_len = 0;
        uint32_t term_id = 0;
        // Sequences are now aligned. Walk them.        
        while(!docs.eof() && !freqs.eof()) {

            // Check if the term is stopped
            bool stopped = (stoplist.find(term_id) != stoplist.end());


            docs.read(reinterpret_cast<char *>(&d_seq_len), sizeof(uint32_t));
            freqs.read(reinterpret_cast<char *>(&f_seq_len), sizeof(uint32_t));
            if (d_seq_len != f_seq_len) {
                std::cerr << "ERROR: Freq and Doc sequences are not aligned. Exiting."
                          << std::endl;
                exit(EXIT_FAILURE);
            }
            uint32_t seq_count = 0;
            uint32_t docid = 0;
            uint32_t fdt = 0;
            while (seq_count < d_seq_len) {
                docs.read(reinterpret_cast<char *>(&docid), sizeof(uint32_t));
                freqs.read(reinterpret_cast<char *>(&fdt), sizeof(uint32_t));
                // Only emplace unstopped terms
                if (!stopped) {
                    plain_terms[docid].emplace_back(term_id);
                    plain_freqs[docid].emplace_back(fdt);
                }
                ++seq_count;
            }
            ++term_id;
        }
        no_terms = term_id;
        
        std::cerr << "Read " << m_size << " lists and " << term_id 
                  << " unique terms. Compressing.\n";

        // Now iterate the plain index, compress, and store
        for (size_t i = 0; i < m_size; ++i) {
            m_doc_vectors.emplace_back(i, plain_terms[i], plain_freqs[i]);
        }

    } 

    void serialize(std::ostream& out) {
        out.write(reinterpret_cast<const char *>(&no_terms), sizeof(no_terms));
        out.write(reinterpret_cast<const char *>(&m_size), sizeof(m_size));
        for (size_t i = 0; i < m_size; ++i) {
            m_doc_vectors[i].serialize(out);
        }
        out.write(reinterpret_cast<const char*>(&m_external_to_internal[0]), m_size * sizeof(uint32_t));
        out.write(reinterpret_cast<const char*>(&m_internal_to_external[0]), m_size * sizeof(uint32_t));
    }

    void load(std::string inf) {
        std::ifstream in(inf, std::ios::binary);
        load(in);
    }

    void load(std::istream& in) {
        in.read(reinterpret_cast<char *>(&no_terms), sizeof(no_terms));
        in.read(reinterpret_cast<char *>(&m_size), sizeof(m_size));
        m_external_to_internal.resize(m_size);
        m_internal_to_external.resize(m_size);
        m_doc_vectors.resize(m_size);
        for (size_t i = 0; i < m_size; ++i) {
            m_doc_vectors[i].load(in);
        }
        in.read(reinterpret_cast<char*>(&m_external_to_internal[0]), m_size * sizeof(uint32_t));
        in.read(reinterpret_cast<char*>(&m_internal_to_external[0]), m_size * sizeof(uint32_t));
    }

    uint32_t in_to_ex(const uint32_t id) const {
      return m_internal_to_external[id];
    }

    uint32_t ex_to_in(const uint32_t id) const {
      return m_external_to_internal[id];
    }


    void dump_to_plaintext() {

        for (size_t docid = 0; docid < m_size; ++docid) {
            std::cout << docid;
            document_vector::const_iterator cur = m_doc_vectors[docid].begin(); // Automatically decompresses
            document_vector::const_iterator end = m_doc_vectors[docid].end();
        
            while(cur != end) {
                std::cout << " " << cur.termid();
                cur.next();
            }
            std::cout << std::endl;
        }
 
    }

    // DaaT traversal for RM -- Non sort version
    std::vector<std::pair<uint32_t, double>> 
    get_rm_daat(std::vector<vector_wrapper*>& docvectors) {
    
        std::vector<std::pair<uint32_t, double>> result;    
        
        auto min = std::min_element(docvectors.begin(),
                                    docvectors.end(),
                                    [] (const vector_wrapper* lhs,
                                        const vector_wrapper* rhs) {
                                          return lhs->cur.termid() < rhs->cur.termid();
                                        });

        uint32_t cur_term = (*min)->cur.termid(); 
        // Main scoring loop
        while (cur_term < no_terms) {

            uint32_t next_term = no_terms;
            double score = 0;
            for (size_t i = 0; i < docvectors.size(); ++i) {
                if (docvectors[i]->cur.termid() == cur_term) {
                    score += docvectors[i]->doc_score * (docvectors[i]->cur.freq() / (docvectors[i]->doc_len * 1.0f));
                    docvectors[i]->cur.next();
                }
                if (docvectors[i]->cur.termid() < next_term) {
                    next_term = docvectors[i]->cur.termid();
                }
            }
            // Add result
            result.emplace_back(cur_term, score);
            cur_term = next_term;
        }  
        
        // Finalize results and return
        std::sort(result.begin(), result.end(), 
                  [](const std::pair<uint32_t, double> &lhs,
                     const std::pair<uint32_t, double> &rhs) {
                      return lhs.second > rhs.second;
                  });
        return result;
    } 



    std::vector<std::pair<uint32_t, double>>
    rm_expander (std::vector<std::pair<double, uint64_t>>& initial_retrieval,
                 size_t terms_to_expand = 0) {

        // 0. Result init
        std::vector<std::pair<uint32_t, double>> result;

        // 1. Prepare the document vectors
        std::vector<vector_wrapper> feedback_vectors(initial_retrieval.size());
        std::vector<vector_wrapper*> feedback_ptr;
        for (size_t i = 0; i < initial_retrieval.size(); ++i) {
            double score = initial_retrieval[i].first;
            uint64_t docid = initial_retrieval[i].second;
            feedback_vectors[i] = vector_wrapper(m_doc_vectors[docid], score);
            feedback_ptr.emplace_back(&(feedback_vectors[i]));
        }

        // Get the result and resize if needed
        result = get_rm_daat(feedback_ptr);
        // Only shrink -- do not allow growth of result
        if (terms_to_expand > 0 && result.size() > terms_to_expand)
          result.resize(terms_to_expand);
          
        return result;
        
    }


    std::vector<std::pair<uint32_t, double>>
    run_query (std::vector<std::pair<double, uint32_t>>& initial_retrieval,
            const query& query) {

        auto sorted_query = query.term_and_weight;
        
        // 0. Result init
        std::vector<std::pair<uint32_t, double>> result;
        result.reserve(initial_retrieval.size());

#ifdef INTERP
        double min_rerank_score = 1000000.0f;
        double max_rerank_score = 0.0f;
#endif
          
        // 1. For each document
        for (size_t i = 0; i < initial_retrieval.size(); ++i) {
            double new_score = 0;
            double initial_score = initial_retrieval[i].first;
            uint32_t external_docid = initial_retrieval[i].second;
            uint32_t target_docid = ex_to_in(external_docid);
            auto cur = m_doc_vectors[target_docid].begin();
            auto end = m_doc_vectors[target_docid].end();


            // 2. For each term
            for (size_t j = 0; j < sorted_query.size(); ++j) {
                auto target_term = sorted_query[j].first;

                // Find the term in the doc if it exists
                while(cur != end && cur.termid() < target_term) {
                    cur.next();
                }
                // Could do interpolation if we wanted...
                if (cur != end && cur.termid() == target_term) {
                    new_score += sorted_query[j].second * cur.freq(); // XXX We only do sum of impacts here for now...
                }
            }
#ifdef INTERP
            min_rerank_score = std::min(min_rerank_score, new_score);
            max_rerank_score = std::max(max_rerank_score, new_score);
#endif
            result.push_back({external_docid, new_score});
        }

#ifdef INTERP
        // Interpolate results
        double max_input_score = 0.0f;
        double min_input_score = 1000.0f;
        if (initial_retrieval.size() > 0) {
          max_input_score = initial_retrieval[0].first;
          min_input_score = initial_retrieval.back().first;
        }
        const double ALPHA = 0.5f;
        for (size_t j = 0; j < result.size(); ++j) {
                         // This is the TILDE part
          double score = ALPHA * (result[j].second - min_rerank_score)/(max_rerank_score - min_rerank_score)
            + (1 - ALPHA) * (initial_retrieval[j].first - min_input_score)/(max_input_score - min_input_score);
          result[j].second = score;
        }
#endif

        // Now sort the result
        std::sort(result.begin(), result.end(), [](const auto& l, const auto& r){ return l.second > r.second; });
        return result;
        
    }



    void test_iteration(uint32_t docid) {
        
        document_vector::const_iterator cur = m_doc_vectors[docid].begin(); // Automatically decompresses
        document_vector::const_iterator end = m_doc_vectors[docid].end();
        
        while(cur != end) {
            std::cerr << cur.termid() << "," << cur.freq() << "\n";
            cur.next();
        }
        std::cerr << "Exiting here now.\n";
    }
};
