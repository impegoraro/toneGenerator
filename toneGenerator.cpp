#include <iostream>
#include <string>

#include <cstdio>
#include <cstdlib>
#include <cmath>

#include <cstring>
#include <getopt.h>

using namespace std;

typedef struct {
    int chunkID;
    int chunkSize;
    int format;

    int   Subchunk1ID;
    int   Subchunk1Size;
    short AudioFormat;
    short NumChannels;
    int   SampleRate;
    int   ByteRate;
    short BlockAlign;
    short BitsPerSample;

    int Subchunk2ID;
    int subChunk2Size;

} WaveHeader;


void writeWaveToFile(FILE* file, short* data, int size, int sampleRate, short nChannels, short bitsPerSample)
{
    WaveHeader wave;

    wave.chunkID= 0x46464952;
    wave.chunkSize = 36;
    wave.format = 0x45564157;
    wave.Subchunk1ID = 0x20746d66;
    wave.Subchunk1Size = 16;
    wave.AudioFormat = 1;
    wave.NumChannels = nChannels;
    wave.SampleRate = sampleRate;
    wave.BitsPerSample = bitsPerSample;
    wave.ByteRate = sampleRate * wave.NumChannels * wave.BitsPerSample/8;
    wave.BlockAlign = wave.NumChannels * wave.BitsPerSample/8;

    wave.Subchunk2ID = 0x61746164;
    wave.subChunk2Size = size * wave.NumChannels * wave.BitsPerSample/8;

    if(file == NULL) return;


    fwrite((char*)&wave, 1, sizeof(wave), file);

    //file.write((char*)&wave.Subchunk2ID, sizeof(wave.Subchunk2ID));
    //file.write((char*)&wave.subChunk2Size, sizeof(wave.subChunk2Size));

    fwrite((char*)data, 1, size * sizeof(short), file);
}

void makeFreqIntInner(short* tone, int samplerate, float frequency, int length, float gain)
{
    float A = frequency * 2 * M_PI;

    for (int i = 0; i < length; i++)
        tone[i] = (short)(32767 * sin(A * (float) i/ samplerate));
}

short* makeFreqInt(int samplerate, float frequency, int length, float gain)
{
    short *tone = new short[length];
    makeFreqIntInner(tone, samplerate, frequency, length, gain);
    return tone;
}

void makeToneIntInner(short* tone, int samplerate, float lowFreq, int highFreq, int length, float gain)
{
    float f1 =  ((lowFreq - highFreq)/2) * 2 * M_PI / samplerate;
    float f2 =  ((lowFreq + highFreq)/2) * 2 * M_PI / samplerate;

    for (int i = 0; i < length; i++) {
        tone[i] = (short) 32767 * cos(f1 * (float) i) * cos(f2 * (float) i);
    }
}

short* makeToneInt(int samplerate, float lowFreq, int highFreq, int length, float gain) {
    short *tone = new short[length];
    makeToneIntInner(tone, samplerate, lowFreq, highFreq, length, gain);
    return tone;
}

void makeToneIntSimpleInner(short *toneArr, int sampleRate, char tone, int length, float gain)
{
    //        1209 Hz     1336 Hz     1477 Hz     1633 Hz
    //697 Hz  1              2            3          A
    //770 Hz  4              5            6          B
    //852 Hz  7              8            9          C
    //941 Hz  *              0            #          D  
    float lowFreq, highFreq;

    switch(tone) {
        case '1': case '2': case '3': case 'A':
            lowFreq = 697.0f;
        break;
        
        case '4': case '5': case '6': case 'B':
            lowFreq = 770.0f;
        break;

        case '7': case '8': case '9': case 'C':
            lowFreq = 852.0f;
        break;

        case '*': case '0': case '#': case 'D':
            lowFreq = 941.0f;
        break;

        default:
            lowFreq = -1;
    }

    switch(tone) {
        case '1': case '4': case '7': case '*':
            highFreq = 1209.0;
        break;
        
        case '2': case '5': case '8': case '0':
            highFreq = 1336.0;
        break;

        case '3': case '6': case '9': case '#':
            highFreq = 1470.0;
        break;

        case 'A': case 'B': case 'C': case 'D':
            highFreq = 1633.0;
        break;

        default:
            highFreq = -1;
    }

    cerr<< "Gen: "<<tone<< " ("<< lowFreq<< ", "<<highFreq<<")"<<endl;
    if(lowFreq != -1 && highFreq != -1)
        makeToneIntInner(toneArr, sampleRate, lowFreq, highFreq, length, gain);
    else { 
        cerr<< "Error generating: "<< tone<< endl;
    }
}

