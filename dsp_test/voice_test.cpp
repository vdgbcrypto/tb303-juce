// Test: new mono voice — portamento glide, envelope-shaped amplitude (no hard
// gate click), last-note priority, velocity->accent. Replicates the DSP math.
#include <cmath>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <cstdlib>

static const double PI = 3.14159265358979323846;
static double gSR = 44100.0;
static double mPhase=0, mEnvLevel=0, mEnvPeak=1, mTargetFreq=110, mPortaFreq=110;
static double mPortaCoeff=0, mAccentAmount=0, mSmoothedAccent=0, mSmoothedDecay=0.3;
static int mNoteStack[8]{}; static int mNoteStackSize=0; static bool mNoteOn=false;
static int mWaveform=0;

void noteOn(int note,double vel){
    if(mNoteStackSize<8) mNoteStack[mNoteStackSize++]=note;
    mTargetFreq = 440.0*pow(2.0,(note-69)/12.0);
    mAccentAmount = std::clamp(mSmoothedAccent*vel,0.0,1.0);
    mNoteOn=true;
}
void noteOff(int note){
    int w=0; for(int r=0;r<mNoteStackSize;++r) if(mNoteStack[r]!=note) mNoteStack[w++]=mNoteStack[r];
    mNoteStackSize=w;
    if(mNoteStackSize>0){ mTargetFreq=440.0*pow(2.0,(mNoteStack[mNoteStackSize-1]-69)/12.0); mNoteOn=true;}
    else mNoteOn=false;
}
double envStep(int gate){
    if(gate){ double t=1+mSmoothedAccent*0.4; double a=1-exp(-1/(0.0025*gSR)); mEnvLevel+=(t-mEnvLevel)*a; mEnvPeak=t;}
    else { double dt=0.05+0.95*mSmoothedDecay; mEnvLevel*=exp(-1/(dt*gSR)); if(mEnvLevel<1e-6)mEnvLevel=0;}
    return mEnvLevel;
}
double drive(double x,double d){ double g=1+d*6; double y=x*g; y=std::clamp(y,-1.0,1.0); return y*(1.5-0.5*y*y);}

// render a sequence of note events, return peak amplitude at note start (to
// check for click = large discontinuity) and confirm legato retrigger behaviour
int main(){
    mPortaCoeff = 1-exp(-1/(0.006*gSR));
    const int N=4410; // 100ms
    std::vector<double> buf(N);
    // Scenario: note A (MIDI 45 ~110Hz) for 30ms, then legato to B (57~220Hz),
    // check portamento glides (no instant jump) and amplitude follows envelope.
    int evT[2]={0, (int)(0.03*gSR)};
    int evN[2]={45,57};
    int ei=0; double prevOut=0; double maxClick=0;
    for(int n=0;n<N;n++){
        if(ei<2 && n==evT[ei]){ noteOn(evN[ei],1.0); ei++; }
        if(ei==2 && n==(int)(0.06*gSR)){ noteOff(45); } // release first, B remains (legato)
        if(ei==2 && n==(int)(0.09*gSR)){ noteOff(57); } // all off
        int gate = mNoteOn?1:0;
        mPortaFreq += (mTargetFreq-mPortaFreq)*mPortaCoeff;
        double freq=mPortaFreq; mPhase += freq*2*PI/gSR; if(mPhase>2*PI)mPhase-=2*PI;
        double saw=mPhase/PI-1, sq=(mPhase<PI)?1:-1;
        double osc=(mWaveform==1)?sq:(mWaveform==2?saw*0.5+sq*0.5:saw);
        double env=envStep(gate);
        double out=drive(osc*env*1.2, mAccentAmount*0.5)*0.5*0.8; // vol 0.5
        if(n>5){ double d=fabs(out-prevOut); if(d>maxClick)maxClick=d; }
        prevOut=out; buf[n]=out;
    }
    printf("max per-sample amplitude jump (click proxy) = %.5f  (low = no click)\n", maxClick);
    // report amplitude envelope shape: peak after attack, value at release
    printf("envPeak=%.3f  finalEnv=%.4f\n", mEnvPeak, mEnvLevel);
    return 0;
}
