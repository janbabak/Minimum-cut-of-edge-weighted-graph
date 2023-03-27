#include <mpi.h>

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
const short MASTER = 0;
const short TAG_DONE = 0;
const short TAG_WORK = 1;
const short TAG_TERMINATE = 2;
const short TAG_RESULT = 3;
const short TAG_RESULT_WEIGHT = 3;
const short TAG_RESULT_CONFIG = 4;

int numberOfProcesses;
int processId;
long minimalSplitWeight = LONG_MAX;
short* minimalSplitConfig = nullptr;
int maxPregeneratedLevelFromMaster = 3;  // number of filled cells of config in master task pool
int maxPregeneratedLevelFromSlave = 5;   // number of filled cells of config in slave task pool
int smallerSetSize;                      // size of smaler set X
vector<short*> taskPool = {};
int configLength;

struct Result {
    short* minimalSplitConfig;
    int minimalSplitWeight;

    Result(short* config, int weight) {
        minimalSplitConfig = config;
        minimalSplitWeight = weight;
    }

    explicit Result() {
        cout << "result init with size " << configLength << endl;
        minimalSplitConfig = new short[configLength];
        minimalSplitWeight = -1;
    }

    // ~Result() { delete[] minimalSplitConfig; }

    long size() { return sizeof(minimalSplitWeight) + sizeof(*minimalSplitConfig) * configLength; }
} typedef Result;

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
void produceTaskPoolAux(short* config, int indexOfFirstUndecided, int maxPregeneratedLength) {
    if (indexOfFirstUndecided >= configLength || indexOfFirstUndecided >= maxPregeneratedLength) {
        taskPool.push_back(config);
        return;
    }

    short* secondConfig = new short[configLength];
    copy(config, config + configLength, secondConfig);

    config[indexOfFirstUndecided] = IN_X;
    secondConfig[indexOfFirstUndecided] = IN_Y;

    indexOfFirstUndecided++;

    produceTaskPoolAux(config, indexOfFirstUndecided, maxPregeneratedLength);
    produceTaskPoolAux(secondConfig, indexOfFirstUndecided, maxPregeneratedLength);
}

// produce initial configurations
void produceMasterTaskPool() {
    short* config = new short[configLength];
    fill_n(config, configLength, NOT_DECIDED);
    produceTaskPoolAux(config, 0, maxPregeneratedLevelFromMaster);
}

void produceSlaveTaskPool(short* initConfig) {
    int indexOfFirstUndecided = 0;
    for (int i = 0; i < configLength; i++) {
        if (initConfig[i] == NOT_DECIDED) {
            indexOfFirstUndecided = (i - 1 >= 0) ? i - 1 : 0;
            break;
        }
    }
    taskPool = {};  // clean the task pool
    produceTaskPoolAux(initConfig, indexOfFirstUndecided, maxPregeneratedLevelFromSlave);
}

// consume configurations
void consumeTaskPool(Graph& graph) {
    int indexOfFirstUndecided = min(maxPregeneratedLevelFromSlave, graph.vertexesCount);

    // #pragma omp parallel for schedule(dynamic)
    for (auto& task : taskPool) {
        searchAux(task, graph, indexOfFirstUndecided, smallerSetSize);
    }

    // for (auto& task : taskPool) {
    // delete[] task;
    // }
}

/** distribute taskpool between processes */
void distributeMasterTaskPool() {
    for (int destination = 0; destination < numberOfProcesses; destination++) {
        MPI_Send(taskPool.back(), configLength, MPI_SHORT, destination, TAG_WORK, MPI_COMM_WORLD);
        taskPool.pop_back();
    }
}