short * makeToneIntSimple(int sampleRate, char tone, int length, float gain)
{
    short *toneArr = new short[length];

    makeToneIntSimpleInner(toneArr, sampleRate, tone, length, gain);
    return toneArr;
}

short *samplesFloatInt(float* arr, int length) {
    short *tone = new short[length];

    for (int i = 0; i < length; i++) {
        tone[i]= arr[i] * 32768;
        if(tone[i] > 32767) tone[i] = 32767;
        if(tone[i] < -32768) tone[i] = -32768;
    }

    return tone;
}

short *generateTonesFromString(char *number, int samplerate, int length, int silenceLength)
{
    short *tones = new short[strlen(number) * length + silenceLength * (strlen(number) - 1)];
    short *tones2 = tones;

    while(*number != '\0') {
        makeToneIntSimpleInner(tones2, samplerate, *number, length, 1.0f);
        tones2 += length;

        if(*(number+1) != '\0') {
            makeFreqIntInner(tones2, samplerate, 0, silenceLength, 1.0f);   
            tones2 += silenceLength;
        }
        number++;

    }

    return tones;
}

int csStrLen(char *str)
{
    int len = 0;
    char *c = str;
    do {
        c = strchr(c, ',');
        if(c == NULL)
            break;
        len++;
        c++;
    } while(c != NULL);
    return len+1;
}

/*short* makeMultiFreqs(int samplerate, int argc, char** argv, int length, int *outSize)
{
    int inti = 0;
    *outSize = 0;
    bool lengthChanged = false;
    for(int i=0; i < argv; i++) {
        if(*argv[i] == '.') {
            *outSize += samplerate * length / 1000 * inti;
            length = atoi(argv[i] + 1);
            lengthChanged = true;
        } else 
            inti++;
    }
}*/

void showHelp()
{
    cerr<<"ToneGenerator: tool for generating sound waves."<<endl<<endl
        <<"    --help                                  shows this help message."<<endl
        <<"    --sample-rate <rate>, -s <rate>         changes the sample rate of the generated sound."<<endl
        <<"    --output <file>, -o <file>              specifies the output file. Default data is sent to standard output."<<endl
        <<"    --frequency <value>, -f <value>         generates a sound of a specific frequency value."<< endl
        <<"    --dtmf, -d                              generates a dtmf sound. It's necessary to specify the low and high frequencies, see bellow."<<endl
        <<"    --low <value>, -l <value>               sets the low frequency."<<endl
        <<"    --high <value>, -h <value>              sets the high frequency."<<endl
        <<"    --time <value>, -t <value>              sets the tone duration. Usage only with tone-string"<<endl
        <<"    --silence-length <value>, -z <value>    sets the silence duration."<<endl
        <<"    --tone-string <string>, -e <string>     string with the numbers to generate a dtmf sound."<<endl<<endl;
}


