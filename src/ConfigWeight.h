#pragma once

#include <mpi.h>

#include <climits>

#include "constants.h"

// struct for sending configuration and weight
class ConfigWeight {
   public:
    ConfigWeight(int configLength, long weight, short* config);

    explicit ConfigWeight(int configLength);

    virtual ~ConfigWeight();

    // send self to destination process id
    virtual void send(int destination, int tag = TAG_RESULT);

    // receive self from destination process id
    virtual MPI_Status receive(int destination = MPI_ANY_SOURCE, int tag = TAG_RESULT);

    // compute size of all props in bytes
    virtual long size();

    long getWeight();

    short* getConfig();

    void setWeightAndConfig(long weight, short* config);

   protected:
    long weight;       // minimal weight of split
    int configLength;  // length of config
    short* config;     // configuration of vertexes
};
