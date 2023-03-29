#include <mpi.h>

#include <climits>

#include "constants.cpp"

// struct for sending configuration and weight
class ConfigWeight {
   public:
    // ConfigWeight(long weight, int configLength, short* config, short* task)
    //     : weight(weight), configLength(configLength) {
    //     this->config = new short[configLength];
    //     std::copy(config, config + configLength, this->config);
    //     this->task = new short[configLength];
    //     std::copy(task, task + configLength, this->task);
    // }

    ConfigWeight(long weight, int configLength, short* config)
        : weight(weight), configLength(configLength) {
        this->config = new short[configLength];
        // std::copy(config, config + configLength, this->config);
        // this->task = new short[configLength];
    }

    explicit ConfigWeight(int configLength) : weight(LONG_MAX), configLength(configLength) {
        config = new short[configLength];
    }

    virtual ~ConfigWeight() {
        delete[] config;
        // delete[] task;
    }

    // send self to destination process id
    virtual void send(int destination) {
        // printf("virtual send called\n");
        int position = 0;
        int bufferSize = size() / sizeof(char);
        char* buffer = new char[bufferSize];
        MPI_Pack(&weight, 1, MPI_LONG, buffer, bufferSize, &position, MPI_COMM_WORLD);
        MPI_Pack(config, configLength, MPI_SHORT, buffer, bufferSize, &position, MPI_COMM_WORLD);
        // MPI_Pack(task, configLength, MPI_SHORT, buffer, bufferSize, &position, MPI_COMM_WORLD);
        MPI_Send(buffer, position, MPI_PACKED, destination, TAG_RESULT, MPI_COMM_WORLD);
        delete[] buffer;
    }

    // receive self from destination process id
    virtual MPI_Status receive(int destination = MPI_ANY_SOURCE) {
        // printf("virtual receive called\n");
        MPI_Status status;
        int position = 0;
        int bufferSize = size() / sizeof(char);
        char* buffer = new char[bufferSize];
        MPI_Recv(buffer, bufferSize, MPI_PACKED, destination, TAG_RESULT, MPI_COMM_WORLD, &status);
        MPI_Unpack(buffer, bufferSize, &position, &weight, 1, MPI_LONG, MPI_COMM_WORLD);
        MPI_Unpack(buffer, bufferSize, &position, config, configLength, MPI_SHORT, MPI_COMM_WORLD);
        // MPI_Unpack(buffer, bufferSize, &position, task, configLength, MPI_SHORT, MPI_COMM_WORLD);
        delete[] buffer;
        return status;
    }

    // compute size of all props in bytes
    long size() { return sizeof(weight) + sizeof(*config) * (long)this->configLength; }

    long getWeight() { return weight; }

    short* getConfig() { return config; }

    // short* getTask() { return task; }

   protected:
    long weight;       // minimal weight of split
    int configLength;  // length of config
    short* config;     // configuration of vertexes
    // short* task;       // task for slave
};