#include <iostream>
#include <unordered_map>
#include <optional>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <map>

#include "fixed_size_pq.h"

struct Parameters {
    enum Command {
        Distinct,
        Top
    } command;
    std::string file_name;
    std::optional<size_t> nb_queries;
    std::optional<size_t> from;
    std::optional<size_t> to;
};

using CountedEntries = std::unordered_map<std::string, size_t>;

static constexpr size_t HEURISTIC_LIMIT_FOR_PRIORITY_QUEUE = 100;

// Note: here, in a real project, I would have used boost::program_options 
Parameters parse_params(size_t argc, char* argv[]) {
    const auto usage = R"(
invalid number of argument, usage:

hnStat distinct [--from TIMESTAMP] [--to TIMESTAMP] <INPUT_FILE>

or

hnStat top <nb_top_queries> [--from TIMESTAMP] [--to TIMESTAMP] <INPUT_FILE>
)";
    const auto bad_usage = [&usage]() { std::cerr << usage << std::endl; std::exit(1); };
    if (argc < 3) {
        bad_usage();
    }

    auto p = Parameters{};
    const std::string command = argv[1];
    size_t extra_params_idx = 2;
    if (command == "top") {
        p.command = Parameters::Command::Top;
        const size_t val = std::strtoull(argv[2], nullptr, 10);
        p.nb_queries = val;
        extra_params_idx ++;
    } else if (command == "distinct") {
        p.command = Parameters::Command::Distinct;
    } else {
        bad_usage();
    }

    for (size_t i = extra_params_idx; i < argc; i++) {
        if (strcmp(argv[i], "--from") == 0) {
            if (i+1 >= argc) {
                bad_usage();
            }
            // NOTE: in a real project I would most likely have used boot::lexical_cast
            // the strtoull function has for example the huge disadvantage of returning 0 on error
            // thus we cannot easily tell if the input is bad or 0
            const size_t val = std::strtoull(argv[i+1], nullptr, 10);
            p.from = {val};
            i++;
        }
        else if (strcmp(argv[i], "--to") == 0) {
            if (i+1 >= argc) {
                bad_usage();
            }
            const size_t val = std::strtoull(argv[i+1], nullptr, 10);
            p.to = {val};
            i++;
        }
        else {
            p.file_name = argv[i];
        }
    }

    if (p.file_name.empty()) {
        bad_usage();
    }

    return p;
}

// Note: in real life I ould have used a real TSV parser or boost::spirit to parse the TSV, this is by no mean a real tsv parser
std::optional<std::pair<size_t, std::string>> parse_line(const std::string& line) {
    std::string timestamp_str, url;
    std::istringstream tokenStream(line);

    if (! std::getline(tokenStream, timestamp_str, '\t')) {
        return std::nullopt;
    }
    if (! std::getline(tokenStream, url, '\t')) {
        return std::nullopt;
    }

    const size_t ts = std::strtoull(timestamp_str.c_str(), nullptr, 10);

    if (ts == 0) {
        // 0 is not a possible value for the dataset, we can consider this has been wrongly parsed by strtoull
        // std::cout << "not parsed: " << url << "| ts: " << timestamp_str << std::endl;
        return std::nullopt;
    }
    return {{ts, url}};
}

bool keep_line(size_t timestamp, const Parameters& params) {
    if (params.from && timestamp < *params.from) {
        return false;
    }
    if (params.to && timestamp > *params.to) {
        return false;
    }
    return true;
}

CountedEntries count_entries(const Parameters& params) {
    std::ifstream file;
    file.open(params.file_name);
    if (! file.is_open()) {
        std::cerr << "impossible to read file " << params.file_name << std::endl;
        std::exit(1);
    }

    auto entries = CountedEntries{};
    std::string line;
    while (std::getline(file, line)) {
        const auto ts_url = parse_line(line);

        if (! ts_url) {
            // the line is not correctly formated, we skip it
            // std::cerr << "line " << line << " is not correctly formated, we skip it" << std::endl;
            continue;
        }
        if (! keep_line(ts_url->first, params)) {
            // the timestamp is not in the filtered range, we skip it
            continue;
        }
        entries[ts_url->second] += 1;
    }
            
    return entries;
}

