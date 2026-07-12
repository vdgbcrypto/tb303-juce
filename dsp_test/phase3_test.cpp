// Verify Phase 3 logic numerically (mirrors processBlock):
// (1) Swing: odd 16th longer, even shorter; a pair spans 2 base steps (avg tempo OK).
// (2) Host MIDI clock: 24 ppq -> 6 ticks per 16th; steps advance on every 6th tick.
#include <cstdio>
#include <vector>
int main(){
    double sr=44100, bpm=120.0;
    double base=(60.0/bpm)/4.0*sr; // 16th = 5512.5 samples
    double swing=0.5;
    double even=base*(1.0-swing), odd=base*(1.0+swing);
    printf("base=%.1f even=%.1f odd=%.1f pair=%.1f (2*base=%.1f)\n",base,even,odd,even+odd,2*base);
    bool swingOK = (even+odd) > (2*base - 1.0) && (even+odd) < (2*base + 1.0) && odd>even;

    // MIDI clock sim: feed 24 ticks/quarter. At 120bpm quarter=0.5s -> tick every 22050 samples.
    // 6 ticks per 16th -> step every 132300 samples (== 3 quarters? no: 16th=5512.5*? ) check count.
    std::vector<int> ticks;
    int tickPeriod=(int)(0.5*sr/24.0); // quarter/24
    for(int p=0; p<tickPeriod*30; p+=tickPeriod) ticks.push_back(p); // 30 ticks
    int tickInStep=0, steps=0; bool running=true;
    for(int t: ticks){ if(!running)break; if(++tickInStep>=6){tickInStep=0;++steps;} }
    printf("ticks=%d steps_advanced=%d (expect 5)\n",(int)ticks.size(),steps);
    bool clockOK = (steps==5);

    printf("SWING_OK=%s CLOCK_OK=%s PASS=%s\n", swingOK?"YES":"NO", clockOK?"YES":"NO", (swingOK&&clockOK)?"YES":"NO");
    return (swingOK&&clockOK)?0:1;
}
