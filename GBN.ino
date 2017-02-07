#include <avr/io.h>
#include <avr/signal.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

extern "C" {
  
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef signed char s8;
typedef signed short s16;
typedef signed long s32;

const u16 PRECISION_SCALER=0x4000;
const u8 GLOBAL_DIVIDE=101;
const u32 GLOBAL_TIMER=(1000000/GLOBAL_DIVIDE);
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

const u8 dutyTable[] = {
  0, 0, 0, 0, 0, 0, 0, 1,
  1, 0, 0, 0, 0, 0, 0, 1,
  1, 0, 0, 0, 0, 1, 1, 1,
  0, 1, 1, 1, 1, 1, 1, 0
};
const u8 waveTable[] = {
  0, 2, 4, 6, 8, 10, 12, 14, 15, 15, 15, 14, 14, 13, 13, 12, 12, 11, 10, 9, 8, 7, 6, 5, 4, 4, 3, 3, 2, 2, 1, 1,
  0, 2, 4, 6, 8, 10, 12, 14, 14, 15, 15, 15, 15, 14, 14, 14, 13, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 2, 1, 1,
  1, 3, 6, 9, 11, 13, 14, 14, 14, 14, 15, 15, 15, 15, 14, 13, 13, 14, 15, 15, 15, 15, 14, 14, 14, 14, 13, 11, 9, 6, 3, 1,
  0, 2, 4, 6, 8, 10, 12, 13, 14, 15, 15, 14, 13, 14, 15, 15, 14, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
  0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 13, 14, 14, 15, 7, 7, 15, 14, 14, 13, 12, 10, 8, 7, 6, 5, 4, 3, 2, 1, 0,
  0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 3, 3, 2, 2, 1, 1, 15, 15, 14, 14, 12, 12, 10, 10, 8, 8, 10, 10, 12, 12, 14, 14,
  0, 2, 4, 6, 8, 10, 12, 14, 12, 11, 10, 9, 8, 7, 6, 5, 15, 15, 15, 14, 14, 13, 13, 12, 4, 4, 3, 3, 2, 2, 1, 1,
  12, 0, 10, 9, 8, 7, 15, 5, 15, 15, 15, 14, 14, 13, 13, 12, 4, 4, 3, 3, 2, 2, 15, 1, 0, 2, 4, 6, 8, 10, 12, 14,
  4, 4, 3, 3, 2, 2, 1, 15, 0, 0, 4, 6, 8, 10, 12, 14, 15, 8, 15, 14, 14, 13, 13, 12, 12, 11, 10, 9, 8, 7, 6, 5,
  1, 1, 0, 0, 0, 0, 0, 8, 0, 0, 1, 3, 5, 7, 9, 10, 11, 4, 11, 10, 10, 9, 9, 8, 8, 7, 6, 5, 4, 3, 2, 1
};
const u8 waveShift[] = {
  4, 0, 1, 2
};

int main() {
  //Serial.begin(9600);
  asm("cli");
  CLKPR = 0x80;
  CLKPR = 0x80;
  
  DDRC = 0x12;
  DDRD = 0xff;
  
  
  TCCR0A = 0x02;
  TCCR0B = 0x02;	// clkIO/8, so 1/8 MHz
  OCR0A = GLOBAL_DIVIDE;
  
  TCCR2A=0b10100011;
  TCCR2B=0b00000001;
  
  TIMSK0 = 0x02;
  
  asm("sei");
  frame=0;
  frameSeq=0;
  frameSeqFrame=0;
  
  NR11=0xC0;
  NR12=0x92;
  NR12Timer=(NR12 & 0b00000111);
  NR12Volume=((NR12 & 0b11110000)>>4);
  NR12Sign=((NR12 & 0b00001000)>>3);
  NR13=0x28;
  NR14=0x06;

  NR21=0xC0;
  NR22=0xA2;
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
  return (131072*PRECISION_SCALER)/(2048-gbFreq);
}

ISR(TIMER0_COMPA_vect){
  OCR2B=outputL;
if(frame>=(GLOBAL_TIMER*PRECISION_SCALER)/60){// called at ~60Hz
  //sequencer code

  
  frame-=(GLOBAL_TIMER*PRECISION_SCALER)/60;
}
if(frameSeq>=(GLOBAL_TIMER*PRECISION_SCALER)/512){// called at ~512Hz
  //Frame sequence code
  if((frameSeqFrame%2)==0){//length counter
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
  
  if((frameSeqFrame%8)==7){//envelope counter
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
  
  if((frameSeqFrame%4)==2){//sweep counter

  }
  
  frameSeqFrame++;
  frameSeq-=(GLOBAL_TIMER*PRECISION_SCALER)/512;
}

	


	outputL=0;
  u8 ch1WavPos=dutyTable[((CH1FPos/PRECISION_SCALER)&0b00000111)+(((NR11&0b11000000)>>6)*8)];
  u8 ch2WavPos=dutyTable[((CH2FPos/PRECISION_SCALER)&0b00000111)+(((NR21&0b11000000)>>6)*8)];
  u8 ch3WavPos=waveTable[((CH3FPos/PRECISION_SCALER)&0b00011111)+(5*32)];
  ch3WavPos=(ch3WavPos >> waveShift[((NR32 & 0b01100000)>>5)]);
  CH1FPos+=(CH1Freq/GLOBAL_TIMER)*8;
  CH2FPos+=(CH2Freq/GLOBAL_TIMER)*8;
  CH3FPos+=(CH3Freq/GLOBAL_TIMER)*32;
  CH1FPos%=8;
  CH2FPos%=8;
  CH3FPos%=32;
	outputL+=(ch1WavPos*NR12Volume*CH1ENL);
  outputL+=(ch2WavPos*NR22Volume*CH2ENL);
  outputL+=(ch3WavPos*CH3ENL);
	//outputL+=(NR42Volume*CH4ENL);
  frame+=PRECISION_SCALER;
  frameSeq+=PRECISION_SCALER;
}

}
