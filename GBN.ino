typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef signed char s8;
typedef signed short s16;
typedef signed long s32;

const u8 PRECISION_DEPTH=12;
const u16 PRECISION_SCALER=(1<<PRECISION_DEPTH);
const u8 GLOBAL_DIVIDE=92;
const u32 GLOBAL_TIMER=(1000000/GLOBAL_DIVIDE);
const u32 FRAME_MINUS=(GLOBAL_TIMER*PRECISION_SCALER)/30;
const u32 FRAME_SEQ_MINUS=(GLOBAL_TIMER*PRECISION_SCALER)/256;

#include "tables.h"
#include "freqtable.h"
#include "song.h"

volatile u8 outputL;
volatile u8 outputR;

bool bufFrame;
u32 frame;
u32 frameSeq;
u8 frameSeqFrame;

//GB variables
u8 NR10Period;
bool NR10Negate;
u8 NR10Shift;
u8 NRx1Duty[2];//only the first 2 channels use this
u8 NRx1Length[4];//6-bit for channels 1,2,4. 8-bit for channel 3
u8 NRx2Decay[4];
u8 NRx2Velocity[4];
u8 NRx2Timer[4];
u8 NRx2Volume[4];
bool NRx2Sign[4];
bool NRx4LenEn[4];//length enable flag
bool NRx4Trigger[4];//Trigger a note
u16 NRxFreq[4];
u8 NR30DAC;
u8 NR32Volume;
u8 NR43;
u8 NR50;
u8 NR51;
u8 NR52;
bool CHENL[4];
bool CHENR[4];
u32 CHFPos[4];
u32 CHFreq[4];

u8 WAV[32];

//sequencer variables
bool playing;
u16 tempo;
u8 curCommand;
u16 trackPos[4];//position in the song array of each track
u16 trackRetPos[4];//position in the song array of each track
u16 trackLoopTo[4];
u8 trackLoopNumber[4];
bool trackLooping[4];
u16 trackDelay[4];//time before next event is read
u16 trackTone[4];
u8 trackNote[4];
u8 trackEnvelope[4];
u8 trackSpeed[4];
u8 trackOctave[4];
s16 trackNoteLength[4];
u8 trackArpDuty[4*4];
bool trackUseArpDuty[4];
//trackPitch[4];
//trackNotePitch[4];
//trackFinalPitch[4];
u8 trackVibratoDepth[4];
u8 trackVibratoDepthAdd[4];
u8 trackVibratoDepthSub[4];
u8 trackVibratoSpeed[4];
u8 trackVibratoDelay[4];
u8 trackVibratoDelayTimer[4];
u8 trackVibratoTimer[4];
u8 trackVibratoState[4];
bool trackDone[4];//track is done playing

u32 GetFreq(u16 gbFreq){
	return pgm_read_word_near(freqTable + gbFreq);
}

void initPlayer(){//set up the variables for starting a song
	for(int i=0; i<4; i++){
		NRx1Length[i]=0;
		NRx2Decay[i]=0;
		NRx2Velocity[i]=0;
		NRx2Timer[i]=0;
		NRx2Volume[i]=0;
		NRx2Sign[i]=0;
		NRx4LenEn[i]=0;
		NRx4Trigger[i]=0;
		NRxFreq[i]=0;
		CHENL[i]=1;
		CHENR[i]=1;
		CHFPos[i]=0;
		CHFreq[i]=0;
		trackPos[i]=0;
		trackRetPos[i]=0;
		trackLoopTo[i]=0;
		trackLoopNumber[i]=0xFF;
		trackLooping[i]=false;
		trackDelay[i]=0;
		trackTone[i]=0;
		trackNote[i]=0;
		trackEnvelope[i]=0;
		trackSpeed[i]=1;
		trackOctave[i]=0;
		trackNoteLength[i]=0;
		trackArpDuty[i*4]=0;
		trackArpDuty[(i*4)+1]=0;
		trackArpDuty[(i*4)+2]=0;
		trackArpDuty[(i*4)+3]=0;
		trackUseArpDuty[i]=false;
		trackVibratoDepth[i]=0;
		trackVibratoDepthAdd[i]=0;
		trackVibratoDepthSub[i]=0;
		trackVibratoSpeed[i]=0;
		trackVibratoDelay[i]=0;
		trackVibratoDelayTimer[i]=0;
		trackVibratoTimer[i]=0;
		trackVibratoState[i]=0;
		trackDone[i]=false;
	}
	NR10Period=0;
	NR10Negate=0;
	NR10Shift=0;
	NRx1Duty[0]=0;
	NRx1Duty[1]=0;
	NR30DAC=1;
	NR32Volume=0;
	NR43=0;
	NR50=0;
	NR51=0;
	NR52=0;
	for(int i=0; i<32; i++) WAV[i]=0;
	playing=false;
	tempo=0x0100;
	curCommand=0;
}

