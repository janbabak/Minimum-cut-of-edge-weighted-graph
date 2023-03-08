#include <omp.h>

#include <cassert>
#include <climits>
#include <iostream>
#include <vector>

#include "Graph.cpp"
#include "TestData.cpp"

using namespace std;

const short IN_X = 1;
const short IN_Y = 0;
const short NOT_DECIDED = -1;

long minimalSplitWeight = LONG_MAX;
short* minimalSplitConfig = nullptr;

// debug print configuration
void printConfig(short* config, int& configSize, ostream& os = cout) {
    os << "[";
    for (int i = 0; i < configSize; i++) {
        os << config[i];
        if (i == configSize - 1) {
            os << "]" << endl;
        } else {
            os << ", ";
        }
    }
}

// compute number of vertexes in X set from configuration
int computeSizeOfX(short* config, int& configurationSize) {
    int count = 0;

    for (int i = 0; i < configurationSize; i++) {
        if (config[i] == IN_X) {
            count++;
        } else if (config[i] == NOT_DECIDED) {
            return count;  // all following vertexes are not decided
        }
    }

    return count;
}

// compute sum of weights of edges, that has one vertex in X and second in Y
long computeSplitWeight(short* config, Graph& graph) {
    long weight = 0;

    for (int i = 0; i < graph.edgesSize; i++) {
        if (config[graph.edges[i].vertexId1] == IN_X && config[graph.edges[i].vertexId2] == IN_Y) {
            weight += graph.edges[i].weight;
        }
    }

    return weight;
}

// compute lower bound of undecided part of vertexes
long lowerBoundOfUndecidedPart(short* config, Graph& graph, int indexOfFirstUndecided,
                               long weightOfDecidedPart) {
    long lowerBound = 0;

    for (int i = indexOfFirstUndecided; i < graph.vertexesCount; i++) {
        config[i] = IN_X;
        long weightWhenInX = computeSplitWeight(config, graph);
        config[i] = IN_Y;
        long weightWhenInY = computeSplitWeight(config, graph);
        lowerBound += (min(weightWhenInX, weightWhenInY) - weightOfDecidedPart);
        config[i] = NOT_DECIDED;
    }

    return lowerBound;
}

// auxiliary recursive function, tries all configurations
void searchAuxSequential(short* config, Graph& graph, int indexOfFirstUndecided,
                         int& targetSizeOfSetX) {
    // configurations in this sub tree contains to much vertexes included in smaller set
    if (computeSizeOfX(config, graph.vertexesCount) > targetSizeOfSetX) {
        return;
    }

    long weightOfDecidedPart = computeSplitWeight(config, graph);

    // all configurations in this sub tree are worse than best solution
    if (weightOfDecidedPart > minimalSplitWeight) {
        return;
    }

    if (weightOfDecidedPart +
            lowerBoundOfUndecidedPart(config, graph, indexOfFirstUndecided, weightOfDecidedPart) >
        minimalSplitWeight) {
        return;
    }

    // end recursion
    if (indexOfFirstUndecided == graph.vertexesCount) {
        // not valid solution
        if (computeSizeOfX(config, graph.vertexesCount) != targetSizeOfSetX) {
            return;
        }

        long weight = computeSplitWeight(config, graph);
        // if best, save it
        if (weight < minimalSplitWeight) {
            minimalSplitWeight = weight;
            for (int i = 0; i < graph.vertexesCount; i++) {
                minimalSplitConfig[i] = config[i];
            }
        }
        return;
    }

    config[indexOfFirstUndecided] = IN_X;
    indexOfFirstUndecided++;
    searchAuxSequential(config, graph, indexOfFirstUndecided, targetSizeOfSetX);

    config[indexOfFirstUndecided - 1] = IN_Y;
    for (int i = indexOfFirstUndecided; i < graph.vertexesCount; i++) {
        config[i] = NOT_DECIDED;
    }
    searchAuxSequential(config, graph, indexOfFirstUndecided, targetSizeOfSetX);
}

