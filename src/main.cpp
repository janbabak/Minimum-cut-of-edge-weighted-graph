#include <mpi.h>
#include <omp.h>

#include <cassert>
#include <chrono>
#include <climits>
#include <cmath>
#include <iostream>
#include <vector>

#include "ConfigWeight.h"
#include "ConfigWeightTask.h"
#include "Graph.h"
#include "TestData.cpp"

using namespace std;
using namespace std::chrono;

int numberOfProcesses;
int processId;
long minimalSplitWeight = LONG_MAX;      // best found weight
short* minimalSplitConfig = nullptr;     // best found configuration
int maxPregeneratedLevelFromMaster = 6;  // number of filled cells of config in master task pool
int maxPregeneratedLevelFromSlave = 9;   // number of filled cells of config in slave task pool
int smallerSetSize;                      // size of smaller set X
int configLength;
vector<short*> taskPool = {};
Graph graph;

/**
 * Print configuration for debug purposes.
 * @param config vertexes configuration to print
 * @param os output stream
 */
void printConfig(short* config, ostream& os = cout) {
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
 * Check if current process is master.
 * @return true if current process is master, false otherwise
 */
[[nodiscard]] inline bool isMaster() { return processId == MASTER; }

/**
 * Compute number of vertexes in both sets (X and Y).
 * @param config vertexes configuration
 * @return pair, where the first item is size of X and the second one the is size of Y
 */
[[nodiscard]] pair<int, int> computeSizeOfXAndY(short* config) {
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
[[nodiscard]] long computeSplitWeight(short* config) {
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
 * @param indexOfFirstUndecided index of first vertex from configuration which is not assigned to X or Y
 * @param weightOfDecidedPart weight of the split computed only from decided vertexes
 * @return lower bound of weight
 */
[[nodiscard]] long lowerBoundOfUndecidedPart(short* config, int indexOfFirstUndecided,
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
 * Auxiliary recursive function which search through the entire graph using DFS.
 * @param config vertexes configuration
 * @param indexOfFirstUndecided index of first vertex from configuration which is not assigned to X or Y
 * @param targetSizeOfSetX size of set X (user parameter)
 */
void searchAux(short* config, int indexOfFirstUndecided, int& targetSizeOfSetX) {
    // configurations in this sub tree contains to much vertexes included in smaller set
    pair<int, int> sizeOfXAndY = computeSizeOfXAndY(config);
    if (sizeOfXAndY.first > targetSizeOfSetX ||
        sizeOfXAndY.second > configLength - targetSizeOfSetX) {
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
        if (computeSizeOfXAndY(config).first != targetSizeOfSetX) {
            return;
        }

        long weight = computeSplitWeight(config);
        // if best, save it
        if (weight < minimalSplitWeight) {
#pragma omp critical
            {
                if (weight < minimalSplitWeight) {
                    minimalSplitWeight = weight;
                    copy(config, config + configLength, minimalSplitConfig);
                }
            }
        }
        return;
    }

    config[indexOfFirstUndecided] = IN_X;
    indexOfFirstUndecided++;
    searchAux(config, indexOfFirstUndecided, targetSizeOfSetX);

    config[indexOfFirstUndecided - 1] = IN_Y;
    for (int i = indexOfFirstUndecided; i < configLength; i++) {
        config[i] = NOT_DECIDED;
    }
    searchAux(config, indexOfFirstUndecided, targetSizeOfSetX);
}

/**
 * Recursive function for producing pregenerated configurations into task pool.
 * @param config vertexes configuration from previous interation (recursive function call)
 * @param indexOfFirstUndecided index of first vertex from configuration which is not assigned to X or Y
 * @param maxPregeneratedLength maximum count of pregenerated vertexes
 */
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

/**
 * Produce master task pool.
 */
void produceMasterTaskPool() {
    short* config = new short[configLength];
    fill_n(config, configLength, NOT_DECIDED);
    produceTaskPoolAux(config, 0, maxPregeneratedLevelFromMaster);
}

/**
 * Produce slave task pool.
 * @param initConfig initial vertex configuration (obtained from master)
 */
void produceSlaveTaskPool(short* initConfig) {
    // find index of first undecided vertex in config
    int indexOfFirstUndecided = 0;
    for (int i = 0; i < configLength; i++) {
        if (initConfig[i] == NOT_DECIDED) {
            indexOfFirstUndecided = (i - 1 >= 0) ? i - 1 : 0;
            break;
        }
    }

    produceTaskPoolAux(initConfig, indexOfFirstUndecided, maxPregeneratedLevelFromSlave);
}

/**
 * Consume task pool (by slave)
 */
void consumeTaskPool() {
    int indexOfFirstUndecided = min(maxPregeneratedLevelFromSlave, configLength);

#pragma omp parallel for schedule(dynamic)
    for (auto& task : taskPool) {
        searchAux(task, indexOfFirstUndecided, smallerSetSize);
    }

    while (taskPool.size()) {
        // delete[] taskPool.back();  // TODO FIX
        taskPool.pop_back();
    }
}

/**
 * Send task to slave process.
 * @param destination id of slave process
 */
void sendTaskToSlave(int destination) {
    ConfigWeightTask message =
        ConfigWeightTask(configLength, minimalSplitWeight, minimalSplitConfig, taskPool.back());
    taskPool.pop_back();
    message.send(destination, TAG_WORK);
}

/**
 * Distribute master taskpool to slave processes.
 */
void distributeMasterTaskPool() {
    for (int destination = 0; destination < numberOfProcesses; destination++) {
        sendTaskToSlave(destination);
    }
}

/**
 * Save configuration if it is the new best configuration.
 * @param resultMessage message containing the result of computation of slave process.
 */
void saveConfigIfBest(ConfigWeight& resultMessage) {
    // save if best
    if (resultMessage.getWeight() < minimalSplitWeight) {
        minimalSplitWeight = resultMessage.getWeight();
        for (int i = 0; i < configLength; i++) {
            minimalSplitConfig[i] = (short)resultMessage.getConfig()[i];
        }
    }
}

/**
 * Collect results from slave processes.
 */
void collectResults() {
    int receivedResults = 0;
    ConfigWeight resultMessage = ConfigWeight(configLength);
    while (receivedResults < numberOfProcesses - 1) {
        resultMessage.receive();
        saveConfigIfBest(resultMessage);
        receivedResults++;
    }
}

/**
 * Main loop of master process. Distribute tasks to slaves and collect results from them.
 */
void masterMainLoop() {
    int workingSlaves = numberOfProcesses - 1;  // minus 1, because of master process
    MPI_Status status;
    ConfigWeight message = ConfigWeight(configLength);

    while (workingSlaves > 0) {
        status = message.receive(MPI_ANY_SOURCE, TAG_DONE);
        saveConfigIfBest(message);

        // if there left some task, assign it to finished process
        if (taskPool.size() > 0) {
            sendTaskToSlave(status.MPI_SOURCE);
        } else {
            // no task left -> terminate slave
            MPI_Send(nullptr, 0, MPI_SHORT, status.MPI_SOURCE, TAG_TERMINATE, MPI_COMM_WORLD);
            workingSlaves--;
        }
    }

    collectResults();
}

/**
 * Master process function.
 */
void master() {
    produceMasterTaskPool();
    distributeMasterTaskPool();
    masterMainLoop();
}

/**
 * Slave process function.
 */
void slave() {
    MPI_Status status;
    ConfigWeightTask taskMessage = ConfigWeightTask(configLength);
    ConfigWeight resultMessage = ConfigWeight(configLength);

    while (true) {
        status = taskMessage.receive(MASTER, MPI_ANY_TAG);

        // send result to master and terminate
        if (status.MPI_TAG == TAG_TERMINATE) {
            resultMessage.setWeightAndConfig(minimalSplitWeight, minimalSplitConfig);
            resultMessage.send(MASTER);
            return;
        }
        // work - compute
        else if (status.MPI_TAG == TAG_WORK) {
            saveConfigIfBest(taskMessage);
            produceSlaveTaskPool(taskMessage.getTask());
            consumeTaskPool();
            resultMessage.setWeightAndConfig(minimalSplitWeight, minimalSplitConfig);
            resultMessage.send(MASTER, TAG_DONE);
        } else {
            printf("ERROR, BAD MESSAGE");
        }
    }
}

/**
 * Initialize some variables before searching for the best configuration.
 */
void initSearch() {
    configLength = graph.vertexesCount;
    minimalSplitWeight = LONG_MAX;
    minimalSplitConfig = new short[configLength];
    taskPool = {};
}

/**
 * Search for in best configuration (split of vertexes to two sets X and Y).
 */
void search() {
    initSearch();

    if (isMaster()) {
        master();
    } else {
        slave();
    }
}

/**
 * Test input.
 * @param testData test data, which contains input and output parameters.
 */
void testInput(TestData& testData) {
    graph = Graph();
    graph.loadFromFile(testData.filePath);
    smallerSetSize = testData.sizeOfX;
    search();

    if (isMaster()) {
        cout << testData.filePath << endl;
        cout << "Minimal weight: " << minimalSplitWeight << endl;
        printConfig(minimalSplitConfig);

        assert(minimalSplitWeight == testData.weight);
    }

    delete[] minimalSplitConfig;
}

// main function
int main(int argc, char** argv) {
    steady_clock::time_point start = steady_clock::now();  // timer start

    // Initialize the open MPI environment with thread enabled
    int provided, required = MPI_THREAD_MULTIPLE;
    MPI_Init_thread(&argc, &argv, required, &provided);
    if (provided < required) {
        throw runtime_error("MPI library does not provide required threading support.");
    }

    // get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &numberOfProcesses);

    // get the process id
    MPI_Comm_rank(MPI_COMM_WORLD, &processId);

    // arguments are: path to input graph, size of smaller set (X) number of threads, optionally
    // solution for testing purposes
    if (argc < 4) {
        cerr << "Usage: " << argv[0]
             << " <path_to_graph> <size_of_set_X> <number_of_threads> <solution>?" << endl;
        return 1;
    }

    char* pathToGraph = argv[1];
    int sizeOfSmallerSet = atoi(argv[2]);
    int numberOfThreads = atoi(argv[3]);
    int solution = -1;
    if (argc == 5) {
        solution = atoi(argv[4]);
    }
    TestData testData = TestData(pathToGraph, sizeOfSmallerSet, solution);

    omp_set_dynamic(0);
    omp_set_num_threads(numberOfThreads);

    testInput(testData);

    // only master proces will display results
    if (isMaster()) {
        steady_clock::time_point end = steady_clock::now();  // timer stop
        auto time = duration<double>(end - start);
        cout << "time: " << std::round(time.count() * 1000.0) / 1000.0 << "s" << endl;
        cout << "________________________________" << endl;
    }

    // finalize the mpi environment
    MPI_Finalize();

    return 0;
}
