#include <mpi.h>

#include <cassert>
#include <climits>
#include <iostream>
#include <vector>

#include "ConfigWeight.h"
#include "ConfigWeightTask.h"
#include "Graph.cpp"
#include "TestData.cpp"

using namespace std;

int numberOfProcesses;
int processId;
long minimalSplitWeight = LONG_MAX;      // best found weight
short* minimalSplitConfig = nullptr;     // best found configuration
int maxPregeneratedLevelFromMaster = 6;  // number of filled cells of config in master task pool
int maxPregeneratedLevelFromSlave = 9;   // number of filled cells of config in slave task pool
int smallerSetSize;                      // size of smaler set X
int configLength;
vector<short*> taskPool = {};
Graph graph;

// debug print configuration
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

// return true if current process is master, false otherwise
[[nodiscard]] inline bool isMaster() { return processId == MASTER; }

// compute number of vertexes in (X set, Y set) from configuration
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

// compute sum of weights of edges, that has one vertex in X and second in Y
[[nodiscard]] long computeSplitWeight(short* config) {
    long weight = 0;

    for (int i = 0; i < graph.edgesSize; i++) {
        if (config[graph.edges[i].vertexId1] == IN_X && config[graph.edges[i].vertexId2] == IN_Y) {
            weight += graph.edges[i].weight;
        }
    }

    return weight;
}

// compute lower bound of undecided part of vertexes
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

// auxiliary recursive function, tries all configurations
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
    // find index of first undecide vertex in config
    int indexOfFirstUndecided = 0;
    for (int i = 0; i < configLength; i++) {
        if (initConfig[i] == NOT_DECIDED) {
            indexOfFirstUndecided = (i - 1 >= 0) ? i - 1 : 0;
            break;
        }
    }

    produceTaskPoolAux(initConfig, indexOfFirstUndecided, maxPregeneratedLevelFromSlave);
}

// consume configurations
void consumeTaskPool(Graph& graph) {
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

// send task to slave
void sendTaskToSlave(int destination) {
    ConfigWeightTask message =
        ConfigWeightTask(configLength, minimalSplitWeight, minimalSplitConfig, taskPool.back());
    taskPool.pop_back();
    message.send(destination, TAG_WORK);
}

/** distribute taskpool between processes */
void distributeMasterTaskPool() {
    for (int destination = 0; destination < numberOfProcesses; destination++) {
        sendTaskToSlave(destination);
    }
}

// save configuration if is the bestf found yet
void saveConfigIfBest(ConfigWeight& resultMessage) {
    // save if best
    if (resultMessage.getWeight() < minimalSplitWeight) {
        minimalSplitWeight = resultMessage.getWeight();
        for (int i = 0; i < configLength; i++) {
            minimalSplitConfig[i] = (short)resultMessage.getConfig()[i];
        }
    }
}

// collect results from slaves
void collectResults() {
    int receivedResults = 0;
    ConfigWeight resultMessage = ConfigWeight(configLength);
    while (receivedResults < numberOfProcesses - 1) {
        resultMessage.receive();
        saveConfigIfBest(resultMessage);
        receivedResults++;
    }
}

// distribute tasks and collect results
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

// master process function
void master() {
    produceMasterTaskPool();
    distributeMasterTaskPool();
    masterMainLoop();
}

// slave process function
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
            produceSlaveTaskPool(taskMessage.getTask());
            saveConfigIfBest(taskMessage);
            consumeTaskPool(graph);
            resultMessage.setWeightAndConfig(minimalSplitWeight, minimalSplitConfig);
            resultMessage.send(MASTER, TAG_DONE);
        } else {
            printf("ERROR, BAD MESSAGE");
        }
    }
}

// init some variables before search
void initSearch() {
    configLength = graph.vertexesCount;
    minimalSplitWeight = LONG_MAX;
    minimalSplitConfig = new short[configLength];
    taskPool = {};
}

// search in best split
void search() {
    initSearch();

    if (isMaster()) {
        master();
    } else {
        slave();
    }
}

// test test input
void testInput(TestData& testData) {
    graph = Graph();
    graph.loadFromFile(testData.filePath);
    smallerSetSize = testData.sizeOfX;
    search();

    if (isMaster()) {
        cout << testData.filePath << endl;
        cout << "Minimal weight: " << minimalSplitWeight << endl;
        printConfig(minimalSplitConfig);
        cout << "________________________________" << endl;

        assert(minimalSplitWeight == testData.weight);
    }

    delete[] minimalSplitConfig;
}

// test all inputs
void test() {
    vector<TestData> testData = {
        TestData("graf_mro/graf_10_5.txt", 5, 974),
        TestData("graf_mro/graf_10_6b.txt", 5, 1300),
        TestData("graf_mro/graf_20_7.txt", 7, 2110),
        TestData("graf_mro/graf_20_7.txt", 10, 2378),
        TestData("graf_mro/graf_20_12.txt", 10, 5060),
        TestData("graf_mro/graf_30_10.txt", 10, 4636),
        TestData("graf_mro/graf_30_10.txt", 15, 5333),
        TestData("graf_mro/graf_30_20.txt", 15, 13159),
        TestData("graf_mro/graf_40_8.txt", 15, 4256),
    };

    // test all unputs
    for (TestData& td : testData) {
        testInput(td);
    }
}

// main - tests
int main(int argc, char** argv) {
    // Initialize the open MPI environment with thread enabled
    int provided, required = MPI_THREAD_MULTIPLE;
    MPI_Init_thread(&argc, &argv, required, &provided);
    if (provided < required) {
        throw runtime_error("MPI library does not provide required threading support");
    }

    // get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &numberOfProcesses);

    // get the process id
    MPI_Comm_rank(MPI_COMM_WORLD, &processId);

    // test inputs
    test();

    // finalize the mpi environment
    MPI_Finalize();

    return 0;
}
