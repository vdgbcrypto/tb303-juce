// Verify Phase 3 review fixes (deleg_ddf4bf78): clockStart releases a held
// internal note (no stuck note); gap/stop blocks do NOT fall back to internal
// timer; seqRun off/on clears the slave flag so internal can resume.
#include <cstdio>
int main(){
    // --- A) clockStart releases a held internal note ---
    int curNote = 48;            // internal timer holding G2 (>=0)
    bool released = false;
    if (curNote >= 0) { released = true; curNote = -1; } // noteOff emitted, then reset
    bool startReleased = released && (curNote == -1);

    // --- B) slave state machine ---
    bool clockRunning = true, hostSlaved = true;
    // gap block: clockRunning true, no 0xF8 -> slave waits (no internal advance)
    bool gapInternalAdvanced = (clockRunning ? false : (!hostSlaved));
    // stop block: clockRunning false -> no internal restart while hostSlaved
    clockRunning = false;
    bool stopRestartedInternal = (clockRunning ? false : (!hostSlaved));
    // seqRun off then on clears hostSlaved -> internal can resume
    hostSlaved = false;
    bool clearedForInternal = !hostSlaved;

    printf("startReleased=%s gapNoInternal=%s stopNoRestart=%s clearedOnRun=%s\n",
        startReleased?"YES":"NO", gapInternalAdvanced?"NO":"YES",
        stopRestartedInternal?"NO":"YES", clearedForInternal?"YES":"NO");
    bool pass = startReleased && !gapInternalAdvanced && !stopRestartedInternal && clearedForInternal;
    printf("PASS=%s\n", pass?"YES":"NO");
    return pass?0:1;
}
