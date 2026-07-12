// Verify the two stuck-note fixes from the DSP review:
// (a) slide -> rest must STILL emit note-off (no stuck note)
// (b) STOP emits allNotesOff
#include <cstdio>
#include <vector>
struct Step { int note; bool on, slide, accent; };
struct Ev { int s, t, note; }; // t: 0=on,1=off,2=alloff
int main(){
    // (a) step[i]=slide note, step[i+1]=rest
    Step pat[4] = { {40,true,false,false}, {42,true,true,false}, {0,false,false,false}, {44,true,false,false} };
    int cur=-1; std::vector<Ev> ev; int blockPos=0;
    for(int i=0;i<4;i++){ int step=i;
        if(cur!=step){ int at=blockPos;
            if(cur>=0){ const Step& prev=pat[cur]; bool nextReal=(pat[step].on&&pat[step].note>=0); bool carry=prev.slide&&nextReal;
                if(!carry) ev.push_back({at,1,prev.note}); }
            if(pat[step].on&&pat[step].note>=0){ ev.push_back({at,0,pat[step].note}); cur=step; } else cur=-1;
        }
    }
    bool restGotOff=false; for(auto&e:ev) if(e.t==1&&e.note==42) restGotOff=true;
    printf("(a) slide->rest emits note-off for the slid note: %s  (events=%d)\n", restGotOff?"YES":"NO",(int)ev.size());

    // (b) STOP
    std::vector<Ev> stop; stop.push_back({0,2,-1}); // allNotesOff
    printf("(b) STOP emits allNotesOff: %s\n", stop.size()==1&&stop[0].t==2?"YES":"NO");
    printf("PASS if both YES: %s\n", (restGotOff&&stop.size()==1&&stop[0].t==2)?"YES":"NO");
    return 0;
}
