#include "agent.h"


Agent::Agent(Neureset* neureset) : neureset(neureset), brainWave(buildBrainWave()) {}


Agent::~Agent() {
    for (int i = 0; i < NUM_BRAIN_SITES; ++i)
        delete[] brainWave[i];

    delete[] brainWave;
}


/*
    Builds brain waveforms 4 amplitudes, 4 frequencies: 21 x sampling rate matrix

    units of 0, 0.5, 1.0, 1.5, ... so DFT can land on exact values and mitigate sampling error.
*/
double** Agent::buildBrainWave() {
    // delta, theta, alpha, beta
    const double centers[4] = {2.5, 6, 10, 21}; // in hz
    const double offsets[4] = {1.5, 2, 2, 9};   // in hz
    const double amps[4] = {50, 40, 35, 25};    // in uv

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> volts(-5., 5.);

    const int samplingRate = MAX_FREQ * MAX_SAMPLES;
    double* time = linspace(0, 1, samplingRate);

    double** newBrainWave = new double* [NUM_BRAIN_SITES];
    for (int i = 0; i < NUM_BRAIN_SITES; ++i)
        newBrainWave[i] = new double[samplingRate]();

    for (int i = 0; i < NUM_BRAIN_SITES; ++i)
        for (int j = 0; j < NUM_BRAIN_FREQ; ++j) {

            std::uniform_real_distribution<> offset(-offsets[j], offsets[j]);
            double freq = std::round((centers[j] + offset(gen)) * 2.) / 2.;

            double amp = amps[j] + volts(gen);

            for (int k = 0; k < samplingRate; ++k)
                newBrainWave[i][k] += amp * std::cos(2. * PI * freq * time[k]);
        }

    delete[] time;
    return newBrainWave;
}


/*
    Generates evenly spaced horizontal axis.

    Used to build time slices for generating waveforms.

    discarded after buildBrainWave().

    returns:
        double** of 21 sites x sampling rate for frequency
*/
double* Agent::linspace(const int start, const int end, const int numPoints) {
    double* arr = new double[numPoints];

    const double step = static_cast<double>(end - start) / numPoints;
    for (int i = 0; i < numPoints; ++i)
        arr[i] = start + i * step;

    return arr;
}


/*
    Pushes the buildBrainWave(s) into the helmet.

    values modified in neureset effect here
*/
void Agent::helmet() {
    neureset->helmet(&brainWave);
}
