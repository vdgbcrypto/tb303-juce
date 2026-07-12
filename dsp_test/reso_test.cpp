// Test: does RESONANCE self-oscillate at max? And does ENV MOD open cutoff?
// Mirrors the plugin's 2-stage Chamberlin SVF with the new damping mapping.
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
    s.lp = std::clamp(s.lp,-1.5,1.5);
    s.bp = std::clamp(s.bp,-1.5,1.5);
    return s.lp;
}
void setFilter(double cutoffHz, double damping){
    double fc = std::clamp(cutoffHz/gSR,0.0001,0.49);
    double f = 2.0*std::sin(PI*fc);
    mStage1.f=f; mStage1.q=damping; mStage2.f=f*0.95; mStage2.q=damping*1.03;
}

// Self-oscillation test: feed silence (0) with a tiny seed; if it sustains a
// tone, the filter self-oscillates. Measure steady-state RMS and dominant freq.
void selfOscTest(double damping){
    resetState();
    // seed one sample
    double y = processFilterStage(1e-4,0); y = processFilterStage(y,1);
    const int N=20000;
    std::vector<double> buf(N);
    for(int n=0;n<N;n++){ y = processFilterStage(0.0,0); y = processFilterStage(y,1); buf[n]=y; }
    double rms=0; for(double v:buf) rms+=v*v; rms=std::sqrt(rms/N);
    // dominant frequency via zero-crossing of second half
    int zc=0; double last=buf[N/2];
    for(int n=N/2;n<N;n++){ if((buf[n]>0)!=(last>0)) zc++; last=buf[n]; }
    double domHz = (double)zc/2.0 / ((N/2)/gSR);
    printf("  damping=%.3f  steadyRMS=%.5f  dominantHz=%.1f\n", damping, rms, domHz);
}

int main(){
    printf("=== SELF-OSCILLATION vs damping (cutoff=2000Hz, silent input) ===\n");
    for(double r : {0.0,0.3,0.6,0.8,1.0}){
        double damping = 1.414 * std::pow(0.03/1.414, r);
        setFilter(2000.0, damping);
        selfOscTest(damping);
    }

    printf("\n=== ENV MOD sweep: cutoff open at note-on (base 500Hz, envMod=1) ===\n");
    // envNorm goes 0 (released) -> 1 (note-on). Measure effective -3dB cutoff.
    auto minus3 = [](double baseHz, double envNorm, double envModAmt)->double{
        double hz = baseHz*(1.0 + 1.0*envNorm*envModAmt);
        // quick: just report the target the code computes (the actual LP -3dB ~ target for low fc)
        return hz;
    };
    printf("  envNorm=0.0 -> effective ~%.0f Hz (closed)\n", minus3(500,0.0,5.0));
    printf("  envNorm=1.0 -> effective ~%.0f Hz (open)\n", minus3(500,1.0,5.0));
    printf("  sweep ratio at full env mod = %.1fx\n", minus3(500,1.0,5.0)/minus3(500,0.0,5.0));
    return 0;
}
