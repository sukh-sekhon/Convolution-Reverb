#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fstream>
#include <iostream>
#include <chrono>

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

/* Function prototypes */
Wave* readFile(Wave* wave, char *fileName);
double* convolve(double x[], int N, double h[], int M, double y[], int P);
void writeFile(char *outputFileName, double *output, int outputSampleRate, int outputLength);


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
    auto start = chrono::high_resolution_clock::now();
    double* output = convolve(
            input->signal, input->signal_size,
            impulse->signal, impulse->signal_size,
            output, outputSize);
    auto finish = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = finish - start;
    cout << "Unoptimized timing for optimization 1: " << elapsed.count() << " seconds" << endl;

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
    int n, m;
    y = (double *) malloc (sizeof(double) * P);

    /*  Make sure the output buffer is the right size: P = N + M - 1  */
    if (P != (N + M - 1)) {
        cout << "Invalid output signal size. Aborting Convolution." << endl;
        return y;
    }

    /*  Clear the output buffer y[] to all zero values  */
    for (n = 0; n < P; n++)
        y[n] = 0.0;

    /*  Outer loop:  process each input value x[n] in turn
        Inner loop:  process x[n] with each sample of h[]  */
    for (n = 0; n < N; n++)
        for (m = 0; m < M; m++)
            y[n + m] += x[n] * h[m];

    return y;
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