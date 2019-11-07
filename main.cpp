#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <thread>
//#include <mutex>
#include <future>
//#include "progressbar.hpp"
#include "tqdm.h"
#include "cxxopts.hpp"

using namespace std;
using namespace cxxopts;

typedef vector<string> vs;

map<string, map<char, vs>> network; // network[node][type] = adjlist
//progressbar* bar;
//tqdm* bar;
atomic<int> progress(0);
mutex mx;

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


vector<vs> do_metapath_randomwalk(
        const string& starting_node, const string& metapath, const string& exclude,
        const int& num_walks = 1000, const int& walk_length = 100) {

    vector<vs> total_walks;
    total_walks.reserve(num_walks);

    vector<string> walk;
    walk.reserve(walk_length);

    string path;
    for (int i = 0; i < ((walk_length / (metapath.length() - 1)) + 1); i++) {
        path += metapath.substr(1);
    }

    for (int walk_counter = 0; walk_counter < num_walks; walk_counter++) {

        walk.clear();
        string current_node = starting_node;

        for (int d = 0; d < walk_length; d++) {

            char node_type = current_node[0];
            if (exclude.find(node_type) == string::npos) {
                walk.push_back(current_node);
            }

            // walk to a random neighbor according to the path type
            vs &neighbors = network[current_node][path[d]];
            int random_index = rand() % neighbors.size();
            current_node = neighbors[random_index];
        }

        total_walks.push_back(walk);
    }

//    mx.lock();
    progress++;
////    bar->update();
//    bar->progress(1);
//    mx.unlock();

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
    Options options("mp2vecwalk", "Building random walk file to be used by metapath2vec");
    options.add_options()
            ("input", "Input file as an edge list", value<string>())
            ("output", "Output path for writing result", value<string>())
            ("metapath", "Metapath for walk", value<string>())
            ("exclude", "Node types to be excluded in the walk (default="")",
                    value<string>()->default_value(""))
            ("num-walks", "Number of walks per node (default=1000)",
                    value<int>()->default_value("100"))
            ("walk-length", "Length of each walk (default=100)",
                    value<int>()->default_value("100"));
    auto result = options.parse(argc, argv);
    string input = result["input"].as<string>();
    string output = result["output"].as<string>();
    string metapath = result["metapath"].as<string>();
    string exclude = result["exclude"].as<string>();
    int num_walks = result["num-walks"].as<int>();
    int walk_length = result["walk-length"].as<int>();

    ifstream input_file(input);
    string node, source, line;

    cout << "reading network...\n";
    vector<string> tokens;

    while (getline(input_file, line)) {
        tokens = split(line, ' ');
        source = tokens[0];
        for (int i = 1; i < tokens.size(); i++)
            network[source][tokens[i][0]].push_back(tokens[i]);
    }

    vector<string> node_list;
    for (auto const &pair : network) {
        if (pair.first[0] == metapath[0])
            node_list.emplace_back(pair.first);
    }


    int total = node_list.size();
//    cout << total << endl;
//    bar = new progressbar(total);
//    bar = new tqdm(total);
    vector<future<vector<vs>>> futures;
    futures.reserve(total);

    cout << "running random walks...\n";
    thread progress_thread(&check_progress, total);

    for (string const &start_node : node_list) {
        futures.push_back(async(launch::async, do_metapath_randomwalk, start_node, metapath, exclude, num_walks, walk_length));
    }

//    cout << "\nwriting result...";
    ofstream out_file(output);
    for (auto& f: futures) {
        vector<vs> walks = f.get();
        for (const vs& v : walks) {
            for ( const string& s : v) {
                out_file << s << ' ';
            }

            out_file << endl;
        }
    }

    progress_thread.join();


    return 0;
}