// auxiliary recursive function, tries all configurations
void searchAuxParallel(short* config, Graph& graph, int indexOfFirstUndecided,
                       int& targetSizeOfSetX) {
    // configurations in this sub tree contains to much vertexes included in smaller set
    if (computeSizeOfX(config, graph.vertexesCount) > targetSizeOfSetX) {
        return;
    }

    long weightOfDecidedPart = computeSplitWeight(config, graph);

    // all configurations in this sub tree are worse than best solution
    if (weightOfDecidedPart > minimalSplitWeight) {
        return;
    }

    if (weightOfDecidedPart +
            lowerBoundOfUndecidedPart(config, graph, indexOfFirstUndecided, weightOfDecidedPart) >
        minimalSplitWeight) {
        return;
    }

    // end recursion
    if (indexOfFirstUndecided == graph.vertexesCount) {
        // not valid solution
        if (computeSizeOfX(config, graph.vertexesCount) != targetSizeOfSetX) {
            return;
        }

        long weight = computeSplitWeight(config, graph);

#pragma omp critical
        {
            // if best, save it
            if (weight < minimalSplitWeight) {
                minimalSplitWeight = weight;
                for (int i = 0; i < graph.vertexesCount; i++) {
                    minimalSplitConfig[i] = config[i];
                }
            }
        }
        return;
    }

    short* secondConfig = new short[graph.vertexesCount];
    for (int i = 0; i < graph.vertexesCount; i++) {
        secondConfig[i] = config[i];
    }

    config[indexOfFirstUndecided] = IN_X;
    secondConfig[indexOfFirstUndecided] = IN_Y;

    indexOfFirstUndecided++;

    if (graph.vertexesCount - indexOfFirstUndecided > 3) {
#pragma omp task
        searchAuxParallel(config, graph, indexOfFirstUndecided, targetSizeOfSetX);

#pragma omp task
        searchAuxParallel(secondConfig, graph, indexOfFirstUndecided, targetSizeOfSetX);

#pragma omp taskwait
        delete[] secondConfig;
    } else {
        searchAuxSequential(config, graph, indexOfFirstUndecided, targetSizeOfSetX);
        searchAuxSequential(secondConfig, graph, indexOfFirstUndecided, targetSizeOfSetX);
        delete[] secondConfig;
    }
}

// search in best split
void search(Graph& graph, int smallerSetSize) {
    short* initConfig = new short[graph.vertexesCount];
    minimalSplitConfig = new short[graph.vertexesCount];
    for (int i = 0; i < graph.vertexesCount; i++) {
        initConfig[i] = NOT_DECIDED;
    }

#pragma omp parallel num_threads(10)
    {
#pragma omp single
        searchAuxParallel(initConfig, graph, 0, smallerSetSize);
    }
    delete[] initConfig;
}

// main - tests
int main() {
    vector<TestData> testData = {
        TestData("graf_mro/graf_10_5.txt", 5, 974),
        TestData("graf_mro/graf_10_6b.txt", 5, 1300),
        TestData("graf_mro/graf_20_7.txt", 7, 2110),
        TestData("graf_mro/graf_20_7.txt", 10, 2378),
        TestData("graf_mro/graf_20_12.txt", 10, 5060),
        TestData("graf_mro/graf_30_10.txt", 10, 4636),
        //  TestData("graf_mro/graf_30_10.txt", 15, 5333),
        //  TestData("graf_mro/graf_30_20.txt", 15, 13159),
        //   TestData("graf_mro/graf_40_8.txt", 15, 4256),
    };

    for (TestData& td : testData) {
        Graph graph = Graph();
        minimalSplitWeight = LONG_MAX;
        graph.loadFromFile(td.filePath);
        search(graph, td.sizeOfX);
        cout << td.filePath << endl;
        cout << "Minimal weight: " << minimalSplitWeight << endl;
        printConfig(minimalSplitConfig, graph.vertexesCount);
        cout << "________________________________" << endl;
        assert(minimalSplitWeight == td.weight);
        delete[] minimalSplitConfig;
    }

    return 0;
}
