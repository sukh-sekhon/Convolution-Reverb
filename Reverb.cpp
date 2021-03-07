#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fstream>
#include <iostream>
#include <chrono>

#define PI         3.141592653589793
#define TWO_PI     (2.0 * PI)
#define SWAP(a,b)  tempr=(a);(a)=(b);(b)=tempr

using namespace std;

typedef struct {
    /* RIFF chunk descriptor */
    char    chunk_id[4];
    int     chunk_size;
    char    format[4];
    /* Fmt sub-chunk */
    char    fmt_chunk_id[4];
    int     fmt_chunk_size;
    short   audio_format;
    short   num_channels;
    int     sample_rate;
    int     byte_rate;
    short   block_align;
    short   bits_per_sample;
    /* Data sub-chunk */
    char    data_chunk_id[4];
    int     data_chunk_size;
    short*  data;
    /* Scaled down signal data */
    double* signal;
    int     signal_size;
} Wave ;

/* Cached values from four1 function */
double* cache = (double *) malloc (1024);
char cacheType = 0;

/* Function prototypes */
Wave* readFile(Wave* wave, char *fileName);
double* convolve(double x[], int N, double h[], int M, double y[], int P);
void writeFile(char *outputFileName, double *output, int outputSampleRate, int outputLength);
void four1(double data[], int nn);

/**
 * Convolves input and impulse response .wav files into an output .wav file
 *
 * arguments:
 *              first: name of input .wav file
 *              second: name of impulse response .wav file
 *              third: name of .wav file to output
 */
int main (int argc, char *argv[]) {
    if (argc != 4) {
        fputs ("Invalid s (use: input.wav impulseResponse.wav output.wav)", stderr); exit (-1) ;
    }
    char *inputFileName = argv[1], *impulseFileName = argv[2], *outputFileName = argv[3];

    /* Read the input and impulse response files */
    cout << "Reading input file " << inputFileName << endl;
    Wave* input = (Wave *) malloc(sizeof(Wave));
    input = readFile(input, inputFileName);

    cout << "Reading impulse response file " << impulseFileName << endl;
    Wave* impulse = (Wave *) malloc(sizeof(Wave));
    impulse = readFile(impulse, impulseFileName);

    /* Convolve */
    int outputSize = input->signal_size + impulse->signal_size - 1;
    cout << "Convolving" << endl;
    double* output = convolve(
            input->signal, input->signal_size,
            impulse->signal, impulse->signal_size,
            output, outputSize);

    cout << "Scaling Data" << endl;
    /* Find maximum absolute output value */
    double maxAbsOutput = 0;
    for (int i = 0; i < outputSize; i ++)
        if (abs(output[i]) > maxAbsOutput)
            maxAbsOutput = abs(output[i]);
    for (int i = 0; i < outputSize; i ++)
        output[i] /= maxAbsOutput;

    /* Write to output file */
    cout << "Writing to " << outputFileName << endl;
    writeFile(outputFileName, output, input->sample_rate, outputSize);

    /* free allocated memory */
    free(input);
    free(impulse);
    free(output);
    free(cache);
    return 0;
}


/**
 * Reads and parses the contents of a .wav file
 *
 * arguments:
 *              wave: location to store data
 *              fileName: file to open
 * Returns:
 *              pointer to stored data
 */
