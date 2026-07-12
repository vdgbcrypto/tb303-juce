// Standalone numerical test of the tb303-juce DSP core (no JUCE).
// Faithfully replicates processBlock math to verify volume + cutoff behaviour.
#include <cmath>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <cstdlib>

static const double PI = 3.14159265358979323846;

struct SVF { double lp=0.0, bp=0.0, hp=0.0, f=0.0, q=0.707; };
static double gSR=44100.0;
static double mSmoothedCutoff=0.5, mSmoothedReso=0.0, mSmoothedEnvMod=0.5,
              mSmoothedDecay=0.3, mSmoothedAccent=0.0, mSmoothedVolume=0.5, mSmoothedTune=0.5;
static double mEnvLevel=0.0;
static SVF mStage1, mStage2;
static double mPhase=0.0;
static double mParamSmoothCoef=0.0;
static bool mNoteOn=false;
static double noteFrequency=220.0;
static float activeLevel=0.0f;

void updateSmoothing(double sr){ mParamSmoothCoef = 1.0 - std::exp(-1.0/(0.01*sr)); }

void updateFilterCoefficients(){
    double cutoff = 20.0 * std::pow(10.0, mSmoothedCutoff*4.0);
    double reso = mSmoothedReso * 3.5;
    double q = 0.05 + 0.707 + reso*(1.0 - 0.707 - 0.08*(cutoff/gSR));
    q = std::max(q, 0.05);
    double fc = cutoff/(gSR*2.0);
    if (fc > 0.49) fc = 0.49;
    mStage1.f = fc; mStage1.q = q;
    mStage2.f = fc*0.95; mStage2.q = q*1.03;
}
double processFilterStage(double x, int stage){
    auto& s = (stage==0)?mStage1:mStage2;
    s.lp += s.f*s.bp;
    s.hp = x - s.lp - s.q*s.bp;
    s.bp += s.f*s.hp;
    s.lp = std::clamp(s.lp,-1.5,1.5);
    s.bp = std::clamp(s.bp,-1.5,1.5);
    return s.lp;
}
double envelopeStep(int gate){
    if (gate){ mEnvLevel = 1.0 + mSmoothedAccent*0.4; }
    else { double dt=0.05+(1.0-0.05)*mSmoothedDecay; double c=std::exp(-1.0/(dt*gSR)); mEnvLevel*=c; if(mEnvLevel<1e-6) mEnvLevel=0.0; }
    return mEnvLevel;
}
double driveSignal(double x, double drive){
    double gain = 1.0 + drive*6.0;
    double y = x*gain;
    if (y>1.0) y=1.0; if (y<-1.0) y=-1.0;
    return y*(1.5 - 0.5*y*y);
}

double renderSample(double tCut,double tRes,double tEnv,double tDec,double tAcc,double tVol,double tTune){
    mSmoothedCutoff += (tCut-mSmoothedCutoff)*mParamSmoothCoef;
    mSmoothedReso   += (tRes-mSmoothedReso)*mParamSmoothCoef;
    mSmoothedEnvMod += (tEnv-mSmoothedEnvMod)*mParamSmoothCoef;
    mSmoothedDecay  += (tDec-mSmoothedDecay)*mParamSmoothCoef;
    mSmoothedAccent += (tAcc-mSmoothedAccent)*mParamSmoothCoef;
    mSmoothedVolume += (tVol-mSmoothedVolume)*mParamSmoothCoef;
    mSmoothedTune   += (tTune-mSmoothedTune)*mParamSmoothCoef;
    updateFilterCoefficients();
    const int gate = mNoteOn?1:0;
    double freq = noteFrequency * std::pow(2.0, mSmoothedTune-0.5);
    mPhase += (freq*2.0*PI)/gSR;
    if (mPhase > 2.0*PI) mPhase -= 2.0*PI;
    double saw = (mPhase/PI)-1.0;
    double sq  = (mPhase<PI)?0.7:-0.7;
    double osc = (saw*0.6 + sq*0.4)*activeLevel;
    double filtered = processFilterStage(osc,0);
    filtered = processFilterStage(filtered,1);
    double env = envelopeStep(gate);
    double accented = env + mSmoothedAccent*0.3;
    double driven = driveSignal(filtered*accented*1.2, mSmoothedAccent*0.5);
    return driven * mSmoothedVolume * 0.8;
}

