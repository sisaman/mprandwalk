# mprandwalk
A meta-path-based random walk generator

### Usage:  
    mprandwalk [OPTION...]

    -i, --input <Path>       Input file as an adjacency list  
    -o, --output <Path>      Output file for writing result  
    -m, --metapath <String>  Metapath to guide the random walk (each character denotes a node type)  
    -e, --exclude <String>   Node types from metapath to be excluded from the result (default: "")  
    -n, --num-walks <Int>    Number of walks per node (default: 100)  
    -w, --walk-length <Int>  Length of each walk (default: 100)  
    -h, --help               Display help  

### Example:  
  ``mprandwalk --input data/adjlist.txt --output result/walks.txt --metapath vapav --exclude ap --num-walks 10 --walk-length 20  ``

### Input
The supported input file format is an adjacency list. Each node in the file is represented 
as a ``<type>+<id>``, where ``<type>`` is
a character denoting the type of the node and ``<id>`` is a string denoting the node id. Example: vKDD

Each line of the input file starts with a source node and the remaining words on 
the same line are neighbors of the source node:
     
     source_node neighbor_node_1 neighbor_node_2 ...
     
### Output
Each line in the output corresponds to a single random walk. 