void writeNR10(u8 value){//sweep for channel 1
	NR10Shift=(value&0b00000111);
	NR10Period=((value>>4)&0b00000111);
	NR10Negate=((value&0b00001000)>>3); 
}

u8 readNR10(){//sweep for channel 1
	return(NR10Shift+(NR10Negate<<3)+(NR10Period<<4));
}

void writeNRx2(u8 channel, u8 value){//envelope, channels 1,2, and 4. 3 is unused here
	NRx2Decay[channel]=NRx2Timer[channel]=(value&0b00000111);
	NRx2Velocity[channel]=NRx2Volume[channel]=(value>>4);
	NRx2Sign[channel]=((value&0b00001000)>>3); 
}

u8 readNRx2(u8 channel){//envelope, channels 1,2, and 4. 3 is unused here
	return(NRx2Decay[channel]+(NRx2Sign[channel]<<3)+(NRx2Velocity[channel]<<4));
}

void writeWAV(u8 index){//copies waveform to the WAV buffer
	for(u8 i=0; i<32; i++) WAV[i]=waveTable[(index*32)+i];
}

#include "pkmplay.h"

int main() {
	//Serial.begin(9600);
	asm("cli");
	CLKPR = 0x80;
	CLKPR = 0x80;

	DDRC = 0x12;
	DDRD = 0xff;


	TCCR0A = 0x02;
	TCCR0B = 0x02;	// 1000000 MHz
	OCR0A = GLOBAL_DIVIDE;

	TCCR2A=0b10100011;
	TCCR2B=0b00000001;

	TIMSK0 = 0x02;

	asm("sei");
	frame=0;
	frameSeq=0;
	frameSeqFrame=0;
	initPlayer();
	trackDone[3]=true;//disable channel 4 for now

	while(1){

	}
}

ISR(TIMER0_COMPA_vect){
	OCR2B=outputL;
  if(frame>=FRAME_MINUS){// called at ~60Hz
		//sequencer code
		if(!playing){
			trackPos[0]=(song[1]+(song[2]<<8))-0x4000;
			trackPos[1]=(song[4]+(song[5]<<8))-0x4000;
			trackPos[2]=(song[7]+(song[8]<<8))-0x4000;
			//trackPos[3]=(song[10]+(song[11]<<8))-0x4000;
			playing=true;
		}else{
			for(u8 i=0; i<4; i++){
				if(trackDone[i]==false){
				   playerProcess(i);
				}
			}
		}
		frame-=FRAME_MINUS;
	}
	if(frameSeq>=FRAME_SEQ_MINUS){// called at ~512Hz
		//Frame sequence code
		if((frameSeqFrame&1)==0){//length counter
			for(u8 channel=0; channel<4; channel++){
				if(NRx4LenEn[channel]==1){
					if(NRx1Length[channel]>0){
						NRx1Length[channel]--;
						if(NRx1Length[channel]==0){
							NR52&=NR52Mask[channel];
							CHENL[channel]=0;
							CHENR[channel]=0;
						}
					}
				}
			}
		}

		if((frameSeqFrame&7)==7){//envelope counter
			for(u8 channel=0; channel<4; channel++){
				if(NRx2Timer[channel]!=0){
					NRx2Timer[channel]--;
					if(NRx2Timer[channel]==0){
						if(NRx2Sign[channel]==1){
							if(NRx2Volume[channel]<0x0F){
								NRx2Volume[channel]++;
								NRx2Timer[channel]=NRx2Decay[channel];
							}
						}else if(NRx2Sign[channel]==0){
							if(NRx2Volume[channel]>0x00){
								NRx2Volume[channel]--;
								NRx2Timer[channel]=NRx2Decay[channel];
							}
						}
					}
				}
			}
		}

		if((frameSeqFrame&3)==2){//sweep counter

		}

		frameSeqFrame++;
		frameSeq-=FRAME_SEQ_MINUS;
	}
  outputL=0;
	u8 chWavPos=dutyTable[((CHFPos[0]>>PRECISION_DEPTH)&0b00000111)+(NRx1Duty[0]*8)];
	outputL+=(chWavPos*NRx2Volume[0]*CHENL[0]);
	chWavPos=dutyTable[((CHFPos[1]>>PRECISION_DEPTH)&0b00000111)+(NRx1Duty[1]*8)];
	outputL+=(chWavPos*NRx2Volume[1]*CHENL[1]);
	chWavPos=WAV[((CHFPos[2]>>PRECISION_DEPTH)&0b00011111)];
	chWavPos=(chWavPos >> waveShift[NR32Volume]);
	outputL+=(chWavPos*CHENL[2]*NR30DAC);
	CHFPos[0]+=CHFreq[0];
	CHFPos[1]+=CHFreq[1];
	CHFPos[2]+=CHFreq[2]*2;
	//outputL+=(NRx2Volume[3]*CHENL[3]);
	frame+=PRECISION_SCALER;
	frameSeq+=PRECISION_SCALER;
}