static double rms(const std::vector<double>& v){
    double s=0; for(double x:v) s+=x*x; return std::sqrt(s/v.size());
}

int main(){
    updateSmoothing(gSR);
    // turn note on
    mNoteOn=true; activeLevel=1.0f; mEnvLevel=1.0;

    // ---- TEST A: VOLUME sweep (cutoff fixed at 0.5) ----
    printf("=== TEST A: VOLUME sweep (target volume -> steady RMS) ===\n");
    for(double v : {0.0,0.3,0.5,0.8,1.0}){
        for(int i=0;i<8000;i++) renderSample(0.5,0.0,0.5,0.3,0.0,v,0.5); // settle
        std::vector<double> buf(4096);
        for(int i=0;i<4096;i++) buf[i]=renderSample(0.5,0.0,0.5,0.3,0.0,v,0.5);
        printf("  volume=%.2f  RMS=%.5f\n", v, rms(buf));
    }

    // ---- TEST B: CUTOFF sweep (volume fixed 0.8) ----
    printf("=== TEST B: CUTOFF sweep (target cutoff -> steady RMS, intended Hz) ===\n");
    for(double c : {0.1,0.3,0.5,0.7,0.9}){
        for(int i=0;i<8000;i++) renderSample(c,0.0,0.5,0.3,0.0,0.8,0.5);
        std::vector<double> buf(4096);
        for(int i=0;i<4096;i++) buf[i]=renderSample(c,0.0,0.5,0.3,0.0,0.8,0.5);
        double intendedHz = 20.0*std::pow(10.0,c*4.0);
        printf("  cutoff=%.2f  intendedHz=%-9.1f  RMS=%.5f\n", c, intendedHz, rms(buf));
    }

    // ---- TEST C: CUTOFF step response (lag in samples/ms) ----
    printf("=== TEST C: CUTOFF step 0.1 -> 0.9, settle time ===\n");
    for(int i=0;i<12000;i++) renderSample(0.1,0.0,0.5,0.3,0.0,0.8,0.5); // settle low
    std::vector<double> low(4096);
    for(int i=0;i<4096;i++) low[i]=renderSample(0.1,0.0,0.5,0.3,0.0,0.8,0.5);
    double rmsLow = rms(low);
    for(int i=0;i<12000;i++) renderSample(0.9,0.0,0.5,0.3,0.0,0.8,0.5); // settle high
    std::vector<double> high(4096);
    for(int i=0;i<4096;i++) high[i]=renderSample(0.9,0.0,0.5,0.3,0.0,0.8,0.5);
    double rmsHigh = rms(high);
    printf("  rmsLow=%.5f  rmsHigh=%.5f\n", rmsLow, rmsHigh);
    // now jump and find 95% of the way from low to high
    double target = rmsLow + 0.95*(rmsHigh-rmsLow);
    int settle=-1;
    std::vector<double> win(1024);
    for(int i=0;i<20000;i++){
        double s = renderSample(0.9,0.0,0.5,0.3,0.0,0.8,0.5);
        for(size_t k=0;k<win.size()-1;k++) win[k]=win[k+1];
        win.back()=s;
        if(i>=1023){
            double r=rms(win);
            if(r>=target){ settle=i; break; }
        }
    }
    if(settle>=0) printf("  -> reaches 95%% of final level after %d samples (~%.1f ms @44.1k)\n", settle, settle/44.1);
    else printf("  -> did not reach 95%% within window\n");
    return 0;
}
