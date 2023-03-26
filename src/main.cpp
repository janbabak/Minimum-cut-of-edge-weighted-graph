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
int maxPregeneratedLevel = 7;  // number of filled cells of config
vector<short*> taskPool = {};

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

// compute number of vertexes in (X set, Y set) from configuration
pair<int, int> computeSizeOfXAndY(short* config, int& configurationSize) {
    int countX = 0;
    int countY = 0;

    for (int i = 0; i < configurationSize; i++) {
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
void searchAux(short* config, Graph& graph, int indexOfFirstUndecided, int& targetSizeOfSetX) {
    // configurations in this sub tree contains to much vertexes included in smaller set
    pair<int, int> sizeOfXAndY = computeSizeOfXAndY(config, graph.vertexesCount);
    if (sizeOfXAndY.first > targetSizeOfSetX ||
        sizeOfXAndY.second > graph.vertexesCount - targetSizeOfSetX) {
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
        if (computeSizeOfXAndY(config, graph.vertexesCount).first != targetSizeOfSetX) {
            return;
        }

        long weight = computeSplitWeight(config, graph);
        // if best, save it
        if (weight < minimalSplitWeight) {
#pragma omp critical
            {
                if (weight < minimalSplitWeight) {
                    minimalSplitWeight = weight;
                    for (int i = 0; i < graph.vertexesCount; i++) {
                        minimalSplitConfig[i] = config[i];
                    }
                }
            }
        }
        return;
    }

    config[indexOfFirstUndecided] = IN_X;
    indexOfFirstUndecided++;
    searchAux(config, graph, indexOfFirstUndecided, targetSizeOfSetX);

    config[indexOfFirstUndecided - 1] = IN_Y;
    for (int i = indexOfFirstUndecided; i < graph.vertexesCount; i++) {
        config[i] = NOT_DECIDED;
    }
    searchAux(config, graph, indexOfFirstUndecided, targetSizeOfSetX);
}

// recursive function for producing pregenerated configurations into task pool
void produceTaskPoolAux(short* config, int size, int indexOfFirstUndecided) {
    if (indexOfFirstUndecided >= size || indexOfFirstUndecided >= maxPregeneratedLevel) {
        taskPool.push_back(config);
        return;
    }

    short* secondConfig = new short[size];
    copy(config, config + size, secondConfig);

    config[indexOfFirstUndecided] = IN_X;
    secondConfig[indexOfFirstUndecided] = IN_Y;

    indexOfFirstUndecided++;

    produceTaskPoolAux(config, size, indexOfFirstUndecided);
    produceTaskPoolAux(secondConfig, size, indexOfFirstUndecided);
}

// produce initial configurations
void produceTaskPool(Graph& graph) {
    short* config = new short[graph.vertexesCount];
    fill_n(config, graph.vertexesCount, NOT_DECIDED);
    produceTaskPoolAux(config, graph.vertexesCount, 0);
}

// consume configurations
void consumeTaskPool(Graph& graph, int smallerSetSize) {
    int indexOfFirstUndecided = min(maxPregeneratedLevel, graph.vertexesCount);

#pragma omp parallel for schedule(dynamic)
    for (auto& task : taskPool) {
        searchAux(task, graph, indexOfFirstUndecided, smallerSetSize);
    }

    for (auto& task : taskPool) {
        delete[] task;
    }
}

// search in best split
void search(Graph& graph, int smallerSetSize) {
    minimalSplitConfig = new short[graph.vertexesCount];

    produceTaskPool(graph);

    consumeTaskPool(graph, smallerSetSize);
}

// reset for new test case
void reset() {
    minimalSplitWeight = LONG_MAX;
    taskPool = {};
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
        TestData("graf_mro/graf_30_10.txt", 15, 5333),
        // TestData("graf_mro/graf_30_20.txt", 15, 13159),
        // TestData("graf_mro/graf_40_8.txt", 15, 4256),
    };

    for (TestData& td : testData) {
        reset();
        Graph graph = Graph();
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
