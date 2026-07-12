// Final check: does the PLUGIN's resonance mapping self-oscillate at max?
// Uses the exact mapping + clamp from PluginProcessor.cpp.
#include <cmath>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <cstdlib>

static const double PI = 3.14159265358979323846;
static double gSR = 44100.0;
struct SVF { double lp=0.0, bp=0.0, hp=0.0, f=0.0, q=0.707; };
static SVF mStage1, mStage2;
static void resetState(){ mStage1.lp=mStage1.bp=mStage1.hp=0; mStage2.lp=mStage2.bp=mStage2.hp=0; }

double processFilterStage(double x, int stage){
    auto& s = (stage==0)?mStage1:mStage2;
    s.lp += s.f*s.bp;
    s.hp = x - s.lp - s.q*s.bp;
    s.bp += s.f*s.hp;
    s.lp = std::clamp(s.lp,-6.0,6.0);
    s.bp = std::clamp(s.bp,-6.0,6.0);
    return s.lp;
}
void setFilter(double cutoffHz, double damping){
    double fc = std::clamp(cutoffHz/gSR,0.0001,0.49);
    double f = 2.0*std::sin(PI*fc);
    mStage1.f=f; mStage1.q=damping; mStage2.f=f; mStage2.q=damping;
}
// plugin mapping: reso -> damping
double dampingFromReso(double r){ return 0.5*(1.0 - r); }

void selfOscTest(double reso){
    double d = dampingFromReso(reso);
    resetState();
    setFilter(2000.0, d);
    double y = processFilterStage(1e-4,0); y = processFilterStage(y,1);
    const int N=20000; std::vector<double> buf(N);
    for(int n=0;n<N;n++){ y = processFilterStage(0.0,0); y = processFilterStage(y,1); buf[n]=y; }
    double rms=0; for(double v:buf) rms+=v*v; rms=std::sqrt(rms/N);
    int zc=0; double last=buf[N/2];
    for(int n=N/2;n<N;n++){ if((buf[n]>0)!=(last>0)) zc++; last=buf[n]; }
    double domHz = (double)zc/2.0 / ((N/2)/gSR);
    printf("  reso=%.2f  damping=%.3f  steadyRMS=%.5f  domHz=%.0f  %s\n", reso, d, rms, domHz, rms>0.01?"<-- SELF-OSC":"");
}

int main(){
    printf("=== PLUGIN resonance mapping self-osc (cutoff=2kHz) ===\n");
    for(double r : {0.0,0.5,0.8,0.95,1.0}) selfOscTest(r);
    return 0;
}
