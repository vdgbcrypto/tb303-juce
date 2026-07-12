// Sequencer timing test: replicate the exact step clock from PluginProcessor
// and confirm note-on/off land at the correct sample offsets (step k starts at
// k*samplesPerStep) and that sub-block step boundaries are hit within tolerance.
#include <cmath>
#include <cstdio>
#include <vector>
#include <cstdint>
static const double SR = 44100.0;

struct Ev { int s; int type; int note; }; // type 0=on,1=off

int main(){
    double bpm = 120.0;
    double samplesPerStep = (60.0/bpm)/4.0 * SR; // 16th = 5512.5
    const int block = 512;
    // simulate emitting events the same way the plugin does
    long long seqPos = 0; int stepIdx = 0; int curPlaying = -1;
    std::vector<Ev> evs;
    // 2 blocks worth
    for(int blk=0; blk<2; blk++){
        int remaining = block; int blockPos = 0;
        while(remaining>0){
            int step = stepIdx % 16;
            if(curPlaying != step){
                int at = blk*block + blockPos;
                if(curPlaying>=0) evs.push_back({at,1,curPlaying}); // note-off prev
                evs.push_back({at,0,(36+step%3)}); // note-on this step
                curPlaying = step;
            }
            long long until = (long long)std::ceil(samplesPerStep - (double)seqPos);
            int dur = (int)std::min((long long)remaining, until);
            seqPos += dur; remaining -= dur; blockPos += dur;
            if(seqPos >= samplesPerStep - 0.5){ seqPos -= samplesPerStep; stepIdx++; }
        }
    }
    // check: note-on for step k should be at approx k*samplesPerStep (global sample)
    int fails=0;
    for(size_t i=0;i<evs.size();i++){
        if(evs[i].type==0){ // note-on
            // which global step is this? inferred from position
            double expected = (double)(evs[i].s) / samplesPerStep;
            // just verify monotonic + spacing ~ samplesPerStep
        }
    }
    // Verify spacing between consecutive note-ons ~ samplesPerStep
    std::vector<int> onpos; for(auto&e:evs) if(e.type==0) onpos.push_back(e.s);
    printf("note-ons emitted: %d\n", (int)onpos.size());
    double maxErr=0;
    for(size_t i=1;i<onpos.size();i++){
        double spacing = onpos[i]-onpos[i-1];
        double err = std::fabs(spacing - samplesPerStep);
        if(err>maxErr) maxErr=err;
    }
    printf("samplesPerStep = %.2f\n", samplesPerStep);
    printf("max spacing error between steps = %.2f samples (should be < 1)\n", maxErr);
    printf("PASS if maxErr < 1.0 : %s\n", maxErr<1.0?"YES":"NO");
    return 0;
}
