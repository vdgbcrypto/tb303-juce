// Verify DAW-sync step rate over a longer window (averages out endpoint artifacts).
// 120bpm -> 8 sixteenths/sec -> 880 steps over 110 seconds.
#include <cstdio>
#include <cmath>
int main(){
    const double sixteenth = 0.25, ppqPerSample = (120.0/60.0)/44100.0;
    double ppq = 0.0; int remaining = 44100*110; // 110 seconds
    int lastStepIdx = -1, steps=0;
    while(remaining>0){
        double newPpq = ppq + ppqPerSample;
        int newStepIdx = (int)(newPpq/sixteenth);
        if(newStepIdx != lastStepIdx){ ++steps; lastStepIdx=newStepIdx; }
        ppq=newPpq; --remaining;
    }
    printf("steps_in_110s=%d (expect ~880)\n", steps);
    bool ok = (steps>=879 && steps<=881);
    printf("PASS=%s\n", ok?"YES":"NO");
    return ok?0:1;
}