void global_print_top_entries(const CountedEntries& counted_entries, size_t top) {
    auto sorted_entries = std::vector<std::pair<std::string, size_t>>{};
    sorted_entries.reserve(counted_entries.size());
    
    for (const auto& p: counted_entries) {
        sorted_entries.push_back(p);
    }

    const auto limit = std::min(top, sorted_entries.size());
    std::sort(sorted_entries.begin(), sorted_entries.end(), [](const auto& a, const auto& b) { return a.second > b.second; });

    // the partial_sort seems more interesing, but it seems to be slower on the benches.
    // it might be because of different sort algorithm used
    // std::partial_sort(sorted_entries.begin(), sorted_entries.end(), sorted_entries.begin() + limit,
    // [](const auto& a, const auto& b) { return a.second > b.second; });

    for (size_t i = 0; i < limit; i++) {
        std::cout << sorted_entries[i].first << " " << sorted_entries[i].second << std::endl;
    }
}

void priority_queue_print_top_entries(const CountedEntries& counted_entries, size_t top) {
    auto fixed_size_heap = FixedSizePriorityQueue{std::min(top, counted_entries.size())};

    for (const auto& p: counted_entries) {
        fixed_size_heap.add(p);
    }
    fixed_size_heap.print();
}

/*
 * The compute_top function uses the same unordered_map as compute_distinct
 * Once the map is created we only have to extract the top entries
 * 
 * To do it 2 methods are implemented:
 * 
 * - Insert all the elements in a vector and sort them all to get the top k element
 *      Complexity: n insertion in a vector + sort in 0(n*log(n)) = 0(n*log(n))
 * 
 * - Use a custom priority queue
 *      Complexity: 
 *          It depends on the value of k=nb_queries
 *          if k is big -> for each element we insert it in a vector at a given position 
 *              this is 0(n*log(n) since we need to:
 *                  * find the right position in the vector -> 0(log(n))
 *                  * shift all the element after the inserted one (O(n))
 *              => 0(n*log(n))
 * 
 *          so the worst case complexity is bad, but for small k value very few insertion will be made in practice
 *          And since we use a fixed size vector, no allocation are made, thus it's more efficient for small k values
 *          (and the memory footprint is also reduced)
 *          
 */
void compute_top(const Parameters& params) {
    if (! params.nb_queries) {
        return;
    }
    const auto counted_entries = count_entries(params);

    // for low values of nb_queries, we use a priority queue
    // for high value, it's more efficient to sort all the elements
    // once in a vector.
    // the value HEURISTIC_LIMIT_FOR_PRIORITY_QUEUE has been defined after some benchmarks on the dataset,
    // but it's only for the test purpose, in a real project the value would be set with more care (and maybe with a ratio)
    if (*params.nb_queries <= HEURISTIC_LIMIT_FOR_PRIORITY_QUEUE) {
        priority_queue_print_top_entries(counted_entries, *params.nb_queries);
    } else {
        global_print_top_entries(counted_entries, *params.nb_queries);
    }
}

/*
 * The compute_distinct function is quite straightforward
 * we store all the entries in a unordered_map and we output the size of the resulting map.
 * For each line in the file, we have to insert it in an unordered map
 * the worst case complexity of a unordered_map insertion is O(n) but in average it's more like constant time
 * the worst case complexity of it is O(n²) and the average complexity is O(n)
 * n being the number of lines in the file
 * 
 * Note: using a map instead of a unordered_map would lead to a worst case complexity of O(n*log(n))
 * but in practive it's 50% slower (benchmarked on the dataset)
 **/
void compute_distinct(const Parameters& params) {
    const auto counted_entries = count_entries(params);

    std::cout << counted_entries.size() << std::endl;
}

int main(int argc, char* argv[]) {
    const auto params = parse_params(static_cast<size_t>(argc), argv);

    if (params.command == Parameters::Command::Distinct) {
        compute_distinct(params);
    }
    if (params.command == Parameters::Command::Top) {
        compute_top(params);
    }
    return 0;
}