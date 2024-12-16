#ifndef NEURESET_H
#define NEURESET_H

#include <iostream>
#include <thread>
#include <random>
#include <QVector>
#include <mutex>

#include "defs.h"


class Neureset {
    private:
        const int samplingRate;
        const int samplingRateDiv2;
        const float maxNoise;

        // to UI
        // Oscilloscope
        const QVector<double> domainTime; // cant set values to const
        QVector<double> ampTime;

        // to UI
        // Spectrum analyzer
        const QVector<double> domainDFT;
        QVector<double> ampDFT;

        bool treat;
        bool isPause;

        double treatAmp;
        double treatFreq;
        int progress;

        const double* const* const dft; // matrix of angles - a partial dft matrix - fixed values
        double* const real;             // cos detects real
        // double* const imag;          // can detects imaginary - phase shift (just in case)

        std::random_device rd;
        std::mt19937 gen;
        std::uniform_real_distribution<double> dis;

        //--------------------------------------------------------------------------------------//

        double* const* const* brain;
        int site;

        std::mutex mtx;

        double angle;

        int maxIndex;
        double maxValue;

        double peakFreq;
        double peakFreqAmp;

        QVector<double> linspace(const int start, const int end, const int num_points);

        double** constructDFT();
        void dftRunner();
        void pretreatment();

        explicit Neureset();
        static Neureset* instance;

    public:
        static Neureset* getInstance();
        ~Neureset();

        void helmet(double* const* const* brain);
        void setSite(const int site);

        void generator();
        void treatment();

        bool togglePause();
        void resetProgress();
        void stopTreatment();

        // Oscilloscope
        const QVector<double>& getDomainTime() const;
        const QVector<double>& getAmpTime() const;
        // Spectrum analyzer
        const QVector<double>& getDomainDFT() const;
        const QVector<double>& getAmpDFT() const;

        const double& getDomFreq() const;
        const double& getPeakFreqAmp() const;
        const int& getProgress() const;

        std::mutex& getMutex();

        const double& getTreatAmp() const;

        double getOverallBaseline();
        int getSite() const;
};
#endif
