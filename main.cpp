#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
using namespace std;
#include "olcNoiseMaker.h" //Include the code to interact with the hardware; Provided by onelonecoder.com



//Frequency to angular velocity function
double w(double dHz) {
    return dHz * 2.0 * M_PI;
}
//Define oscillations for easier editting
#define osc_sine 0
#define osc_square 1
#define osc_triangle 2
#define osc_saw_analogue 3
#define osc_saw_digital 4
#define osc_noise 5

double osc(double dHz, double dTime, int nType = osc_sine, double dLFOHz = 0.0, double dLFOAmp = 0.0) {
    //Base frequency
    double dFreq = w(dHz) * dTime + dLFOAmp * dHz * sin(w(dLFOHz) * dTime);
    
    switch (nType){
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
        double dOut = 0.0;
        for (double j = 1.0; j < 40.0; j++) {
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

//Create a structure envelope for the [Attack Decay Sustain Release]
struct sEnvelope {
    double dAtkTime;
    double dDecayTime;
    double dReleaseTime;
    double dSustainAmp;
    double dInitialAmp;
    double dTriggerOn;
    double dTriggerOff;
    bool bNoteOn;
    //sEnvelope() {
    //    dAtkTime = 0.10;
    //    dDecayTime = 0.01;
    //    dReleaseTime = 0.20;
    //    dSustainAmp = 0.8;
    //    dInitialAmp = 1.0;
    //    dTriggerOn = 0.0;
    //    dTriggerOff = 0.0;
    //    bNoteOn = false;
    //}

    void NoteOn(double dTimeOn) {
        dTriggerOn = dTimeOn;
        bNoteOn = true;
    }
    void NoteOff(double dTimeOff) {
        dTriggerOff = dTimeOff;
        bNoteOn = false;
    }

    //Create function to get the amplitude
    double GetAmplitude(double dTime) {
        double dAmp = 0.0;
        double dLifeTime = dTime - dTriggerOn;
        if (bNoteOn) {
            //Attack
            if (dLifeTime <= dAtkTime) {
                dAmp = (dLifeTime / dAtkTime) * dInitialAmp;
            }
            //Decay
            if (dLifeTime > dAtkTime && dLifeTime <= (dAtkTime + dDecayTime)) {
                dAmp = ((dLifeTime - dAtkTime) / dDecayTime) * (dSustainAmp - dInitialAmp) + dInitialAmp;
            }
            //Sustain
            if (dLifeTime > (dAtkTime + dDecayTime)) {
                dAmp = dSustainAmp;
            }
        }
        else {
            //Release
            dAmp = ((dTime - dTriggerOff) / dReleaseTime) * (0.0 - dSustainAmp) + dSustainAmp;
        }
        if (dAmp <= 0.0001) {
            dAmp = 0;
        }
        return dAmp;
    }
};

//Global synth variables
atomic<double> dFrequency = 0.0;
sEnvelope envelope;
double dOctaveBaseFrequency = 220.0;
double d12thRoot2 = pow(2.0, 1.0 / 12.0);

//Structure of instruments
struct instrument
{
    double dVolume;
    sEnvelope env;
    virtual double sound(double dTime, double dFrequency) = 0;
};



struct bell : public instrument
{
    bell() {
        env.dAtkTime = 0.01;
        env.dDecayTime = 1.0;
        env.dInitialAmp = 1.0;
        env.dSustainAmp = 0.0;
        env.dReleaseTime = 1.0;
    }
    double sound(double dTime, double dFreq) {
        double dOutput = envelope.GetAmplitude(dTime) *
            (
                + 1.0 * osc(dFrequency, dTime, osc_sine, 5.0, 0.01)
                + 0.5 * osc(dFrequency * 2.0, dTime, osc_sine)
                + 0.25 * osc(dFrequency * 3.0, dTime, osc_sine)
            );
        return dOutput;
    }
};

struct harmonica : public instrument
{
    harmonica() {
        env.dAtkTime = 0.01;
        env.dDecayTime = 1.0;
        env.dInitialAmp = 1.0;
        env.dSustainAmp = 0.0;
        env.dReleaseTime = 1.0;
    }
    double sound(double dTime, double dFreq) {
        double dOutput = envelope.GetAmplitude(dTime) *
            (
                +1.0 * osc(dFrequency, dTime, osc_square, 5.0, 0.01)
                + 0.5 * osc(dFrequency * 2.0, dTime, osc_square)
                + 0.25 * osc(dFrequency * 3.0, dTime, osc_noise)
                );
        return dOutput;
    }
};

instrument *voice = nullptr;

//Function that generates sound waves.
double CreateNoise(double nChannel, double dTime){
    double dOutput = voice->sound(dTime, dFrequency);
    return dOutput * 0.5;
}
//Harmonica
            //+ 1.0 * osc(dFrequency, dTime, osc_square, 5.0, 0.01)
            //+ 0.5 * osc(dFrequency * 1.5, dTime, osc_square)
            //+ 0.25 * osc(dFrequency * 2.0, dTime, osc_square)
            //+ 0.05 * osc(0, dTime, osc_noise)

int main(){
    wcout << "Synthesizer" << endl;
    //Retrieve sound hardware on device
    vector<wstring> devices = olcNoiseMaker<short>::Enumerate();
    //Display sound hardware on device
    for(auto d : devices) wcout << "Found Sound Hardware:" << d << endl;
    //Create sound machine, specify frequency, stereo value to false, latency values
    olcNoiseMaker<short> sound(devices[0], 44100, 1, 8, 512);
    voice = new bell();
    //Link the create noise function with the sound machine
    sound.SetUserFunction(CreateNoise); 

    bool bKeyPress = false;
    int nPressedKey = -1;

    while (1) {
        //Add a piano keyboard to easier control the audio
        bKeyPress = false;
        for (int i = 0;i < 16;i++) {
            //Maps keyboard presses to different frequencies
            if (GetAsyncKeyState((unsigned char)("ZSXCFVGBNJMK\xbcL\xbe\xbf"[i])) & 0x8000) {
                if (nPressedKey != i) {
                    dFrequency = dOctaveBaseFrequency * pow(d12thRoot2, i);
                    envelope.NoteOn(sound.GetTime());
                    wcout << "\rNote ON : " << sound.GetTime() << "s " << dFrequency << "Hz";
                    nPressedKey = i;
                }
                bKeyPress = true;
            }
        }
        //Turns audio off if no key is being pressed
        if (!bKeyPress) {
            if (nPressedKey != -1) {
                wcout << "\rNote Off: " << sound.GetTime() << "s";
                envelope.NoteOff(sound.GetTime());
                nPressedKey = -1;
            }
        }
    }

    return 0;
}