Wave* readFile(Wave* wave, char *fileName) {
    FILE* fp = fopen(fileName, "r");

    /* Throw error if issues encountered reading wave file */
    if (fp==NULL) {
        fputs ("Error reading wave file", stderr); exit (-1) ;
    }

    /* RIFF chunk descriptor */
    fread(&wave->chunk_id, sizeof(char), 4, fp);
    fread(&wave->chunk_size, sizeof(int), 1, fp);
    fread(&wave->format, sizeof(char), 4, fp);

    /* Fmt sub-chunk */
    fread(&wave->fmt_chunk_id, sizeof(char), 4, fp);
    fread(&wave->fmt_chunk_size, sizeof(int), 1, fp);
    fread(&wave->audio_format, sizeof(short), 1, fp);
    fread(&wave->num_channels, sizeof(short), 1, fp);
    fread(&wave->sample_rate, sizeof(int), 1, fp);
    fread(&wave->byte_rate, sizeof(int), 1, fp);
    fread(&wave->block_align, sizeof(short), 1, fp);
    fread(&wave->bits_per_sample, sizeof(short), 1, fp);

    /* Seek if the chunk size isn't standard (16) */
    if (wave->fmt_chunk_size != 16)
        fseek(fp, wave->fmt_chunk_size - 16, SEEK_CUR);

    /* Data sub-chunk */
    fread(&wave->data_chunk_id, sizeof(char), 4, fp);
    fread(&wave->data_chunk_size, sizeof(int), 1, fp);

    int sampleCount = wave->data_chunk_size / (wave->bits_per_sample / 8);
    wave->data = new short[sampleCount];
    wave->signal = new double[sampleCount];
    wave->signal_size = sampleCount;

    /* Scaling down signal */
    for (int i = 0; i < sampleCount; i++) {
        fread(&wave->data[i], 1, wave->bits_per_sample / 8, fp);
        wave->signal[i] = wave->data[i] / (double) 32768.0;
    }

    fclose(fp);
    return wave;
}


/**
 * Convolves the data from an input and impulse response file
 * The data must be scaled to a range of -1.0 to 1.0
 *
 * arguments:
 *              x: input signal
 *              N: input signal size
 *              h: impulse response signal
 *              M: impulse response signal size
 *              y: output signal
 *              P: output signal size
 * Returns:
 *              output signal
 */
double* convolve(double* x, int N, double* h, int M, double* y, int P) {
    y = (double *) malloc (sizeof(double) * P);

    /* Size of padded array ((value^2)*2 s.t. value = max(N, M))*/
    auto start = chrono::high_resolution_clock::now();
    int arrayLength = pow(2, ceil(log(max(N, M))/log(2))) * 2;
    auto finish = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = finish - start;
    cout << "Unoptimized timing for optimization 3: " << elapsed.count() << " seconds" << endl;


    /* Initialize arrays to undergo FFT to 0 (sets the imaginary component) */
    auto *input = new double[arrayLength] {};
    auto *impulse = new double[arrayLength] {};
    auto *output = new double[arrayLength] {};

    /* Set the real components and perform FFT */
    for (int i = 0; i < N; i++)
        input[i*2] = x[i];
    for (int i = 0; i < M; i++)
        impulse[i*2] = h[i];

    four1(input - 1, arrayLength / 2);
    four1(impulse - 1, arrayLength / 2);

    /* (a,b)*(c,d) = (ac-bd, ad+bc) = (real output, complex output) where:
     *      a is the real component of input
     *      b is the complex component of input
     *      c is the real component of impulse
     *      d is the complex component of input */
    for (int i = 0; i < arrayLength; i+=2) {
        output[i] = input[i] * impulse[i] - input[i+1] * impulse[i+1];
        output[i+1] = input[i+1] * impulse[i] + input[i] * impulse[i+1];
    }

    /* Performs inverse DFT to get the output values */
    four1(output - 1, arrayLength / 2);

    /* Discard any imaginary components */
    for (int i = 0; i < P*2; i+=2)
        y[i/2] = output[i];

    return y;
}

/**
 * Adopted from Numerical Recipes in C: The Art of Scientific Computing (pages 507,508)
 */
