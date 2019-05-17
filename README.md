# Convolution Reverb Processing Optimization [![forthebadge](https://forthebadge.com/images/badges/made-with-C-Plus-Plus.svg)](https://forthebadge.com)

## What is Convolution Reverb?
Convolution reverb is used in audio signal processing to simulate reverberations from different environments. By using an impulse response from a specific environment, any audio signal can be transformed to sound as if it was recorded there.

## Usage
To use the most optimized version:
1. Clone the repo ```git clone https://github.com/sukhjot-sekhon/Convolution-Reverb-Processing-Optimization```
2. Compile the file with compiler-level optimizations ```gcc -Ofast ReverbOptimized7b.cpp -o ReverbOptimized7b```
3. Run the program with input and impulse response WAVE files ```ReverbOptimized7b input.wav impulse.wav output.wav```

## Features
* __Reading and writing WAVE files__
* Performs __convolution reverb__

## Optimizations
Overall, these optimizations __improved performance by 742,022%__!
* Changed from input side convolution algorithm to a __fast Fourier transform__
* Precomputing results of expensive operations
* Strength reduction in computing the next greatest power of two
* Partially unrolling loops
* Minimizing array references
* Caching values instead of recomputing
* Jamming loops together
* Compiler-level optimizations
