#include "neureset.h"

Neureset* Neureset::instance = nullptr;

// Constructor
Neureset::Neureset() : samplingRate(MAX_FREQ * MAX_SAMPLES), samplingRateDiv2(samplingRate / 2),
                       maxNoise(NOISE_FLOOR / 2.),

                       domainTime(linspace(0, 1, samplingRate)), ampTime(samplingRate, 0.),

                       domainDFT(linspace(0, MAX_FREQ, samplingRateDiv2)), ampDFT(samplingRateDiv2, 0.),

                       treat(false), isPause(false),

                       treatAmp(0.), treatFreq(0.), progress(0),

                       dft(constructDFT()), real(new double[samplingRateDiv2]()),
                    //    imag(new double[samplingRateDiv2]()),

                       gen(rd()), dis(-maxNoise, maxNoise) {

    std::cout << "Sampling Rate: " << samplingRate << std::endl;
    std::cout << "Max Frequency: " << MAX_FREQ << std::endl;
    std::cout << "Max Samples: " << MAX_SAMPLES << std::endl;

    site = -1;
}

// Destructor
Neureset::~Neureset() {
    stopTreatment();

    for (int i = 0; i < samplingRate; ++i)
        delete[] dft[i];
    delete[] dft;

    delete[] real;
    // delete[] imag;
}


/*
    Generates evenly spaced horizontal axis.

    Used for frequency and time domains.

    Used in plots.
*/
QVector<double> Neureset::linspace(const int start, const int end, const int num_points) {
    QVector<double> vec;
    vec.resize(num_points);

    const double step = static_cast<double>(end - start) / num_points;

    for (int i = 0; i < num_points; ++i)
        vec[i] = start + i * step;

    return vec;
}


/*
    Construct a partial DFT matrix - reduces unnecessary recalculations.

    Partial transformation matrix mapping time to frequency domains.

    returns:
        2D matrix
*/
double** Neureset::constructDFT() {
    double** dftMatrix = new double* [samplingRate];
    for (int i = 0; i < samplingRate; ++i)
        dftMatrix[i] = new double[samplingRateDiv2]();

    const double two_pi = 2. * PI;
    const double factor = PI / samplingRate;

    for (int n = 0; n < samplingRate; ++n)
        for (int k = 0; k < samplingRateDiv2; ++k) {
            double angle = factor * k * n;

            // [0, 2 * PI)
            angle = fmod(angle, two_pi);
            dftMatrix[n][k] = angle;
        }

    return dftMatrix;
}


/*
    Helmet is attached.

    Controlled from Agent.
*/
void Neureset::helmet(double* const* const* brain) {
    mtx.lock();
    this->brain = brain;
    mtx.unlock();
}


/*
    Select a row in brain.

    Controlled from UI and internally.
*/
void Neureset::setSite(const int site) {
    mtx.lock();
    this->site = site;
    generator();
    mtx.unlock();
}


/*
    Generates noise for dynamical signal visual.

    Aggregates notes, brain site, and artificial treatment signal.

    calls dftRunner to calculate max amp and freq.
*/
void Neureset::generator() {

    for (int i = 0; i < samplingRate; ++i) {
        // stops accumulation of values
        // amps[i] = 0.; // if you dont want jitter
        ampTime[i] = dis(gen);

        // test frequency injection
        // ampTime[i] += 1. * std::cos(2. * PI * 30 * domainTime[i]);

        // signal - if valid in range from [0, 20]
        if (site > -1 && site < NUM_BRAIN_SITES)
            ampTime[i] += (*brain)[site][i];

        // treatment - visual only
        ampTime[i] += treatAmp * std::cos(2. * PI * treatFreq * domainTime[i]);
    }

    dftRunner();
}


