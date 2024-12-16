#ifndef AGENT_H
#define AGENT_H

#include <iostream>
#include <random>

#include "defs.h"
#include "neureset.h"

class Agent {
    private:
        Neureset* neureset;
        double* const* const brainWave;

        double** buildBrainWave();
        double* linspace(const int start, const int end, const int numPoints);

    public:
        explicit Agent(Neureset* neureset);
        ~Agent();

        void helmet();
};
#endif
