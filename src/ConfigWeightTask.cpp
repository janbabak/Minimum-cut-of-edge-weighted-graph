#include "ConfigWeightTask.h"

ConfigWeightTask::ConfigWeightTask(int configLength, long weight, short* config, short* task)
    : ConfigWeight(configLength, weight, config) {
    this->task = new short[configLength];
    std::copy(task, task + configLength, this->task);
}

ConfigWeightTask::ConfigWeightTask(int configLength, long weight, short* config)
    : ConfigWeight(configLength, weight, config) {
    this->task = new short[configLength];
}

ConfigWeightTask::ConfigWeightTask(int configLength) : ConfigWeight(configLength) {
    task = new short[configLength];
}

ConfigWeightTask::~ConfigWeightTask() { delete[] task; }

// send self to destination process id
void ConfigWeightTask::send(int destination, int tag) {
    int position = 0;
    int bufferSize = size() / sizeof(char);
    char* buffer = new char[bufferSize];
    MPI_Pack(&weight, 1, MPI_LONG, buffer, bufferSize, &position, MPI_COMM_WORLD);
    MPI_Pack(config, configLength, MPI_SHORT, buffer, bufferSize, &position, MPI_COMM_WORLD);
    MPI_Pack(task, configLength, MPI_SHORT, buffer, bufferSize, &position, MPI_COMM_WORLD);
    MPI_Send(buffer, position, MPI_PACKED, destination, tag, MPI_COMM_WORLD);
    delete[] buffer;
}

// receive self from destination process id
MPI_Status ConfigWeightTask::receive(int destination, int tag) {
    MPI_Status status;
    // printf("status.")
    int position = 0;
    int bufferSize = size() / sizeof(char);
    char* buffer = new char[bufferSize];
    MPI_Recv(buffer, bufferSize, MPI_PACKED, destination, tag, MPI_COMM_WORLD, &status);
    if (status.MPI_TAG == TAG_TERMINATE) {
        delete[] buffer;
        return status;
    }
    MPI_Unpack(buffer, bufferSize, &position, &weight, 1, MPI_LONG, MPI_COMM_WORLD);
    MPI_Unpack(buffer, bufferSize, &position, config, configLength, MPI_SHORT, MPI_COMM_WORLD);
    MPI_Unpack(buffer, bufferSize, &position, task, configLength, MPI_SHORT, MPI_COMM_WORLD);
    delete[] buffer;
    return status;
}

// compute size of all props in bytes
long ConfigWeightTask::size() {
    return sizeof(weight) + sizeof(*config) * (long)this->configLength +
           sizeof(*task) * (long)this->configLength;
}

short* ConfigWeightTask::getTask() { return task; }