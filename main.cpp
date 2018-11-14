#include <iostream>
#include <unordered_map>
#include <optional>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

struct Parameters {
    enum Command {
        Distinct,
        Top
    } command;
    std::string file_name;
    std::optional<u_int64_t> nb_queries;
    std::optional<u_int64_t> from;
    std::optional<u_int64_t> to;
};

// Note: here, in real life, I would have used boost::program_options 
Parameters parse_params(int argc, char* argv[]) {
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
    uint extra_params_idx = 2;
    if (command == "top") {
        p.command = Parameters::Command::Top;
        const uint64_t val = std::strtoull(argv[2], nullptr, 10);
        p.nb_queries = val;
        extra_params_idx ++;
    } else if (command == "distinct") {
        p.command = Parameters::Command::Distinct;
    } else {
        bad_usage();
    }

    for (uint i = extra_params_idx; i < argc; i++) {
        if (strcmp(argv[i], "--from") == 0) {
            if (i+1 >= argc) {
                bad_usage();
            }
            // NOTE: in real life I would most likely have used boot::lexical_cast
            // the strtoull function has for example the huge disadvantage of returning 0 on error
            // thus we cannot easily tell if the input is bad or 0
            const uint64_t val = std::strtoull(argv[i+1], nullptr, 10);
            p.from = {val};
            i++;
        }
        else if (strcmp(argv[i], "--to") == 0) {
            if (i+1 >= argc) {
                bad_usage();
            }
            const uint64_t val = std::strtoull(argv[i+1], nullptr, 10);
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

// Note: in real life I ould have used boost::spirit to parse the TSV, this is by no mean a real tsv parser
std::optional<std::pair<u_int64_t, std::string>> parse_line(const std::string& line) {
    std::string timestamp_str, url;
    std::istringstream tokenStream(line);

    if (! std::getline(tokenStream, timestamp_str, '\t')) {
        return std::nullopt;
    }
    if (! std::getline(tokenStream, url, '\t')) {
        return std::nullopt;
    }

    const u_int64_t ts = std::strtoull(timestamp_str.c_str(), nullptr, 10);

    if (ts == 0) {
        // 0 is not a possible value for the dataset, we can consider this has been wrongly parsed
        // std::cout << "not parsed: " << url << "| ts: " << timestamp_str << std::endl;
        return std::nullopt;
    }
    return {{ts, url}};
}

bool keep_line(u_int64_t timestamp, const Parameters params) {
    if (params.from && timestamp < *params.from) {
        return false;
    }
    if (params.to && timestamp > *params.to) {
        return false;
    }
    return true;
}

std::unordered_map<std::string, uint> count_entries(const Parameters& params) {
    std::ifstream file;
    file.open(params.file_name);
    if (! file.is_open()) {
        std::cerr << "impossible to read file " << params.file_name << std::endl;
        std::exit(1);
    }

    auto entries = std::unordered_map<std::string, uint>{};
    std::string line;
    while (std::getline(file, line)) {
        // std::cout << "line: " << line << std::endl;
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

void compute_top(const Parameters& params) {
    if (! params.nb_queries) {
        return;
    }
    const auto counted_entries = count_entries(params);

    auto sorted_entries = std::vector<std::pair<std::string, u_int64_t>>{counted_entries.size()};
    
    for (const auto& p: counted_entries) {
        sorted_entries.push_back(p);
    }
    std::sort(sorted_entries.begin(), sorted_entries.end(), [](const auto& a, const auto&b) { return a.second > b.second; });

    for (uint i = 0; i < sorted_entries.size() && i < *params.nb_queries; i++) {
        std::cout << sorted_entries[i].first << " " << sorted_entries[i].second << std::endl;
    }
}

void compute_distinct(const Parameters& params) {
    const auto counted_entries = count_entries(params);

    std::cout << counted_entries.size() << std::endl;
}

int main(int argc, char* argv[]) {
    const auto params = parse_params(argc, argv);

    if (params.command == Parameters::Command::Distinct) {
        compute_distinct(params);
    }
    if (params.command == Parameters::Command::Top) {
        compute_top(params);
    }
    return 0;
}