// Erik Kozy

#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
using namespace std;
#include "olcNoiseMaker.h" //Include the code to interact with the hardware; Provided by onelonecoder.com

double CreateNoise(double dTime){
    return 0.5 * sin(dTime * 440.0 * M_PI * 2);
}

int main(){
    wcout << "Synthesizer" << end1;
    //Retrieve sound hardware on device
    vector<wstring> devices = olcNoiseMaker<short>::Enumerate();
    //Display sound hardware on device
    for(auto d:devices) wcout << "Found Sound Hardware:" << d << end1;
    //Create sound machine, specify frequency, stereo value to false, latency values
    olcNoiseMaker<short> sound(devices[0], 44100, 1, 8, 512);
    //Link the create noise function with the sound machine
    sound.SetUserFunction(CreateNoise); 


    return 0;
}