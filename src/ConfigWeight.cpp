#include <mpi.h>

#include <climits>

#include "constants.cpp"

// struct for sending configuration and weight
class ConfigWeight {
   public:
    ConfigWeight(int configLength, long weight, short* config)
        : weight(weight), configLength(configLength) {
        this->config = new short[configLength];
        std::copy(config, config + configLength, this->config);
    }

    explicit ConfigWeight(int configLength) : weight(LONG_MAX), configLength(configLength) {
        config = new short[configLength];
    }

    virtual ~ConfigWeight() {
        delete[] config;
    }

    // send self to destination process id
    virtual void send(int destination, int tag = TAG_RESULT) {
        int position = 0;
        int bufferSize = size() / sizeof(char);
        char* buffer = new char[bufferSize];
        MPI_Pack(&weight, 1, MPI_LONG, buffer, bufferSize, &position, MPI_COMM_WORLD);
        MPI_Pack(config, configLength, MPI_SHORT, buffer, bufferSize, &position, MPI_COMM_WORLD);
        MPI_Send(buffer, position, MPI_PACKED, destination, tag, MPI_COMM_WORLD);
        delete[] buffer;
    }

    // receive self from destination process id
    virtual MPI_Status receive(int destination = MPI_ANY_SOURCE, int tag = TAG_RESULT) {
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
        delete[] buffer;
        return status;
    }

    // compute size of all props in bytes
    virtual long size() { return sizeof(weight) + sizeof(*config) * (long)this->configLength; }

    long getWeight() { return weight; }

    short* getConfig() { return config; }

    void setWeightAndConfig(long weight, short* config) {
        this->weight = weight;
        std::copy(config, config + this->configLength, this->config);
    }

   protected:
    long weight;       // minimal weight of split
    int configLength;  // length of config
    short* config;     // configuration of vertexes
};

// struct for sending configuration and weight and task configuration
class ConfigWeightTask : public ConfigWeight {
   public:
    ConfigWeightTask(int configLength, long weight, short* config, short* task)
        : ConfigWeight(configLength, weight, config) {
        this->task = new short[configLength];
        std::copy(task, task + configLength, this->task);
    }

    ConfigWeightTask(int configLength, long weight, short* config)
        : ConfigWeight(configLength, weight, config) {
        this->task = new short[configLength];
    }

    explicit ConfigWeightTask(int configLength) : ConfigWeight(configLength) {
        task = new short[configLength];
    }

    ~ConfigWeightTask() override {
        // delete[] config; //TODO
        delete[] task;
    }

    // send self to destination process id
    void send(int destination, int tag = TAG_RESULT) override {
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
    MPI_Status receive(int destination = MPI_ANY_SOURCE, int tag = TAG_RESULT) override {
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
    long size() override {
        return sizeof(weight) + sizeof(*config) * (long)this->configLength +
               sizeof(*task) * (long)this->configLength;
    }

    short* getTask() { return task; }

   protected:
    short* task;  // task for slave
};