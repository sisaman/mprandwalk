#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <thread>
#include <future>
#include "tqdm.h"
#include "cxxopts.hpp"

using namespace std;
using namespace cxxopts;

typedef vector<int> vi;

vector<string> nodes;
map<string, int> node_map;
map<int, map<char, vi>> network; // network[node][type] = adjlist
atomic<int> progress(0);

vector<string> split(const string &text, char sep) {
    vector<string> tokens;
    int start = 0, end = 0;
    while ((end = text.find(sep, start)) != string::npos) {
        tokens.push_back(text.substr(start, end - start));
        start = end + 1;
    }
    tokens.push_back(text.substr(start));
    return tokens;
}

inline const char get_node_type(int& node)  {
    return nodes[node][0];
}

inline int get_node_index(string& node) {
    if (node_map.count(node)) return node_map[node];
    else {
        node_map[node] = nodes.size();
        nodes.emplace_back(node);
        return nodes.size() - 1;
    }
}


vector<vi> do_metapath_randomwalk(
        const int& starting_node, const string& metapath, const string& exclude,
        const int& num_walks = 1000, const int& walk_length = 100) {

    vector<vi> total_walks;
    total_walks.reserve(num_walks);

    vector<int> walk;
    walk.reserve(walk_length);

    string path;
    for (int i = 0; i < ((walk_length / (metapath.length() - 1)) + 1); i++) {
        path += metapath.substr(1);
    }

    for (int walk_counter = 0; walk_counter < num_walks; walk_counter++) {

        walk.clear();
        int current_node = starting_node;

        for (int d = 0; d < walk_length; d++) {

            char node_type = get_node_type(current_node);
            if (exclude.find(node_type) == string::npos) {
                walk.push_back(current_node);
            }

            // walk to a random neighbor according to the path type
            vector<int> &neighbors = network[current_node][path[d]];
            int random_index = rand() % neighbors.size();
            current_node = neighbors[random_index];
        }

        total_walks.push_back(walk);
    }

    progress++;
    return total_walks;
}

void check_progress(int total) {
    tqdm bar;
    while (progress < total) {
        bar.progress(progress, total);
        this_thread::sleep_for(0.1s);
    }
    bar.finish();
    cout << "\ndone.\n";
}


int main(int argc, char **argv) {
    Options options("mprandwalk", "A meta-path-based random walk generator");
    options.add_options()
            ("i,input", "Input file as an adjacency list", value<string>(), "<Path>")
            ("o,output", "Output file for writing result", value<string>(), "<Path>")
            ("m,metapath", "Metapath to guide the random walk (each character denotes a node type)", value<string>(), "<String>")
            ("e,exclude", "Node types from metapath to be excluded from the result",
                    value<string>()->default_value(""), "<String>")
            ("n,num-walks", "Number of walks per node",
                    value<int>()->default_value("100"), "<Int>")
            ("w,walk-length", "Length of each walk",
                    value<int>()->default_value("100"), "<Int>")
            ("h,help", "Display help");
    auto result = options.parse(argc, argv);

    if (!(result.count("input") && result.count("output") && result.count("metapath")) || result.count("help")) {
        cout << options.help() << endl;
        cout << "Example:\n  mprandwalk --input data/adjlist.txt --output result/walks.txt "
                "--metapath vapav --exclude ap --num-walks 10 --walk-length 20" << endl;
        exit(0);
    }

    string input = result["input"].as<string>();
    string output = result["output"].as<string>();
    string metapath = result["metapath"].as<string>();
    string exclude = result["exclude"].as<string>();
    int num_walks = result["num-walks"].as<int>();
    int walk_length = result["walk-length"].as<int>();

    ifstream input_file(input);
    string source_str, line;
    int source, target;
    char node_type;

    cout << "reading network...\n";
    nodes.reserve(10000);

    while (getline(input_file, line)) {
        vector<string> tokens = split(line, ' ');
        source = get_node_index(tokens[0]);

        for (int i = 1; i < tokens.size(); i++) {
            target = get_node_index(tokens[i]);
            node_type = get_node_type(target);
            network[source][node_type].push_back(target);
        }
    }

    vector<int> starting_nodes;
    for (int i = 0; i < nodes.size(); i++) {
        if (metapath[0] == get_node_type(i))
            starting_nodes.emplace_back(i);
    }


    int total = starting_nodes.size();
    vector<future<vector<vi>>> futures;
    futures.reserve(total);

    cout << "running random walks...\n";
    thread progress_thread(&check_progress, total);

    for (int const &start_node : starting_nodes) {
//        do_metapath_randomwalk(start_node, metapath, exclude, num_walks, walk_length);
        futures.push_back(async(launch::async, do_metapath_randomwalk, start_node, metapath, exclude, num_walks, walk_length));
    }

    ofstream out_file(output);
    for (auto& f: futures) {
        vector<vi> walks = f.get();
        for (const vi& v : walks) {
            for ( const int& s : v) {
                out_file << nodes[s] << ' ';
            }

            out_file << endl;
        }
    }

    progress_thread.join();


    return 0;
}