void masterMainLoop() {
    int workingSlaves = numberOfProcesses - 1;  // minus 1, because of master process
    MPI_Status status;

    while (workingSlaves > 0) {
        MPI_Recv(nullptr, 0, MPI_SHORT, MPI_ANY_SOURCE, TAG_DONE, MPI_COMM_WORLD, &status);

        // if there left some task, assign it to finished process
        if (taskPool.size() > 0) {
            MPI_Send(taskPool.back(), configLength, MPI_SHORT, status.MPI_SOURCE, TAG_WORK,
                     MPI_COMM_WORLD);
            taskPool.pop_back();
        }
        // no task left -> terminate slave
        else {
            MPI_Send(nullptr, 0, MPI_SHORT, status.MPI_SOURCE, TAG_TERMINATE, MPI_COMM_WORLD);
            workingSlaves--;
        }
    }

    int receivedResults = 0;
    Result* result = new Result();
    while (receivedResults < numberOfProcesses - 1) {
        MPI_Recv((void*)result, result->size(), MPI_BYTE, MPI_ANY_SOURCE, TAG_RESULT,
                 MPI_COMM_WORLD, &status);
        printf("master got %d\n", result->minimalSplitWeight);
        if (result->minimalSplitWeight < minimalSplitWeight) {
            minimalSplitWeight = result->minimalSplitWeight;
            // copy(result->minimalSplitConfig, result->minimalSplitConfig + configLength,
            //      minimalSplitConfig);
            for (int i = 0; i < configLength; i++) {
                // minimalSplitConfig[i] = result->minimalSplitConfig[i];
                cout << result->minimalSplitConfig[i] << endl;
            }
        }
        receivedResults++;
    }
    // delete result;
}

void master(Graph& graph) {
    produceMasterTaskPool();
    distributeMasterTaskPool();
    masterMainLoop();
}

void slave(Graph& graph) {
    // consumeTaskPool(graph, smallerSetSize);
    MPI_Status status;
    short* config = new short[configLength];

    while (true) {
        MPI_Recv(config, configLength, MPI_SHORT, MASTER, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        // terminate
        if (status.MPI_TAG == TAG_TERMINATE) {
            Result* result = new Result(minimalSplitConfig, minimalSplitWeight);
            MPI_Send((void*)result, result->size(), MPI_BYTE, status.MPI_SOURCE, TAG_RESULT,
                     MPI_COMM_WORLD);
            return;
        }
        // work
        else if (status.MPI_TAG == TAG_WORK) {
            // printf("process %d work\n", processId);
            produceSlaveTaskPool(config);
            consumeTaskPool(graph);
            MPI_Send(nullptr, 0, MPI_SHORT, status.MPI_SOURCE, TAG_DONE, MPI_COMM_WORLD);
        } else {
            printf("ERROR, BAD MESSAGE");
        }
    }
}

// search in best split
void search(Graph& graph) {
    // init minimal split in all processes
    configLength = graph.vertexesCount;
    minimalSplitWeight = LONG_MAX;
    minimalSplitConfig = new short[configLength];

    if (processId == 0) {
        master(graph);
    } else {
        slave(graph);
    }
}

// reset for new test case
void reset() { taskPool = {}; }

// main - tests
int main(int argc, char** argv) {
    // initialize the open MPI environment
    MPI_Init(&argc, &argv);

    // get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &numberOfProcesses);

    // get the process id
    MPI_Comm_rank(MPI_COMM_WORLD, &processId);

    // printf("world size: %d, world rank %d\n\n", numberOfProcesses, processId);

    vector<TestData> testData = {
        TestData("graf_mro/graf_10_5.txt", 5, 974),
        // TestData("graf_mro/graf_10_6b.txt", 5, 1300),
        // TestData("graf_mro/graf_20_7.txt", 7, 2110),
        // TestData("graf_mro/graf_20_7.txt", 10, 2378),
        // TestData("graf_mro/graf_20_12.txt", 10, 5060),
        // TestData("graf_mro/graf_30_10.txt", 10, 4636),
        // TestData("graf_mro/graf_30_10.txt", 15, 5333),
        // TestData("graf_mro/graf_30_20.txt", 15, 13159),
        // TestData("graf_mro/graf_40_8.txt", 15, 4256),
    };

    for (TestData& td : testData) {
        reset();
        Graph graph = Graph();
        graph.loadFromFile(td.filePath);
        smallerSetSize = td.sizeOfX;
        search(graph);

        // cout << td.filePath << endl;
        // cout << "Minimal weight: " << minimalSplitWeight << endl;
        // printConfig(minimalSplitConfig, graph.vertexesCount);
        // cout << "________________________________" << endl;

        // assert(minimalSplitWeight == td.weight);

        // delete[] minimalSplitConfig;
    }

    // finalize the mpi environment
    MPI_Finalize();

    return 0;
}
