#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <list>
#include <algorithm>
using namespace std;
#define FTYPE double
#include "olcNoiseMaker.h" //Include the code to interact with the hardware; Provided by onelonecoder.com

namespace synthesizer {
    //Frequency to angular velocity function
    FTYPE w(const FTYPE dHz) {
        return dHz * 2.0 * M_PI;
    }
    //Basic note
    struct note {
        //Position
        int id;
        FTYPE on;
        FTYPE off;
        bool active;
        int channel;
        note() {
            id = 0;
            on = 0.0;
            off = 0.0;
            active = false;
            channel = 0;
        }
    };
    //Define oscillations for easier editting
    const int osc_sine = 0;
    const int osc_square = 1;
    const int osc_triangle = 2;
    const int osc_saw_analogue = 3;
    const int osc_saw_digital = 4;
    const int osc_noise = 5;

    FTYPE osc(const FTYPE dTime, const FTYPE dHz, const int nType = osc_sine, const FTYPE dLFOHz = 0.0, const FTYPE dLFOAmp = 0.0, FTYPE dCust = 50.0) {
        //Base frequency
        FTYPE dFreq = w(dHz) * dTime + dLFOAmp * dHz * sin(w(dLFOHz) * dTime);

        switch (nType) {
            //Sine wave
        case osc_sine:
            return sin(dFreq);
            //Square wave
        case osc_square:
            return sin(dFreq) > 0.0 ? 1.0 : -1.0;
            //Triangle wave
        case osc_triangle:
            return asin(sin(dFreq) * dTime) * (2.0 / M_PI);
            //Saw tooth wave, analogue / slow implementation
        case osc_saw_analogue: {
            FTYPE dOut = 0.0;
            for (FTYPE j = 1.0; j < dCust; j++) {
                dOut += (sin(j * dFreq)) / j;
            }
            return dOut * (2.0 / M_PI);
        }
                             //Saw tooth wave, optimized implementation
        case osc_saw_digital:
            return (2.0 / M_PI) * (dHz * M_PI * fmod(dTime, 1.0 / dHz) - (M_PI / 2.0));
            //Pseudo random noise (WARNING LOUD AND PROB DONT USE THIS)
        case osc_noise:
            return 2.0 * ((double)rand() / (double)RAND_MAX) - 1.0;
            //Return nothing as to not break speakers
        default:
            return 0.0;
        }
    }
    //Scale-Frequency conversions
    const int scale_def = 0;
    FTYPE scale(const int nNote, const int nScale = scale_def) {
        switch (nScale) {
        case scale_def: default:
            return 256 * pow(1.0594630943592952645618252949463, nNote);
        }
    }
    //Envelope
    struct sEnvelope {
        virtual FTYPE amplitude(const FTYPE dTime, const FTYPE dTimeOn, const FTYPE dTimeOff) = 0;
    };

    struct sEnvelopeADSR : public sEnvelope {
        FTYPE dAtkTime;
        FTYPE dDecayTime;
        FTYPE dReleaseTime;
        FTYPE dSustainAmp;
        FTYPE dInitialAmp;
        sEnvelopeADSR() {
            dAtkTime = 0.1;
            dDecayTime = 0.1;
            dReleaseTime = 0.2;
            dSustainAmp = 1.0;
            dInitialAmp = 1.0;
        }
        //Create function to get the amplitude
        virtual FTYPE amplitude(const FTYPE dTime, const FTYPE dTimeOn, const FTYPE dTimeOff) {
            FTYPE dAmp = 0.0;
            FTYPE dReleaseAmp = 0.0;
            //Note = on
            if (dTimeOn > dTimeOff) {
                FTYPE dLifeTime = dTime - dTimeOn;
                if (dLifeTime <= dAtkTime) {
                    dAmp = (dLifeTime / dAtkTime) * dInitialAmp;
                }
                if (dLifeTime > dAtkTime && dLifeTime <= (dAtkTime + dDecayTime)) {
                    dAmp = ((dLifeTime - dAtkTime) / dDecayTime) * (dSustainAmp - dInitialAmp) + dInitialAmp;
                }
                if (dLifeTime > (dAtkTime + dDecayTime)) {
                    dAmp = dSustainAmp;
                }
            }
            //Note = off
            else {
                FTYPE dLifeTime = dTimeOff - dTimeOn;
                if (dLifeTime <= dAtkTime) {
                    dReleaseAmp = (dLifeTime / dAtkTime) * dInitialAmp;
                }
                if (dLifeTime > dAtkTime && dLifeTime <= (dAtkTime + dDecayTime)) {
                    dReleaseAmp = ((dLifeTime - dAtkTime) / dDecayTime) * (dSustainAmp - dInitialAmp) + dInitialAmp;
                }
                if (dLifeTime > (dAtkTime + dDecayTime)) {
                    dReleaseAmp = dSustainAmp;
                }
                dAmp = ((dTime - dTimeOff) / dReleaseTime) * (0.0 - dReleaseAmp) + dReleaseAmp;
            }
            //+ve Amp
            if (dAmp <= 0.0) {
                dAmp = 0;
            }

            return dAmp;
        }
    };

    FTYPE env(const FTYPE dTime, sEnvelope& env, const FTYPE dTimeOn, const FTYPE dTimeOff) {
        return env.amplitude(dTime, dTimeOn, dTimeOff);
    }

        //Structure of instruments
        struct instrument
        {
            FTYPE dVolume;
            synthesizer::sEnvelopeADSR env;
            virtual FTYPE sound(const FTYPE dTime, synthesizer::note n, bool &NoteFinished) = 0;
        };

