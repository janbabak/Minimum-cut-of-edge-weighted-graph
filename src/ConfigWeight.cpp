#include "ConfigWeight.h"

ConfigWeight::ConfigWeight(int configLength, long weight, short* config)
    : weight(weight), configLength(configLength) {
    this->config = new short[configLength];
    std::copy(config, config + configLength, this->config);
}

ConfigWeight::ConfigWeight(int configLength) : weight(LONG_MAX), configLength(configLength) {
    config = new short[configLength];
}

ConfigWeight::~ConfigWeight() { delete[] config; }

// send self to destination process id
void ConfigWeight::send(int destination, int tag) {
    int position = 0;
    int bufferSize = size() / sizeof(char);
    char* buffer = new char[bufferSize];
    MPI_Pack(&weight, 1, MPI_LONG, buffer, bufferSize, &position, MPI_COMM_WORLD);
    MPI_Pack(config, configLength, MPI_SHORT, buffer, bufferSize, &position, MPI_COMM_WORLD);
    MPI_Send(buffer, position, MPI_PACKED, destination, tag, MPI_COMM_WORLD);
    delete[] buffer;
}

// receive self from destination process id
MPI_Status ConfigWeight::receive(int destination, int tag) {
    MPI_Status status;
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
    delete[] buffer;
    return status;
}

// compute size of all props in bytes
long ConfigWeight::size() { return sizeof(weight) + sizeof(*config) * (long)this->configLength; }

long ConfigWeight::getWeight() { return weight; }

short* ConfigWeight::getConfig() { return config; }

void ConfigWeight::setWeightAndConfig(long weight, short* config) {
    this->weight = weight;
    std::copy(config, config + this->configLength, this->config);
}
