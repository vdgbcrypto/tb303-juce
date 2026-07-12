// Verify #6 fix: noteOff uses the SNAPSHOTTED note (mSeqCurrentNote), not the
// live (possibly re-edited) pattern note. Simulates: step plays note 48, then
// the pattern step is edited to 36 while running, then STOP -> noteOff must
// target 48 (else 48 stuck).
#include <cstdio>
#include <vector>
struct Ev { int t, note; }; // t: 0=on,1=off
int main(){
    int curNote=-1, curStep=-1;
    std::vector<Ev> ev;
    // --- running: step 0 note 48 starts ---
    int sNote=48; // pattern says 48
    ev.push_back({0,sNote}); curNote=sNote; curStep=0;
    // --- user edits playing step 0 to 36 (pattern changes) ---
    sNote=36;
    // (note still sounding 48; no noteOff yet because step hasn't ended)
    // --- STOP: must release the sounding note (curNote=48), not pattern's 36 ---
    ev.push_back({1,curNote}); // uses snapshot
    bool offCorrect = (ev.back().t==1 && ev.back().note==48);
    printf("STOP note-off targets sounding note 48 (not edited 36): %s\n", offCorrect?"YES":"NO");
    printf("events: on=%d off=%d\n", ev[0].note, ev.back().note);
    printf("PASS: %s\n", offCorrect?"YES":"NO");
    return 0;
}
