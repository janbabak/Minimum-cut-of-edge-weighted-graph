#pragma once

#include "ConfigWeight.h"

// struct for sending configuration and weight and task configuration
class ConfigWeightTask : public ConfigWeight {
   public:
    ConfigWeightTask(int configLength, long weight, short* config, short* task);

    ConfigWeightTask(int configLength, long weight, short* config);

    explicit ConfigWeightTask(int configLength);

    ~ConfigWeightTask() override;

    // send self to destination process id
    void send(int destination, int tag = TAG_RESULT) override;

    // receive self from destination process id
    MPI_Status receive(int destination = MPI_ANY_SOURCE, int tag = TAG_RESULT) override;

    // compute size of all props in bytes
    long size() override;
    short* getTask();

   protected:
    short* task;  // task for slave
};