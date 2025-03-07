# Scaling Distance Labeling on Small-World Networks

This repository implements the Hub-Labeling algorithm _PSL_ described in the paper _Scaling Distance Labeling on Small-World Networks_ [1].
This implementation handles directed graphs, thus producing two labels per node (one outgoing / forward, and one incoming / backward label).

## Compiling and use

Compile the project using the `./compile.sh` program. This will create the `build` directory and build the executable there.

Using the program is as easy as calling `./PSL -h`, which will print all necessary information about how to use the program to create hub labels.

## Output Format
The algorithm can write its results to a plain-text file using a structured, line-based format. Here’s how to interpret the file:

- **Header Line**: 
The first line starts with the character `V` followed by the total number of vertices in the graph.
- **Label Lines**: 
For each vertex, the file contains two lines:
-- _Forward Labels_ (Outgoing): 
Begins with the letter `o` (short for “outgoing”). 
Followed by the vertex identifier. 
Then, a sequence of pairs where each pair represents a hub and its corresponding distance.
-- _Backward Labels_ (Incoming): 
Begins with the letter `i` (short for “incoming”). 
Followed by the same vertex identifier. 
Then, similarly, a sequence of hub–distance pairs.

**General Structure**: 
Each line is composed of space-separated tokens. 
The prefixes (V, o, i) help differentiate between the overall vertex count and the label types for each vertex. 
This format is designed to be both human-readable (when opened in a text editor) and easily parsed by automated tools.

### Example

The example in the [1] paper generates this output:

```bash
V 12
o 0 0 0
i 0 0 0
o 1 0 1 1 0 2 1
i 1 0 1 1 0 2 1
o 2 0 1 2 0
i 2 0 1 2 0
o 3 0 1 2 1 3 0 4 1
i 3 0 1 2 1 3 0 4 1
o 4 0 1 4 0
i 4 0 1 4 0
o 5 0 2 1 1 2 1 5 0
i 5 0 2 1 1 2 1 5 0
o 6 0 2 1 1 2 1 5 1 6 0
i 6 0 2 1 1 2 1 5 1 6 0
o 7 0 1 4 1 7 0
i 7 0 1 4 1 7 0
o 8 0 1 7 1 8 0 9 1
i 8 0 1 7 1 8 0 9 1
o 9 0 1 1 1 9 0
i 9 0 1 1 1 9 0
o 10 0 2 2 2 3 1 4 1 10 0
i 10 0 2 2 2 3 1 4 1 10 0
o 11 0 2 2 2 3 1 4 1 11 0
i 11 0 2 2 2 3 1 4 1 11 0
```

## Sources
[1] https://dl.acm.org/doi/10.1145/3299869.3319877
