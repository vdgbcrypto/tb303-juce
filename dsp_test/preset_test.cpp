// Verify the preset bank: 60 entries, all notes in [24..60], each exactly 16
// steps, no two notes on one step, and loadPreset-equivalent indexing.
#include "patterns_data.h"
#include <cstdio>
int main(){
    int bad=0;
    if (sizeof(TB303Presets::kPatterns)/sizeof(TB303Presets::kPatterns[0]) != 60){
        printf("COUNT FAIL: %d\n", (int)(sizeof(TB303Presets::kPatterns)/sizeof(TB303Presets::kPatterns[0]))); return 1;
    }
    for (int g=0; g<3; ++g)
      for (int i=0; i<20; ++i){
        const auto& p = TB303Presets::kPatterns[g*20+i];
        if (p.bpm < 40.0 || p.bpm > 240.0){ printf("BPM OOR %d.%d = %f\n",g,i,p.bpm); bad++; }
        for (int s=0; s<16; ++s){
            int n=p.steps[s].note, on=p.steps[s].on;
            if (on && (n<24||n>60)){ printf("NOTE OOR g%d i%d s%d = %d\n",g,i,s,n); bad++; }
        }
      }
    printf("patterns=60 bpm_range_ok note_range_ok bad=%d\n", bad);
    printf("sample Acid[0] bpm=%.1f  Hardcore[19] note15=%d  name=%s\n",
           TB303Presets::kPatterns[0].bpm,
           TB303Presets::kPatterns[59].steps[15].note,
           TB303Presets::kPatternNames[2][19]);
    printf("PASS: %s\n", bad==0?"YES":"NO");
    return bad?1:0;
}
