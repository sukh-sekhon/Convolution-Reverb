# Convolution Reverb Processor 
[![forthebadge](https://forthebadge.com/images/badges/made-with-C-Plus-Plus.svg)](Reverb.cpp)

## What is Convolution Reverb?
Convolution reverb is used in audio signal processing to simulate reverberations from different environments. By using an impulse response from a specific environment, any audio signal can be transformed to sound as if it was recorded there.

## Demo
Play these sample files to see this in action:

[![forthebadge](https://img.shields.io/badge/input-gainsboro?style=for-the-badge&logo=asciinema&logoColor=red)](https://github.com/sukhjot-sekhon/Convolution-Reverb/raw/master/Audio%20Files/inputGuitar.wav) [![forthebadge](https://img.shields.io/badge/impulse%20response-gainsboro?style=for-the-badge&logo=asciinema&logoColor=red)](https://github.com/sukhjot-sekhon/Convolution-Reverb/raw/master/Audio%20Files/impulseTajMahal.wav) [![forthebadge](https://img.shields.io/badge/output-red?style=for-the-badge&logo=asciinema&logoColor=white)](https://github.com/sukhjot-sekhon/Convolution-Reverb/raw/master/Audio%20Files/output.wav)

## Features
* __Reading and writing WAVE files__
* Performing __Convolution Reverb__

## Usage
To use the most optimized version:
1. Clone the repo `git clone https://github.com/sukh-sekhon/Convolution-Reverb`
2. Compile the file with compiler-level optimizations `gcc -Ofast Reverb.cpp -o Reverb`
3. Run the program with input and impulse response WAVE files `Reverb input.wav impulse.wav output.wav`
  
## Optimizations
My initial version of this tool was very slow, with some optimizations, performance was improved by **742,022%**
* Changed from input side convolution algorithm to a *fast Fourier transform*
* Precomputing results of expensive operations
* Strength reduction in computing the next greatest power of two
* Partially unrolling loops
* Minimizing array references
* Caching values instead of recomputing
* Jamming loops together
* Compiler-level optimizations
