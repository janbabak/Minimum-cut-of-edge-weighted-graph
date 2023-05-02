#include <cassert>
#include <chrono>
#include <climits>
#include <cmath>
#include <iostream>
#include <vector>
#include <omp.h>

#include "Graph.cpp"
#include "TestData.cpp"

using namespace std;
using namespace std::chrono;

const short IN_X = 1;
const short IN_Y = 0;
const short NOT_DECIDED = -1;

long minimalSplitWeight = LONG_MAX;   // best found weight
short *minimalSplitConfig = nullptr;  // best found configuration
int smallerSetSize;                   // size of smaller set X
int configLength;                     // number of vertexes
Graph graph;

/**
 * Print configuration for debug purposes.
 * @param config vertexes configuration to print
 * @param os output stream
 */
void printConfig(short *config, ostream &os = cout) {
    os << "[";
    for (int i = 0; i < configLength; i++) {
        os << config[i];
        if (i == configLength - 1) {
            os << "]" << endl;
        } else {
            os << ", ";
        }
    }
}

/**
 * Compute the number of vertexes in both sets (X and Y).
 * @param config vertexes configuration
 * @return pair, where the first item is size of X and the second one the is size of Y
 */
[[nodiscard]] pair<int, int> computeSizeOfXAndY(short *config) {
    int countX = 0;
    int countY = 0;

    for (int i = 0; i < configLength; i++) {
        if (config[i] == IN_X) {
            countX++;
        } else if (config[i] == IN_Y) {
            countY++;
        } else {
            return make_pair(countX, countY);  // all following vertexes are not decided
        }
    }

    return make_pair(countX, countY);
}

/**
 * Compute sum of weights of edges, that has one vertex in set X and second one in set Y.
 * @param config vertexes configuration
 * @return weight of split
 */
[[nodiscard]] long computeSplitWeight(short *config) {
    long weight = 0;

    for (int i = 0; i < graph.edgesSize; i++) {
        if (config[graph.edges[i].vertexId1] == IN_X && config[graph.edges[i].vertexId2] == IN_Y) {
            weight += graph.edges[i].weight;
        }
    }

    return weight;
}

/**
 * Compute lower bound of undecided part of vertexes.
 * @param config vertexes configuration
 * @param indexOfFirstUndecided index of first vertex from configuration which is not assigned to X
 * or Y
 * @param weightOfDecidedPart weight of the split computed only from decided vertexes
 * @return lower bound of weight
 */
[[nodiscard]] long lowerBoundOfUndecidedPart(short *config, int indexOfFirstUndecided,
                                             long weightOfDecidedPart) {
    long lowerBound = 0;

    for (int i = indexOfFirstUndecided; i < configLength; i++) {
        config[i] = IN_X;
        long weightWhenInX = computeSplitWeight(config);
        config[i] = IN_Y;
        long weightWhenInY = computeSplitWeight(config);
        lowerBound += (min(weightWhenInX, weightWhenInY) - weightOfDecidedPart);
        config[i] = NOT_DECIDED;
    }

    return lowerBound;
}

/**
 * Auxiliary recursive function which search through the bottom layers of the graph using
 * DFS.
 * @param config vertexes configuration
 * @param indexOfFirstUndecided index of first vertex from configuration which is not assigned to X
 * or Y
 */
void searchAux(short *config, int indexOfFirstUndecided) {

    // configurations in this sub tree contains to much vertexes included in smaller set
    pair<int, int> sizeOfXAndY = computeSizeOfXAndY(config);
    if (sizeOfXAndY.first > smallerSetSize ||
        sizeOfXAndY.second > configLength - smallerSetSize) {
        return;
    }

    long weightOfDecidedPart = computeSplitWeight(config);

    // all configurations in this sub tree are worse than best solution
    if (weightOfDecidedPart > minimalSplitWeight) {
        return;
    }

    if (weightOfDecidedPart +
            lowerBoundOfUndecidedPart(config, indexOfFirstUndecided, weightOfDecidedPart) >
        minimalSplitWeight) {
        return;
    }

    // end recursion
    if (indexOfFirstUndecided == configLength) {
        // not valid solution
        if (computeSizeOfXAndY(config).first != smallerSetSize) {
            return;
        }

        long weight = computeSplitWeight(config);
        // if best, save it
        if (weight < minimalSplitWeight) {
            minimalSplitWeight = weight;
            copy(config, config + configLength, minimalSplitConfig);
        }
        return;
    }

    config[indexOfFirstUndecided] = IN_X;
    indexOfFirstUndecided++;
    searchAux(config, indexOfFirstUndecided);

    config[indexOfFirstUndecided - 1] = IN_Y;
    for (int i = indexOfFirstUndecided; i < configLength; i++) {
        config[i] = NOT_DECIDED;
    }
    searchAux(config, indexOfFirstUndecided);
}

// search in best split
void search() {
    configLength = graph.vertexesCount;
    short* initConfig = new short[configLength];
    minimalSplitConfig = new short[configLength];
    fill_n(initConfig, configLength, NOT_DECIDED);
    searchAux(initConfig, 0);
    delete[] initConfig;
}

/**
 * Test input.
 * @param testData test data, which contains input and output parameters.
 */
void testInput(TestData &testData) {
    graph = Graph();
    if (!graph.loadFromFile(testData.filePath)) {
        exit(1);
    }
    smallerSetSize = testData.sizeOfX;
    search();

    cout << testData.filePath << endl;
    cout << "Minimal weight: " << minimalSplitWeight << endl;
    printConfig(minimalSplitConfig);

    if (testData.weight != -1) {
        assert(minimalSplitWeight == testData.weight);
    }

    delete[] minimalSplitConfig;
}

// main function
int main(int argc, char** argv) {
    steady_clock::time_point start = steady_clock::now();  // timer start

    // arguments are: path to input graph, size of smaller set (X) number of threads, optionally
    // solution for testing purposes
    if (argc < 4) {
        cerr << "Usage: " << argv[0]
             << " <path_to_graph> <size_of_set_X> <solution>?" << endl;
        return 1;
    }

    char *pathToGraph = argv[1];
    int sizeOfSmallerSet = atoi(argv[2]);
    int solution = -1;
    if (argc == 4) {
        solution = atoi(argv[3]);
    }

    TestData testData = TestData(pathToGraph, sizeOfSmallerSet, solution);
    testInput(testData);

    steady_clock::time_point end = steady_clock::now();  // timer stop
    auto time = duration<double>(end - start);
    cout << "time: " << std::round(time.count() * 1000.0) / 1000.0 << "s" << endl;
    cout << "________________________________" << endl;

    return 0;
}
