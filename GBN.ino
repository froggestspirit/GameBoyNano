typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef signed char s8;
typedef signed short s16;
typedef signed long s32;

#include "tables.h"

const u8 PRECISION_DEPTH=13;
const u16 PRECISION_SCALER=(1<<PRECISION_DEPTH);
const u8 GLOBAL_DIVIDE=100;
const u32 GLOBAL_TIMER=(1000000/GLOBAL_DIVIDE);
const u32 FRAME_MINUS=(GLOBAL_TIMER*PRECISION_SCALER)/60;
const u32 FRAME_SEQ_MINUS=(GLOBAL_TIMER*PRECISION_SCALER)/512;
volatile u8 outputL;
volatile u8 outputR;

u32 frame;
u32 frameSeq;
u8 frameSeqFrame;
u8 envelope;
u8 envelopeTimer;
u8 envelopeVolume;
s8 envelopeSign;

u8 NR10;
u8 NR11;
u8 NR12;
u8 NR12Timer;
u8 NR12Volume;
bool NR12Sign;
u8 NR13;
u8 NR14;
u8 NR21;
u8 NR22;
u8 NR22Timer;
u8 NR22Volume;
bool NR22Sign;
u8 NR23;
u8 NR24;
u8 NR30;
u8 NR31;
u8 NR32;
u8 NR33;
u8 NR34;
u8 NR41;
u8 NR42;
u8 NR42Timer;
u8 NR42Volume;
bool NR42Sign;
u8 NR43;
u8 NR44;
u8 NR50;
u8 NR51;
u8 NR52;
bool CH1ENL;
bool CH1ENR;
bool CH2ENL;
bool CH2ENR;
bool CH3ENL;
bool CH3ENR;
bool CH4ENL;
bool CH4ENR;
u32 CH1FPos;
u32 CH2FPos;
u32 CH3FPos;
u32 CH4FPos;
u32 CH1Freq;
u32 CH2Freq;
u32 CH3Freq;
u32 CH4Freq;

u8 WAV[32];

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

	NR11=0xC0;
	NR12=0x97;
	NR12Timer=(NR12 & 0b00000111);
	NR12Volume=((NR12 & 0b11110000)>>4);
	NR12Sign=((NR12 & 0b00001000)>>3);
	NR13=0x28;
	NR14=0x06;

	NR21=0xC0;
	NR22=0xA7;
	NR22Timer=(NR22 & 0b00000111);
	NR22Volume=((NR22 & 0b11110000)>>4);
	NR22Sign=((NR22 & 0b00001000)>>3);
	NR23=0x9D;
	NR24=0x06;

	NR32=0x40;
	NR33=0x3B;
	NR34=0x05;
	CH1ENL=1;
	CH2ENL=1;
	CH3ENL=1;
	CH1FPos=0;
	CH2FPos=0;
	CH3FPos=0;
	CH4FPos=0;
	CH1Freq=GetFreq(NR13+((NR14&0b00000111)<<8));
	CH2Freq=GetFreq(NR23+((NR24&0b00000111)<<8));
	CH3Freq=GetFreq(NR33+((NR34&0b00000111)<<8));
	CH4Freq=CH1Freq;
	while(1){

	}
}

u32 GetFreq(u16 gbFreq){
	return ((131072*PRECISION_SCALER)/(2048-gbFreq))/(GLOBAL_TIMER/4);
}

ISR(TIMER0_COMPA_vect){
	OCR2B=outputL;
	if(frame>=FRAME_MINUS){// called at ~60Hz
		//sequencer code


		frame-=FRAME_MINUS;
	}
	if(frameSeq>=FRAME_SEQ_MINUS){// called at ~512Hz
		//Frame sequence code
		if((frameSeqFrame&1)==0){//length counter
			if((NR14 & 0b01000000)==0b01000000){//PU1
				if((NR11 & 0b00111111)>0){
					NR11--;
					if((NR11 & 0b00111111)==0){
						NR52&=0b01110111;
						CH1ENL=0;
						CH1ENR=0;
					}
				}
			}
			if((NR24 & 0b01000000)==0b01000000){//PU2
				if((NR21 & 0b00111111)>0){
					NR21--;
					if((NR21 & 0b00111111)==0){
						NR52&=0b10111011;
						CH2ENL=0;
						CH2ENR=0;
					}
				}
			}
			if((NR34 & 0b01000000)==0b01000000){//WAVE
				if(NR31>0){
					NR31--;
					if(NR31==0){
						NR52&=0b11011101;
						CH3ENL=0;
						CH3ENR=0;
					}
				}
			}
			if((NR44 & 0b01000000)==0b01000000){//NSE
				if((NR41 & 0b00111111)>0){
					NR41--;
					if((NR41 & 0b00111111)==0){
						NR52&=0b11101110;
						CH4ENL=0;
						CH4ENR=0;
					}
				}
			}
		}

		if((frameSeqFrame&7)==7){//envelope counter
			if(NR12Timer!=0){//PU1
				NR12Timer--;
				if(NR12Timer==0){
					if(NR12Sign==1){
						if(NR12Volume<0x0F){
							NR12Volume++;
							NR12Timer=(NR12 & 0b00000111);
						}
					}else if(NR12Sign==0){
						if(NR12Volume>0x00){
							NR12Volume--;
							NR12Timer=(NR12 & 0b00000111);
						}
					}
				}
			}
			if(NR22Timer!=0){//PU2
				NR22Timer--;
				if(NR22Timer==0){
					if(NR22Sign==1){
						if(NR22Volume<0x0F){
							NR22Volume++;
							NR22Timer=(NR22 & 0b00000111);
						}
					}else if(NR22Sign==0){
						if(NR22Volume>0x00){
							NR22Volume--;
							NR22Timer=(NR22 & 0b00000111);
						}
					}
				}
			}
			if(NR42Timer!=0){//NSE
				NR42Timer--;
				if(NR42Timer==0){
					if(NR42Sign==1){
						if(NR42Volume<0x0F){
							NR42Volume++;
							NR42Timer=(NR42 & 0b00000111);
						}
					}else if(NR42Sign==0){
						if(NR42Volume>0x00){
							NR42Volume--;
							NR42Timer=(NR42 & 0b00000111);
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
	u8 ch1WavPos=dutyTable[((CH1FPos>>PRECISION_DEPTH)&0b00000111)+(((NR11&0b11000000)>>6)*8)];
	u8 ch2WavPos=dutyTable[((CH2FPos>>PRECISION_DEPTH)&0b00000111)+(((NR21&0b11000000)>>6)*8)];
	u8 ch3WavPos=waveTable[((CH3FPos>>PRECISION_DEPTH)&0b00011111)+(5*32)];
	ch3WavPos=(ch3WavPos >> waveShift[((NR32 & 0b01100000)>>5)]);
	CH1FPos+=CH1Freq;
	CH2FPos+=CH2Freq;
	CH3FPos+=CH3Freq*2;
	outputL+=(ch1WavPos*NR12Volume*CH1ENL);
	outputL+=(ch2WavPos*NR22Volume*CH2ENL);
	outputL+=(ch3WavPos*CH3ENL);
	//outputL+=(NR42Volume*CH4ENL);
	frame+=PRECISION_SCALER;
	frameSeq+=PRECISION_SCALER;
	//Serial.println(ch3WavPos, DEC);
}
