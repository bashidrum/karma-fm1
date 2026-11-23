/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
static uint32_t into_s(const track_t *t)          /* samples into the step the clock is in */
{
    uint32_t into, len;
    trk_grid(t, &into, &len);
    return into / (uint32_t)song.g[G_BPM];
}
int main(int argc,char **argv)
{
    uint32_t i,j,k,v,total=0; uint64_t energy[DRUM_KITS]={0};
    int32_t out[CTL*2];
    host_tracks_init();
    dpat_clear(&dpat);
    for(i=0;i<DRUM_LANES;i++)dt_set(3,i,1u+i*8u);                 /* (PHEBUS wave 2) all 16 sounds' trigs on one step */
    assert(dt_mask(3)==0xFFFFu && dt_mask(2)==0u);
    for(i=0;i<DRUM_LANES;i++)assert(dt_has(3,i) && dt_vel(3,i)==1u+i*8u);
    dt_clr(3,5);assert(!dt_has(3,5) && dt_has(3,4) && dt_has(3,6) && dt_vel(3,6)==49u);
    dt_set(3,7,0);assert(dt_vel(3,7)==1u);dt_set(3,7,200);assert(dt_vel(3,7)==127u);   /* (a trig: 1..127) */
    dpat_clear(&dpat);
    for(i=0;i<27u;i++)assert(lane_of_key(i)<DRUM_LANES);
    /* (the drum grid, Ben 23 Nov 21:14) key 1 (F3) HAT, its black key the same; key 4 KICK; key 16 (G5) SNARE */
    assert(lane_of_key(0)==4 && lane_of_key(1)==4 && lane_of_key(6)==0 && lane_of_key(26)==2 && key_of_lane(2)==26 && key_of_white(15)==26);
    for(i=0;i<DRUM_LANES;i++)assert(lane_of_key(key_of_lane(i))==i && lane_of_white(white_of_lane(i))==i);
    for(i=35;i<=81;i++)assert(lane_of_note(i)<DRUM_LANES);
    for(i=0;i<DRUM_LANES;i++)assert(lane_of_note(LANE_NOTE[i])==i);
    /* Render the same hits through all five kits; each must be distinct,
     * finite and silent after its one-shots have finished. */
    for(i=0;i<DRUM_USR;i++)   /* (the built-in kits: HEX and synthesised) */ {
        memset(&drums,0,sizeof drums);
        kit_set((uint32_t)((int16_t)i));
        drum_on(36,110);drum_on(38,100);drum_on(46,80);
        for(j=0;j<FS*2u/CTL;j++) {
            int32_t l[CTL]={0},r[CTL]={0},rev[CTL]={0};
            drums_render(l,r,rev,rev,CTL);
            for(k=0;k<CTL;k++){assert(l[k]>-131072 && l[k]<131072);energy[i]+=l[k]<0?-l[k]:l[k];}
        }
        assert(energy[i]>10000);
        for(k=0;k<NDRUM;k++)assert(!drums.v[k].active);
        for(k=0;k<i;k++)assert(energy[k]!=energy[i]);
    }
    /* (PHEBUS wave 1) the user sample kits are cut: their five numbers are no kit. A project on one plays the power-on
     * kit; PRESETS / KIT step over them (MY HEX 1 comes straight after the last synthesised kit) */
    for(i=DRUM_USR;i<DRUM_SYN;i++){kit_set((uint32_t)((int16_t)i));assert(drum_kit()==DRUM_DEFAULT_KIT);}
    assert(drum_kit_step(DRUM_USR-1,1)==(int16_t)DRUM_SYN && drum_kit_step(DRUM_SYN,-1)==(int16_t)(DRUM_USR-1));
    assert(drum_kit_step(DRUM_KITS-1,5)==(int16_t)(DRUM_KITS-1) && drum_kit_step(0,-3)==0 && drum_kit_step(3,2)==5);
    kit_set((uint32_t)((int16_t)DRUM_DEFAULT_KIT));
    /* LIVE metronome (seq.c click_tick): 120 BPM, 2 s = 4 beats; REC mode clicks only while a
     * track records, ON always while playing, OFF never; the first beat of the bar is louder */
    for(v=0;v<4;v++) {
        uint32_t hits=0,loud=0,age0;
        host_tracks_init();memset(&drums,0,sizeof drums);
        song.g[G_BPM]=120;song.g[G_CLOCK]=v==3?0:v==2?2:1;song.rec=v==0?1u:0u;rec_wait=0;   /* OFF / ON / REC */
        transport_req=1;age0=drums.age;
        for(j=0;j<(FS*2u-FS/4u)/CTL;j++) {               /* up to just before beat 4 */
            uint32_t c=click_n;                          /* (2.4.1: the click is its own voice, drums.c click_on) */
            mix_block(out,CTL);
            if(click_n!=c){hits++;loud+=click_acc;}
        }
        transport_req=2;events_block(CTL);
        if(v==0||v==2)assert(hits==4 && loud==1);
        else assert(hits==0 && drums.age==age0);
    }
    /* LIVE record start. Stopped and armed, a project with notes: the first note starts the
     * loop and is step 1 */
    {
        uint32_t period, b, k0 = 7u, half, c;            /* key 7 = C4 */
        host_tracks_init();song.g[G_BPM]=120;song.g[G_CLOCK]=0;song.sel=0;song.playing=0;
        for(c=0;c<NTRK;c++){memset(trk[c].step,0,sizeof trk[c].step);for(j=0;j<NSTEP;j++)trk[c].step[j].time=ST_REST;}
        for(c=0;c<NTRK;c++)steps_clear(&trk[c]);
        dtest_set(3, 0,LV_NORM,0);
        rec_wait=1;
        for(b=0;b<10;b++)mix_block(out,CTL);            /* nothing happens while it waits */
        assert(!song.playing && !song.rec && rec_wait==1);
        fm1_in.notes=1u<<k0;mix_block(out,CTL);
        assert(song.playing && song.rec==1u && !rec_wait && rec_go);
        assert(trk[0].step[0].n==1 && trk[0].step[0].note[0]==60 && trk[0].step[0].time==ST_NOTE);
        fm1_in.notes=0;mix_block(out,CTL);
        transport_req=2;mix_block(out,CTL);
        assert(!song.playing && !song.rec && !rec_wait);     /* STOP ends the take */
        /* playing: recording starts at once; a note goes to the step heard: up to REC_LAT after
         * the middle of a step it is still that step, later the next one */
        song.sel=1;trk[1].p[P_SLEN]=16;period=div_samples((uint32_t)trk[1].p[P_SDIV]);half=period/2u;
        transport_req=1;mix_block(out,CTL);
        while(trk[1].seq_idx!=5u)mix_block(out,CTL);
        rec_begin();assert(song.rec==2u);
        while(into_s(&trk[1])<half+REC_LAT/2u)mix_block(out,CTL);
        fm1_in.notes=1u<<k0;mix_block(out,CTL);fm1_in.notes=0;mix_block(out,CTL);
        assert(trk[1].step[5].n==1 && !trk[1].step[6].n);   /* late by less than the latency: on 5 */
        while(trk[1].seq_idx!=9u)mix_block(out,CTL);
        while(into_s(&trk[1])<half+REC_LAT+2u*CTL)mix_block(out,CTL);
        fm1_in.notes=1u<<(k0+2u);mix_block(out,CTL);fm1_in.notes=0;mix_block(out,CTL);
        assert(!trk[1].step[9].n && trk[1].step[10].n==1 && trk[1].step[10].note[0]==62);
        transport_req=2;mix_block(out,CTL);song.rec=0;rec_wait=0;
    }
    /* (PHEBUS wave 1: the free take is cut) an empty project armed: the first note starts the loop at the tempo set,
     * recording on, the note on step 1, as in a project with notes; the tempo and the lengths do not change */
    {
        uint32_t c, k0 = 7u;
        host_tracks_init();song.g[G_BPM]=97;song.g[G_CLOCK]=0;song.sel=0;song.playing=0;song.rec=0;rec_count=0;
        for(c=0;c<NTRK;c++)steps_clear(&trk[c]);
        rec_wait=1;mix_block(out,CTL);
        assert(!song.playing && rec_wait);
        fm1_in.notes=1u<<k0;mix_block(out,CTL);fm1_in.notes=0;mix_block(out,CTL);
        assert(song.playing && song.rec==1u && !rec_wait && song.g[G_BPM]==97 && trk[0].p[P_SLEN]==16);
        assert(trk[0].step[0].n==1 && trk[0].step[0].note[0]==60);
        transport_req=2;mix_block(out,CTL);song.rec=0;rec_wait=0;
    }
    /* the arm follows the selected track (rec_follow), and a disarmed song stays disarmed */
    song.rec=1u;rec_follow(2);assert(song.rec==4u);rec_follow(3);assert(song.rec==8u);
    song.rec=0;rec_follow(1);assert(song.rec==0);
    total=(FS*4u/CTL)*CTL;
    FILE *f=argc>1?fopen(argv[1],"wb"):0;
    if(f)wav_hdr(f,total);
    host_tracks_init();song.g[G_BPM]=120;song.rec=1u;transport_req=1;
    for(j=0;j<FS*4u/CTL;j++) {
        mix_block(out,CTL);
        for(k=0;k<CTL;k++)if(f)wav_put(f,out[k*2],out[k*2+1]);
    }
    transport_req=2;events_block(CTL);song.rec=0;
    if(f)fclose(f);
    puts("drums: 16 lanes a step (levels, ratchets), the key / GM maps, every kit distinct, bounded audio and finite tails, metronome (OFF/REC/ON, accent), record: first note starts the loop, at once while playing, latency-compensated steps, free takes set the loop and tempo, arm follows the track PASS");
    return 0;
}