void four1(double data[], int nn) {
    unsigned long n, mmax, m, j, istep, i;
    double wtemp, wr, wpr, wpi, wi, theta;
    double tempr, tempi;

    n = nn << 1;
    j = 1;

    for (i = 1; i < n; i += 2) {
        if (j > i) {
            SWAP(data[j], data[i]);
            SWAP(data[j+1], data[i+1]);
        }
        m = nn;
        while (m >= 2 && j > m) {
            j -= m;
            m >>= 1;
        }
        j += m;
    }

    auto start = chrono::high_resolution_clock::now();
    mmax = 2;
    unsigned short counter = 0;
    while (n > mmax) {
        istep = mmax << 1;

        /* If running function for first time, calculate values and cache them */
        if (cacheType == 0) {
            theta = TWO_PI / mmax;
            wtemp = sin(0.5 * theta);
            wpr = -2.0 * wtemp * wtemp;
            wpi = sin(theta);

            cache[counter] = wpr;
            cache[counter + 1] = wpi;
        }
        /* If running function with type 2, retrieve from cache */
        else if(cacheType == 1) {
            wpr = cache[counter];
            wpi = cache[counter + 1];
        }
        /* If running function with type 3, retrieve from cache and perform small compuatation */
        else {
            wpr = cache[counter];
            wpi = cache[counter + 1] * - 1;
        }

        wr = 1.0;
        wi = 0.0;
        for (m = 1; m < mmax; m += 2) {
            for (i = m; i <= n; i += istep) {
                j = i + mmax;
                tempr = wr * data[j] - wi * data[j+1];
                tempi = wr * data[j+1] + wi * data[j];
                data[j] = data[i] - tempr;
                data[j+1] = data[i+1] - tempi;
                data[i] += tempr;
                data[i+1] += tempi;
            }
            wr = (wtemp = wr) * wpr - wi * wpi + wr;
            wi = wi * wpr + wtemp * wpi + wi;
        }
        mmax = istep;
        counter += 2;
    }
    cacheType++;
    auto finish = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = finish - start;
    cout << "Optimized timing for optimization 2: " << elapsed.count() << " seconds" << endl;
}


/**
 * Writes output signal as a .wav file
 *
 * arguments:
 *              ouputFileName: Name of file to output
 *              output: Pointer to output signal (non-scaled)
 *              outputSampleRate: Sample rate of output signal
 *              outputLength: Length of output signal
 */
void writeFile(char *outputFileName, double *output, int outputSampleRate, int outputLength) {
    FILE* fp = fopen(outputFileName, "w");

    /* Setup RIFF chunk descriptor */
    char    chunkID[] = {'R', 'I', 'F', 'F'};
    int     chunkSize = 36 + (outputLength * sizeof(short));
    char    format[] = {'W', 'A', 'V', 'E'};
    /* Setup fmt sub-chunk */
    char    fmtChunkID[] = {'f', 'm', 't', ' '};
    int     fmtChunkSize = 16;
    short   audioFormat = 1;
    short   numChannels = 1;
    int     sampleRate = outputSampleRate;
    int     byteRate = sampleRate * sizeof(short);
    int     blockAlign = sizeof(short);
    int     bitsPerSample = 8 * sizeof(short);
    /* Set up data sub-chunk */
    char    dataChunkID[] = {'d', 'a', 't', 'a'};
    int     outputSize = outputLength * sizeof(short);

    /* Write RIFF chunk descriptor */
    fwrite(chunkID, sizeof(char), sizeof(chunkID), fp);
    fwrite((char *) &chunkSize, sizeof(int), 1, fp);
    fwrite(format, sizeof(char), sizeof(format), fp);
    /* Write fmt sub-chunk */
    fwrite(fmtChunkID, sizeof(char), sizeof(chunkID), fp);
    fwrite((char *) &fmtChunkSize, sizeof(int), 1, fp);
    fwrite((char *) &audioFormat, sizeof(short), 1, fp);
    fwrite((char *) &numChannels, sizeof(short), 1, fp);
    fwrite((char *) &sampleRate, sizeof(int), 1, fp);
    fwrite((char *) &byteRate, sizeof(int), 1, fp);
    fwrite((char *) &blockAlign, sizeof(short), 1, fp);
    fwrite((char *) &bitsPerSample, sizeof(short), 1, fp);
    /* Write data sub-chunk */
    fwrite(dataChunkID, sizeof(char), sizeof(dataChunkID), fp);
    fwrite((char *) &outputSize, sizeof(int), 1, fp);

    /* Write scaled data */
    for (int i = 0; i < outputLength; i ++) {
        auto scaledValue = (short) (output[i] * 32767.0);
        fwrite((char *) &scaledValue, sizeof(short), 1, fp);
    }

    fclose(fp);
}