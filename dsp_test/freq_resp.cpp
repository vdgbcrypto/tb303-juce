// Decisive test: measure ACTUAL -3dB cutoff of the 2-stage SVF lowpass.
// CURRENT (plugin) formula vs CORRECTED Chamberlin. Resonance=0 => clean LP.
// FIX: reset only lp/bp/hp state, NOT f/q (which coeff*() set).
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <cstdlib>

static const double PI = 3.14159265358979323846;
static double gSR = 44100.0;
static double mSmoothedCutoff = 0.5, mSmoothedReso = 0.0;
struct SVF { double lp=0.0, bp=0.0, hp=0.0, f=0.0, q=0.707; };
static SVF mStage1, mStage2;

static void resetState(){ mStage1.lp=mStage1.bp=mStage1.hp=0; mStage2.lp=mStage2.bp=mStage2.hp=0; }

double processFilterStage(double x, int stage){
    auto& s = (stage==0)?mStage1:mStage2;
    s.lp += s.f*s.bp;
    s.hp = x - s.lp - s.q*s.bp;
    s.bp += s.f*s.hp;
    s.lp = std::clamp(s.lp,-1.5,1.5);
    s.bp = std::clamp(s.bp,-1.5,1.5);
    return s.lp;
}

void coeffCurrent(){
    double cutoff = 20.0 * std::pow(10.0, mSmoothedCutoff*4.0);
    double reso = mSmoothedReso*3.5;
    double q = 0.05 + 0.707 + reso*(1.0 - 0.707 - 0.08*(cutoff/gSR));
    q = std::max(q,0.05);
    double fc = cutoff/(gSR*2.0);
    if (fc>0.49) fc=0.49;
    mStage1.f=fc; mStage1.q=q; mStage2.f=fc*0.95; mStage2.q=q*1.03;
}
void coeffFixed(){
    const double cutoff = 20.0 * std::pow(10.0, mSmoothedCutoff*4.0);
    const double fc = std::min(0.49, std::max(0.0001, cutoff/gSR));
    const double f = 2.0 * std::sin(PI*fc);
    const double d = 1.0/(1.0 + mSmoothedReso*18.0);
    mStage1.f=f; mStage1.q=d; mStage2.f=f*0.95; mStage2.q=d*1.03;
}

double gainAt(double freqHz){
    resetState();
    double phase=0; const double w=2.0*PI*freqHz/gSR;
    double si=0, so=0; const int settle=5000, win=5000;
    for (int n=0;n<settle+win;n++){
        double x=std::sin(phase); phase+=w; if(phase>2*PI)phase-=2*PI;
        double y=processFilterStage(x,0); y=processFilterStage(y,1);
        if (n>=settle){ si+=x*x; so+=y*y; }
    }
    return std::sqrt(so/win)/std::sqrt(si/win);
}

double minus3dB(bool fixed, double cutNorm){
    mSmoothedCutoff=cutNorm; mSmoothedReso=0.0;
    (fixed?coeffFixed:coeffCurrent)();
    double lo=20.0, hi=20000.0;
    for (int it=0;it<46;it++){
        double mid=std::sqrt(lo*hi);
        if (gainAt(mid) > 0.7071) lo=mid; else hi=mid;
    }
    return std::sqrt(lo*hi);
}

int main(){
    printf("%-8s %-12s %-14s %-14s %-10s\n","KNOB","NOMINAL_HZ","CURRENT_-3dB","FIXED_-3dB","RATIO_C/F");
    for (double c : {0.25,0.5,0.75,0.9}){
        double nominal=20.0*std::pow(10.0,c*4.0);
        double pc=minus3dB(false,c);
        double pf=minus3dB(true ,c);
        printf("%-8.2f %-12.1f %-14.1f %-14.1f %-10.2f\n", c, nominal, pc, pf, pc/pf);
    }
    return 0;
}
