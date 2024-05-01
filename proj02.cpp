#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

#define SQR(x) ((x)*(x))
#define M_PI 3.14159265358979323846

// Global variables
int NowYear;            // 2024- 2029
int NowMonth;           // 0 - 11

float NowPrecip;        // inches of rain per month
float NowTemp;          // temperature this month
float NowHeight;        // grain height in inches
int NowNumDeer;         // number of deer in the current population
int NowNumTribe;       

const float GRAIN_GROWS_PER_MONTH = 12.0;
const float ONE_DEER_EATS_PER_MONTH = 1.0;

const float TRIBE_FARMING_PER_MONTH = 5.0;
const float TRIBE_DEER_LOSS_PERCENTAGE = 0.15;


const float AVG_PRECIP_PER_MONTH = 7.0;    // average
const float AMP_PRECIP_PER_MONTH = 6.0;    // plus or minus
const float RANDOM_PRECIP = 2.0;    // plus or minus noise

const float AVG_TEMP = 60.0;   // average
const float AMP_TEMP = 20.0;   // plus or minus
const float RANDOM_TEMP = 10.0;   // plus or minus noise

const float MIDTEMP = 40.0;
const float MIDPRECIP = 10.0;

unsigned int seed = 0;

omp_lock_t  Lock;
volatile int    NumInThreadTeam;
volatile int    NumAtBarrier;
volatile int    NumGone;

// Function prototypes
void InitBarrier(int);
void WaitBarrier();

// Function to initialize barrier
void InitBarrier(int n) {
    NumInThreadTeam = n;
    NumAtBarrier = 0;
    omp_init_lock(&Lock);
}

// Function for barrier synchronization
void WaitBarrier() {
    omp_set_lock(&Lock);
    {
        NumAtBarrier++;
        if (NumAtBarrier == NumInThreadTeam) {
            NumGone = 0;
            NumAtBarrier = 0;
            while (NumGone != NumInThreadTeam - 1);
            omp_unset_lock(&Lock);
            return;
        }
    }
    omp_unset_lock(&Lock);
    while (NumAtBarrier != 0);
#pragma omp atomic
    NumGone++;
}

// Function to generate random float
float Ranf(float low, float high) {
    float r = (float)rand();    // 0 - RAND_MAX
    float t = r / (float)RAND_MAX; // 0. - 1.

    return low + t * (high - low);
}

// Function for Grain thread
void Grain() {
    while (NowYear < 2030) {
        float tempFactor = exp(-SQR((NowTemp - MIDTEMP) / 10.0));
        float precipFactor = exp(-SQR((NowPrecip - MIDPRECIP) / 10.0));

        float nextHeight = NowHeight;
        nextHeight += tempFactor * precipFactor * GRAIN_GROWS_PER_MONTH;
        nextHeight -= (float)NowNumDeer * ONE_DEER_EATS_PER_MONTH;
        if (nextHeight < 0.0) {
            nextHeight = 0.0;
        }

        if (NowMonth <= 8) {
            nextHeight += TRIBE_FARMING_PER_MONTH;
            if (nextHeight < 0.0) {
                nextHeight = 0.0;
            }
        }

        WaitBarrier();
        NowHeight = nextHeight;
        WaitBarrier();
        WaitBarrier();
        WaitBarrier();
    }
}

// Function for Deer thread
void Deer() {
    while (NowYear < 2030) {
        int nextNumDeer = NowNumDeer;
        int carryingCapacity = (int)(NowHeight);
        if (nextNumDeer < carryingCapacity) {
            nextNumDeer++;
        }
        else if (nextNumDeer > carryingCapacity) {
            nextNumDeer--;
        }
        if (nextNumDeer < 0) {
            nextNumDeer = 0;
        }

        if (NowMonth <= 8) {
            nextNumDeer -= nextNumDeer * TRIBE_DEER_LOSS_PERCENTAGE;
            if (nextNumDeer < 0) {
                nextNumDeer = 0;
            }
        }

        WaitBarrier();
        NowNumDeer = nextNumDeer;
        WaitBarrier();
        WaitBarrier();
        WaitBarrier();
    }
}

// Function for Watcher thread
void Watcher() {
    while (NowYear < 2030) {
        WaitBarrier();
        WaitBarrier();
        WaitBarrier();
        printf("%d, %d, %.2f, %.2f, %.2f, %d, %d\n",
            NowYear, NowMonth, NowTemp, NowPrecip, NowHeight, NowNumDeer, NowNumTribe);
        NowMonth++;
        if (NowMonth == 12) {
            NowMonth = 0;
            NowYear++;
        }

        float ang = (30.0 * (float)NowMonth + 15.0) * (M_PI / 180.0);
        float temp = AVG_TEMP - AMP_TEMP * cos(ang);
        NowTemp = temp + Ranf(-RANDOM_TEMP, RANDOM_TEMP);

        float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin(ang);
        NowPrecip = precip + Ranf(-RANDOM_PRECIP, RANDOM_PRECIP);
        if (NowPrecip < 0.0) {
            NowPrecip = 0.0;
        }

        WaitBarrier();
    }
}

void NomadTribe() {
    while (NowYear < 2030) {
        if (NowMonth <= 8) {
            //if (NowMonth >= 5 && NowMonth <= 10) {
            // Reduce grain and deer population
            int NextNumTribe = NowNumTribe;
            int availableResources = (int)(NowHeight)+(NowNumDeer);

            // Calculate the ratio of available resources to current tribe population
            float resourceRatio = (float)availableResources / (float)NowNumTribe;

            // Adjust the tribe population based on the resource ratio
            if (resourceRatio > 1.0) {
                // Increase the tribe population
                NextNumTribe += (int)(NowNumTribe * 0.1) + 4; // Increase tribe population by 10%
            }
            else {
                // Decrease the tribe population
                NextNumTribe -= (int)(NowNumTribe * 0.5) - 8; // Decrease tribe population by 10%
                if (NextNumTribe < 0) {
                    NextNumTribe = 0; // Ensure tribe population does not go below 0
                }

            }
            WaitBarrier();
            NowNumTribe = NextNumTribe;
            WaitBarrier();
            WaitBarrier();
            WaitBarrier();
        }
        else {
            WaitBarrier();
            WaitBarrier();
            WaitBarrier();
            WaitBarrier();
        }
    }
}

// Main function
int main() {
    omp_set_num_threads(4);
    InitBarrier(4);

    NowMonth = 0;
    NowYear = 2024;
    NowNumDeer = 2;
    NowHeight = 5.0;

#pragma omp parallel sections
    {
#pragma omp section
        {
            Deer();
        }
#pragma omp section
        {
            Grain();
        }
#pragma omp section
        {
            Watcher();
        }
#pragma omp section
        {
            NomadTribe();
        }
    }

    return 0;
}