        struct bell : public instrument
        {
            bell() {
                env.dAtkTime = 0.01;
                env.dDecayTime = 1.0;
                env.dSustainAmp = 0.0;
                env.dReleaseTime = 1.0;
                dVolume = 1.0;
            }
            virtual FTYPE sound(const FTYPE dTime, synthesizer::note n, bool& NoteFinished) {
                FTYPE dOutput = synthesizer::env(dTime, env, n.on, n.off);
                if (dOutput <= 0.0)
                    NoteFinished = true;
                FTYPE dSound =
                    (
                        +1.0 * synthesizer::osc(n.on - dTime, synthesizer::scale(n.id + 12), synthesizer::osc_sine, 5.0, 0.001)
                        + 0.5 * synthesizer::osc(n.on - dTime, synthesizer::scale(n.id + 24))
                        + 0.25 * synthesizer::osc(n.on - dTime, synthesizer::scale(n.id + 36))
                        );
                return dOutput * dSound * dVolume;
            }
        };
        struct harmonica : public instrument
        {
            harmonica() {
                env.dAtkTime = 0.05;
                env.dDecayTime = 1.0;
                env.dSustainAmp = 0.95;
                env.dReleaseTime = 0.1;
                dVolume = 1.0;
            }
            virtual FTYPE sound(const FTYPE dTime, synthesizer::note n, bool& NoteFinished) {
                FTYPE dOutput = synthesizer::env(dTime, env, n.on, n.off);
                if (dOutput <= 0.0)
                    NoteFinished = true;
                FTYPE dSound =
                    +1.0 * synthesizer::osc(n.on - dTime, synthesizer::scale(n.id), synthesizer::osc_square, 5.0, 0.001)
                    + 0.5 * synthesizer::osc(n.on - dTime, synthesizer::scale(n.id + 12), synthesizer::osc_square)
                    + 0.25 * synthesizer::osc(n.on - dTime, synthesizer::scale(n.id + 24), synthesizer::osc_noise);
                    return dOutput * dSound * dVolume;
            }
        };
}

    vector<synthesizer::note> vecNote;
    mutex muxNote;
    synthesizer::bell instBell;
    synthesizer::harmonica instHarm;

    typedef bool(*lambda)(synthesizer::note const& item);
    template<class T>
    void remove_safely(T& v, lambda f) {
        auto u = v.begin();
        while (u != v.end())
            if (!f(*u))
                u = v.erase(u);
            else
                ++u;
    }

    //Function that generates sound waves.
    double CreateNoise(int nChannel, FTYPE dTime) {
        unique_lock<mutex> lm(muxNote);
        FTYPE dMixedOutput = 0.0;
        for (auto& n : vecNote) {
            bool NoteFinished = false;
            FTYPE dSound = 0;
            if (n.channel == 2)
                dSound = instBell.sound(dTime, n, NoteFinished);
            if (n.channel == 1)
                dSound = instHarm.sound(dTime, n, NoteFinished) * 0.5;
            dMixedOutput += dSound;
            if (NoteFinished && n.off > n.on)
                n.active = false;
        }
        //Lmao bru
        remove_safely<vector<synthesizer::note>>(vecNote, [](synthesizer::note const& item) {return item.active; });
        return dMixedOutput * 0.2;
    }

    int main() {
        wcout << "Synthesizer" << endl;
        //Retrieve sound hardware on device
        vector<wstring> devices = olcNoiseMaker<short>::Enumerate();
        //Display sound hardware on device
        for (auto d : devices) wcout << "Found Sound Hardware:" << d << endl;
        //Create sound machine, specify frequency, stereo value to false, latency values
        olcNoiseMaker<short> sound(devices[0], 44100, 1, 8, 512);
        
        


        // I COMMENTED THIS OUT BECAUSE ITS NOT USEFUL ANYMORE
        // voice = new bell();

        
        
        
        //Link the create noise function with the sound machine
        sound.SetUserFunction(CreateNoise);

        char keeb[129];
        memset(keeb, ' ', 127);
        keeb[128] = '\0';
        auto clock_old = chrono::high_resolution_clock::now();
        auto clock_real = chrono::high_resolution_clock::now();
        double dTimeElapsed = 0.0;

        while (1) {
            //Add a piano keyboard to easier control the audio
            for (int i = 0; i < 16; i++) {
                short nKeyState = GetAsyncKeyState((unsigned char)("ZSXCFVGBNJMK\xbcL\xbe\xbf"[i]));
                double dTimeCurr = sound.GetTime();
                //Check if note already being played
                muxNote.lock();
                auto FindNote = find_if(vecNote.begin(), vecNote.end(), [&i](synthesizer::note const& item) {return item.id == i; });
                if (FindNote == vecNote.end()) {
                    //Note not found
                    if (nKeyState & 0x8000) {
                        //Create new note
                        synthesizer::note n;
                        n.id = i;
                        n.on = dTimeCurr;
                        n.channel = 1;
                        n.active = true;
                        //Add note
                        vecNote.emplace_back(n);
                    }
                    else {
                        //Do nothing
                    }
                }
                else {
                    //Note found
                    if (nKeyState & 0x8000) {
                        //Key still there, do not touch
                        if (FindNote->off > FindNote->on) {
                            //Key pressed again during release
                            FindNote->on = dTimeCurr;
                            FindNote->active = true;
                        }
                    }
                    else {
                        //Switch off after key is released
                        if (FindNote->off < FindNote->on) {
                            FindNote->off = dTimeCurr;
                        }
                    }
                }
                muxNote.unlock();
            }
            wcout << "\rNotes: " << vecNote.size() << "    ";
        }
        return 0;
    }