/*
    Generates the DFT.

    Finds peak and amplitude.
*/
void Neureset::dftRunner() {

    for (int k = 0; k < samplingRateDiv2; ++k) { // each frequency bin
        real[k] = 0.;
        // imag[k] = 0.;

        // complete rotatation matrix
        for (int n = 0; n < samplingRate; ++n) {
            angle = dft[n][k];
            real[k] += ampTime[n] * cos(angle); // real component arbitrary cos
            // imag[k] += ampTime[n] * sin(angle); // imaginary component (any phase shift relative to cos)
        }

        // horizontal scaling
        real[k] /= samplingRateDiv2;
        // imag[i] /= samplingRateDiv2;

        // dft solution here
        ampDFT[k] = std::abs(real[k]);
        // ampDFT[i] = std::sqrt(real[i]*real[i] + imag[i]*imag[i]);
    }

    maxIndex = 0;
    maxValue = 0.;

    // define max freq and its amplitude from dft
    for (int i = 0; i < samplingRateDiv2; ++i)
        if (ampDFT[i] > maxValue) {
            maxValue = ampDFT[i];
            maxIndex = i;
        }

    // dft freq and amp here
    peakFreq = domainDFT[maxIndex];
    peakFreqAmp = maxValue;
}


/*
    Calcualte local average dominant frequency.

    Not utilized in downstream calculations, cout only.

    Controlled from UI.

    Blocking.

    Initiates delay timer on green light - following customer requirements.
*/
void Neureset::pretreatment() { // delay 1)
    treat = true;
    double localBaseline = 0.;
    int loops = 5;

    for (int i = 0; i < loops; ++i) { // 5 x 1 second delay
        if (!treat)
            return;

        while (isPause) // user pauses treatment
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        localBaseline += peakFreq;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    localBaseline /= loops;
    std::cout << " Pretreatment analysis local baseline: " << localBaseline << std::endl;
}


/*
    Use DFT solved amplitude and frequency to determine artificial and real determine treatment.

    Controlled from UI.

    Blocking.
*/
void Neureset::treatment() {

    pretreatment();

    for (int i = 1; i < NUM_OFFSETS + 1; ++i) { // 5 10 15 20 offset freq treatment
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // delay 2)

        while (isPause) // user pauses treatment
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        mtx.lock();

        if (!treat) {
            treatAmp = 0.;
            mtx.unlock();
            return;
        }

        // artificial treatment - visual
        treatFreq = peakFreq + 5 * i;
        treatAmp = peakFreqAmp * 0.5;

        // actual treatment - not visual
        for (int j = 0; j < samplingRate; ++j)
            (*brain)[site][j] -= 0.2 * peakFreqAmp * std::cos(2. * PI * peakFreq * domainTime[j]);

        progress += 1;
        std::cout << progress << std::endl;

        mtx.unlock();

        // show treatment for 1 second
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // delay 3)

        mtx.lock();
        treatAmp = 0.;
        mtx.unlock();

    }

    treat = false;
}


//--------------------------------------------------------------------------------------//
// visual

// Oscilloscope
const QVector<double>& Neureset::getDomainTime() const {
    return domainTime;
}

const QVector<double>& Neureset::getAmpTime() const {
    return ampTime;
}

const QVector<double>& Neureset::getDomainDFT() const {
    return domainDFT;
}

// Spectrum analyzer
const QVector<double>& Neureset::getAmpDFT() const {
    return ampDFT;
}


// strongest freq
const double& Neureset::getDomFreq() const {
    return peakFreq;
}

// amplitude
const double& Neureset::getPeakFreqAmp() const {
    return peakFreqAmp;
}

// progress bar
const int& Neureset::getProgress() const {
    return progress;
}

// flashing green
const double& Neureset::getTreatAmp() const {
    return treatAmp;
}

//--------------------------------------------------------------------------------------//
// control

bool Neureset::togglePause() {
    isPause = !isPause;
    mtx.lock();
    treatAmp = 0.;
    mtx.unlock();
    return isPause;
}

void Neureset::resetProgress() {
    progress = 0;
}

void Neureset::stopTreatment() {
    treat = false;
    treatAmp = 0;
}

std::mutex& Neureset::getMutex() {
    return mtx;
}



/*
    Calcualte global baseline

    Controlled from UI.

    returns:
        average peak freq over all sites
*/
double Neureset::getOverallBaseline() {
    double temp = 0.;

    for (int i = 0; i < NUM_BRAIN_SITES; ++i) {
        setSite(i);
        temp += peakFreq;
    }

    return temp / NUM_BRAIN_SITES;
}


int Neureset::getSite() const {
    return site;
}

// Singleton get instance
Neureset* Neureset::getInstance() {
    if (instance == nullptr) {
        instance = new Neureset();
    }
    return instance;
}
