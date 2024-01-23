#pragma once

#include <iostream>
#include <vector>
#include <iterator>
#include <fstream>
#include <istream>
#include <sstream>
#include <ios>
#include "compress_qmx.h" // QMX
#include <limits>
#include <stdexcept>
#include <x86intrin.h>
#include <algorithm>
#include <numeric>
#include <sys/time.h>
#include <unordered_map>
#include <unordered_set>
#include <string_view>
#include <chrono>


struct query {
    uint32_t id;
    std::vector<std::pair<uint32_t, uint32_t>> term_and_weight;

    query(uint32_t id, std::vector<std::pair<uint32_t, uint32_t>> tw) : id(id), term_and_weight(tw) {}
};

auto split_query_at_colon(std::string const& query_string)
    -> std::pair<std::string, std::string_view>
{
    // query id : terms (or ids)
    auto colon = std::find(query_string.begin(), query_string.end(), ':');
    std::string id;
    if (colon != query_string.end()) {
        id = std::string(query_string.begin(), colon);
    }
    auto pos = colon == query_string.end() ? query_string.begin() : std::next(colon);
    auto raw_query = std::string_view(&*pos, std::distance(pos, query_string.end()));
    return {std::move(id), raw_query};
}

std::vector<std::pair<uint32_t, uint32_t>> query_freqs(std::vector<uint32_t> terms) {
    std::vector<std::pair<uint32_t, uint32_t>> freqs;
    std::sort(terms.begin(), terms.end());
    for (size_t i = 0; i < terms.size(); ++i) {
        if (i == 0 || terms[i] != terms[i - 1]) {
            freqs.emplace_back(terms[i], 1);
        } else {
            freqs.back().second += 1;
        }
    }
    return freqs;
}

// int query with ID
std::vector<query> read_queries(std::ifstream &is) {
    std::vector<query> queries;
    std::string line;
    while (std::getline(is, line)) {

        std::vector<uint32_t> parsed_query;
        auto [id, raw_query] = split_query_at_colon(line);
        uint32_t int_id = std::stoul(id);
        std::string query_string(raw_query);
        std::stringstream my_terms(query_string);
        uint32_t s_term;
        while (my_terms >> s_term) {
            parsed_query.push_back(s_term);
        }

        // We now have the id and terms. Construct the weighted query
        auto freqs = query_freqs(parsed_query);
        queries.emplace_back(int_id, freqs);

    }
    std::cerr << "Read " << queries.size() << " queries.\n";
    return queries;
}

// (2): Load the TREC run
// 1048585	Q0	7617404	0	735	name
std::unordered_map<uint32_t, std::vector<std::pair<double, uint32_t>>>
load_trec_runs(std::ifstream &is) {
    
    std::cerr << "Loading TREC run. Assuming the run is sorted.\n";
    std::unordered_map<uint32_t, std::vector<std::pair<double, uint32_t>>> trec_runs;
    
    size_t count = 0;
    uint32_t qid, docid, rank;
    double score;
    std::string q0, run;
    while (is >> qid >> q0 >> docid >> rank >> score >> run) {
        trec_runs[qid].emplace_back(score, docid); 
        ++count;
    }

    std::cerr << "Read " << count << " lines for " << trec_runs.size() << " queries...\n";

    return trec_runs;

}

void read_lexicon_d (std::ifstream &is, 
                    std::unordered_map<std::string, uint32_t>& lexicon) {
    std::string term;
    size_t id = 0;
    while (is >> term) {
        ++id;
        lexicon.insert({term, id});
    }
    std::cerr << "Lexicon read: " << lexicon.size() << " terms" << std::endl;
}


void generate_stoplist(std::ifstream& is,
                    std::unordered_map<std::string, uint32_t>& lex,
                    std::unordered_set<uint32_t>& stoplist) {
    std::string term;
    while (is >> term) {
        auto lex_term = lex.find(term);
        // If not OOV
        if (lex_term != lex.end()) {
            stoplist.insert(lex_term->second);
        }
    }
    std::cerr << "Stoplist contains: " << stoplist.size() << " terms" << std::endl;
}
                                      
    

  // STOLEN FROM FASTPFORLIB: Please credit in final README/paper
template <class T> static void delta(T *data, const size_t size) {
  if (size == 0)
    throw std::runtime_error("delta coding impossible with no value!");
  for (size_t i = size - 1; i > 0; --i) {
    data[i] -= data[i - 1];
  }
}

template <class T> static void fastDelta(T *pData, const size_t TotalQty) {
  if (TotalQty < 5) {
    delta(pData, TotalQty); // no SIMD
    return;
  }

  const size_t Qty4 = TotalQty / 4;
  __m128i *pCurr = reinterpret_cast<__m128i *>(pData);
  const __m128i *pEnd = pCurr + Qty4;

  __m128i last = _mm_setzero_si128();
  while (pCurr < pEnd) {
    __m128i a0 = _mm_load_si128(pCurr);
    __m128i a1 = _mm_sub_epi32(a0, _mm_srli_si128(last, 12));
    a1 = _mm_sub_epi32(a1, _mm_slli_si128(a0, 4));
    last = a0;
    _mm_store_si128(pCurr++, a1);
  }

  if (Qty4 * 4 < TotalQty) {
    uint32_t lastVal = _mm_cvtsi128_si32(_mm_srli_si128(last, 12));
    for (size_t i = Qty4 * 4; i < TotalQty; ++i) {
      uint32_t newVal = pData[i];
      pData[i] -= lastVal; 
      lastVal = newVal;
    }
  }
}

inline double get_time_usecs()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}