int main(int argc, char** argv)
{
    int nextOp;
    float freq1 = -1, freq2 = -1;
    int dtmf = -1;
    char *fname = NULL;
    int SAMPLE_RATE = 8000;
    int duration = 400;
    int silenceDuration = 100;
    char *toneStr = NULL;
    char *mulFreqs;
    const char *shortOps = "s:o:f:dl:h:t:z:e:p";
    const struct option longOpts[] = {
        {"sample-rate", 1, NULL, 0},
        {"output", 1, NULL, 0},
        {"frequency", 1, NULL, 0},
        {"dtmf", 0, NULL, 0},
        {"low-frequency", 1, NULL, 0},
        {"high-frequency", 1, NULL, 0},
        {"time", 1, NULL, 0},
        {"silence-length", 1, NULL, 0},
        {"tone-string", 1, NULL, 0},
        {"help", 0, NULL, 0},
    };

    do {
        nextOp = getopt(argc, argv, shortOps);

        switch(nextOp) {
            case 'p':
                showHelp();
                return EXIT_SUCCESS;
            case 's':
                SAMPLE_RATE = atoi(optarg);
                break;
            case 'o':
                fname = optarg;
                break;
            case 'f':
                dtmf = 0;
                freq1 = atof(optarg);
                break;
            case 'e':
                dtmf = 1;
                toneStr = optarg;
                break;
            case 'd':
                dtmf = 1;
                break;
            case 'l':
                freq1 = atof(optarg);
                break;
            case 'h':
                freq2 = atof(optarg);
                break;
            case 't':
                duration = atoi(optarg);
                break;
            case 'z':
                silenceDuration = atoi(optarg);
                break;
            //case 'm':
            //    mulFreqs = optarg;
            //    break;
            default:
                break;
        }
    } while(nextOp != -1);
    
    //if(fname == NULL) {
    //    cerr<< "Missing output filename"<< endl;
    //    return EXIT_FAILURE;
    //}
    FILE *file = NULL;
    if(fname != NULL)
        file = fopen(fname, "wb");
    else  {
        file = stdout;
        fname = (char*)"stdout";
    }

    if((!dtmf && freq1 < 0) && (dtmf == 1 && ((freq1 < 0 || freq2 < 0) && toneStr == NULL)) && mulFreqs == NULL) {
        if((!dtmf && freq1 < 0))
            cerr<< "Missing single frequency parameters."<< endl;
        else if(mulFreqs == NULL)
            cerr<< "Missing multiple frequency string."<<endl;
        else 
            cerr<< "Missing dtmf frequency parameters."<< freq1 << ", "<<freq2<< endl;
        return EXIT_FAILURE;
    }
    
    if(duration <= 0 || silenceDuration <= 0) {
        cerr<< "Invalid duration."<< endl;
        return EXIT_FAILURE;
    }

    if(dtmf == 1 && toneStr != NULL) {
        int sizeTone = SAMPLE_RATE * duration / 1000;
        int silenceLength = SAMPLE_RATE * silenceDuration / 1000;
        
        cerr<<"Generating tone for "<< toneStr<< " ("<< duration<< " ms or "<< sizeTone<< " samples)"<< endl<< ":: Writing to file "<< fname<< "..."<< endl;
        short *i = generateTonesFromString(toneStr, SAMPLE_RATE, sizeTone, silenceLength);
        writeWaveToFile(file, i, sizeTone * strlen(toneStr) + silenceLength * (strlen(toneStr) - 1), SAMPLE_RATE, 1, 16);
        delete i;
        return EXIT_SUCCESS;
    } else if(dtmf == 1) {
        // since toneStr is null, all that is left is freq1 and freq2 > 0
        int sizeTone = SAMPLE_RATE * duration / 1000;
        cerr.precision(2);
        cerr<< "Generating dtmf tone low frequency "<< freq1<< " and high frequency "<< freq2<< " ("<< duration<< "ms or "<< sizeTone<< " samples)"<< endl<< ": Writing to file "<<fname<<"..."<<endl;
        short *i = makeToneInt(SAMPLE_RATE, freq1, freq2, sizeTone, 1.0f);
        
        writeWaveToFile(file, i, sizeTone, SAMPLE_RATE, 1, 16);
        delete i;
    } else if(dtmf == 0) {
        int sizeTone = SAMPLE_RATE * duration / 1000;
        //printf("Generating frequency %.2f (%d ms or %d samples)\n:: Writing to file %s...\n", freq1, duration, sizeTone, fname);
        cerr<< "Generating dtmf tone low frequency "<< freq1<< "("<< duration<< "ms or "<< sizeTone<< " samples)"<< endl<< ": Writing to file "<<fname<<"..."<<endl;
        short *i = makeFreqInt(SAMPLE_RATE, freq1, sizeTone, 1.0f);
        writeWaveToFile(file, i, sizeTone, SAMPLE_RATE, 1, 16);
        delete i;
    }
    fclose(file);

    /* else if (mulFreqs != NULL) {
        char **argf;
        int len = csStrLen(mulFreqs);
        if(len) {
            char *c, *strTmp = mulFreqs;
            argf = new char*[len];
            cerr<< "Len is:"<<len <<endl;
            for(int i = 0; i < len; i++) {
                c = strchrnul(strTmp, ',');
                *c='\0';
                argf[i] = strTmp;
                ++c;
                strTmp = c;
            }

            delete[] argf;
        } else {
            cerr<< "Invalid format for multifrequency generation."<< endl;
        }
    }*/
}

