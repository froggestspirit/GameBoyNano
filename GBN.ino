typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef signed char s8;
typedef signed short s16;
typedef signed long s32;

const u8 PRECISION_DEPTH=12;
const u8 PRECISION_DEPTHP3=PRECISION_DEPTH+3;
const u16 PRECISION_SCALER=(1<<PRECISION_DEPTH);
const u8 GLOBAL_DIVIDE=125;
const u32 GLOBAL_TIMER=(1000000/GLOBAL_DIVIDE);
const u32 FRAME_MINUS=(GLOBAL_TIMER*PRECISION_SCALER)/30;
const u32 FRAME_SEQ_MINUS=(GLOBAL_TIMER*PRECISION_SCALER)/256;

const u32 SONG_POINTERS=0xE9071;
const u32 DRUMKIT_POINTERS=0xE8E5E;
const u32 DRUMKIT_DATA=0xE8EFA;//so we can get the size of the drumkit pointers
const u32 DRUMKIT_END=0xE8FC2;//so we can get the size of the drumkits
const u32 WAVEFORM_POINTERS=0xE8DB2;
const u8 TOTAL_SONGS=102;

u8 curSong;
u32 songAddress;
u16 songOffset;

#include <SD.h>

File songFile;
const int chipSelect = 4;
const int buttonNext = 2;
bool buttonNextState;
bool buttonNextLState;

#include "tables.h"
#include "freqtable.h"
#include "lfsr.h"

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
u8 NRx2Decay[4];//used for channels 1,2 and 4. channel 4 is index "2", not "3"
u8 NRx2Velocity[4];
u8 NRx2Timer[4];
u8 NRx2Volume[4];
bool NRx2Sign[4];
bool NRx4LenEn[4];//length enable flag
bool NRx4Trigger[4];//Trigger a note
u16 NRxFreq[4];
bool NR43Stage;//15 or 7 stage mode?
u8 NR30DAC;
u8 NR32Volume;
u8 NR43;
u8 NR50;
u8 NR51;
u8 NR52;
bool CHENL[4];
bool CHENR[4];
u8 CHFPos8[4] asm("CHFPos8") __attribute__ ((used)) ;
u16 CHFPos8N asm("CHFPos8N") __attribute__ ((used)) ;
u32 CHFPos[4] asm("CHFPos") __attribute__ ((used)) ;
u32 CHFreq[4] asm("CHFreq") __attribute__ ((used)) ;

u8 WAV[32] asm("WAV") __attribute__ ((used)) ;

//sequencer variables
bool playing;
u16 tempo;
u8 curCommand;
u8 trackLooped[4];//how many times the track looped, for limiting playback to 2 times before changing song
u8 tracksComplete;//bits set for when a channel is ready to fade
u8 fadeTimer;//timer for fading
u16 trackPos[4];//position in the song array of each track
u16 trackRetPos[4];//position in the song array of each track
u16 trackLoopTo[4];
u8 trackLoopNumber[4];
bool trackLooping[4];
u16 trackDelay[4];//time before next event is read
u16 trackTone[4];
u8 trackNote[4];
s8 trackNoteOffset[4];
u8 trackEnvelope[4];
u8 trackSpeed[4];
u8 trackOctave[4];
s32 trackNoteLength[4];
u8 trackArpDuty[4*4];
bool trackUseArpDuty[4];
u8 trackVibratoDepth[4];
u8 trackVibratoDepthAdd[4];
u8 trackVibratoDepthSub[4];
u8 trackVibratoSpeed[4];
u8 trackVibratoDelay[4];
u8 trackVibratoDelayTimer[4];
u8 trackVibratoTimer[4];
u8 trackVibratoState[4];
bool trackDone[4];//track is done playing
u8 drumIndex;//where the drumdata array is
u8 drumTimer;//delays for the drums
u8 drumSet;//FF if not initialized

u32 getFreq(u16 gbFreq){
	return pgm_read_word_near(freqTable + gbFreq);
}

u32 getNSEFreq(u16 gbFreq){
  gbFreq=(gbFreq&7)+((gbFreq&0xF0)>>1);
  gbFreq=gbFreq<<1;
  u32 r=pgm_read_word_near(freqNSETable + gbFreq+1);
  r=r<<16;
  r+=pgm_read_word_near(freqNSETable + gbFreq);
	return r;
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
		NR43Stage=0;
		CHENL[i]=0;
		CHENR[i]=0;
		CHFPos[i]=0;
		CHFreq[i]=0;
		trackLooped[i]=0;
 		trackPos[i]=0;
		trackRetPos[i]=0;
		trackLoopTo[i]=0;
		trackLoopNumber[i]=0xFF;
		trackLooping[i]=false;
		trackDelay[i]=0;
		trackTone[i]=0;
		trackNote[i]=0;
		trackNoteOffset[i]=0;
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
		trackDone[i]=true;
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
	drumTimer=0;
	drumIndex=0;
	drumSet=0xFF;
	playing=false;
	tempo=0x0100;
	fadeTimer=0x00;
	songFile.seek(SONG_POINTERS+(curSong*3));
	songAddress=0;
	songOffset=0;
	songAddress=(songFile.read());
	songAddress*=(0x4000);
	songAddress+=(songFile.read());
	songAddress+=(songFile.read())*0x100;
	songOffset=(songAddress & 0x3FFF)+0x4000;
	songAddress-=0x4000;
	songFile.seek(songAddress);
	curCommand=songFile.read();//(this tells number of channels)
	curCommand=curCommand>>6;
	curCommand++;
	songFile.seek(songAddress);
	tracksComplete=0x0F;
	u8 i=0;
	while(curCommand>0){
		i=songFile.read();//track number
		i&=3;
		trackPos[i]=(songFile.read()+(songFile.read()<<8))-songOffset;
		CHENL[i]=1;
		CHENR[i]=1;
		trackDone[i]=false;
		tracksComplete^=(1<<i);
		curCommand--;
	}
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
	for(u8 i=0; i<16; i++){
		WAV[i*2]=waveTable[index+i]>>4;
		WAV[(i*2)+1]=waveTable[index+i]&0x0F;
	}
}

void loadInstruments(){
	songFile.seek(WAVEFORM_POINTERS);
	for(int i=0; i<160; i++){
		waveTable[i]=songFile.read();
	}
	songFile.seek(DRUMKIT_POINTERS);
	u16 temp=0;
	for(int i=0; i<78; i++){
		temp=songFile.read();
		temp+=(songFile.read()<<8);		
		drumTable[i]=temp-((DRUMKIT_DATA&0x3FFF)+0x4000);//should offset drum data to 0 for the array
	}
	for(int i=0; i<200; i++){
		drumData[i]=songFile.read();
	}
}

#include "pkmplay.h"

void setup() {
	//Serial.begin(9600);
	pinMode(SS, OUTPUT);
	buttonNextState=false;
	buttonNextLState=false;
	SD.begin(chipSelect);
 
	songFile = SD.open("crystal.gbc");
	if(songFile){
		frame=0;
		frameSeq=0;
		frameSeqFrame=0;
		curSong=0;
		loadInstruments();
		initPlayer();
	}
	asm("cli");
	CLKPR = 0x80;
	CLKPR = 0x80;

	DDRC |= 0x12;
	DDRD |= 0x08;


	TCCR0A = 0x02;
	TCCR0B = 0x02;  // 1000000 MHz
	OCR0A = GLOBAL_DIVIDE;

	TCCR2A=0b10100011;
	TCCR2B=0b00000001;

	TIMSK0 = 0x02;
	asm("sei");
}

void loop(){
  
}

ISR(TIMER0_COMPA_vect){
	OCR2B=(outputL >> (fadeTimer>>5));
	if(frame>=FRAME_MINUS){// called at ~60Hz
		buttonNextLState=buttonNextState;
		buttonNextState=(PIND&4);
		if((tracksComplete&0x0F)==0x0F){
			fadeTimer++;
			if(tracksComplete==0xFF) fadeTimer=0xF0;
			if(fadeTimer>=0xF0){
				curSong++;
				curSong%=TOTAL_SONGS;
				//skip these to avoid glitches
				if(curSong==31) curSong++;//after rival fight
				if(curSong==57) curSong++;//look poke-maniac (this song will loop endlessly due to loop structure in ch1)
				if(curSong==73) curSong++;//wild battle night
				if(curSong==75) curSong++;//wild victory 2
				initPlayer();
			}
		}
		//sequencer code
		for(u8 i=0; i<3; i++){
			if(!trackDone[i]){
			   playerProcess(i);
			}
		}
		if(!trackDone[3]){
			playerProcessNSE();
		}
		frame-=FRAME_MINUS;
	}
	if(frameSeq>=FRAME_SEQ_MINUS){// called at ~512Hz
		//Frame sequence code Disabled for now to speed up
		/*if((frameSeqFrame&1)==0){//length counter
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
		}*/

		if((frameSeqFrame&7)==7){//envelope counter
			for(u8 channel=0; channel<3; channel++){
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

		//if((frameSeqFrame&3)==2){//sweep counter

		//}

		frameSeqFrame++;
		frameSeq-=FRAME_SEQ_MINUS;
	}
	outputL=0;
	asm volatile (//this makes the multiple bit shifts for precision more optimal
		"ldi r27, hi8(CHFPos+1)\n\t"//set X to point to CHPos+1
		"ldi r26, lo8(CHFPos+1)\n\t"

		"ldi r31, hi8(CHFPos8)\n\t"//set Z to point to CHPos8 (setting Y didn't work)
		"ldi r30, lo8(CHFPos8)\n\t"
		
		//goal here is CHPos8[0]=((CHPos>>12)&7) 00000000 00000000 01110000 00000000 bits marked 1 we keep, bits after 1 are for precision, and get temporarily discarded
		"ld r25, x\n\t"//load CHPos+1 to r25
		"swap r25\n\t"//swap nibbles
		"andi r25,7\n\t"//only keep the 3 LS bits
		"st z+, r25\n\t"//store into CHPos8[0], and increase Z by 1 (now points to CHPos8 for channel 2
		"adiw r26,4\n\t"//add 4 to the X pointer (now pointing to CHPos+1 of channel 2)
		
		"ld r25, x\n\t"//same for channel 2
		"swap r25\n\t"
		"andi r25,7\n\t"
		"st z+, r25\n\t"
		"adiw r26,4\n\t"//increase by 5 here instead (pointing to CHPos+2 in channel 3 now)
		
		//goal here is CHPos8[2]=((CHPos>>12)&31) 00000000 00000000 11111000 00000000
		"ld r25, x\n\t"//load byte (the one with 1 bit we keep) into r25
		"lsr r25\n\t"//shift right 3 times
		"lsr r25\n\t"
		"lsr r25\n\t"
		"st z+, r25\n\t"//store into CHPos8[2] for ch 3
		"adiw r26,5\n\t"
		
		//goal here is CHPos8[3]=((CHPos>>17)&7) //this is so we can get the right noise bit
		"ld r25, x\n\t"
		"lsr r25\n\t"
		"andi r25,7\n\t"//only keep 3 lsb
		"st z, r25\n\t"//store into CHPos8[3]

		//goal here is CHPos8N=(CHPos>>20) 22221111 11110000 00000000 00000000 bits marked 1 go into CHPos8N(low byte), bits marked 2 go into CHPos8N(high byte)
		"ldi r31, hi8(CHFPos8N)\n\t"//set Z to point to CHPos8N
		"ldi r30, lo8(CHFPos8N)\n\t"
		"ld r25, x+\n\t"//load byte, then increase pointer X by 1
		"ld r24, x\n\t"
		"andi r24,15\n\t"
		"or r25,r24\n\t"
		"swap r25\n\t"
		"st z+, r25\n\t"//store this to CHPos8N(low byte)
		"ld r25, x\n\t"//load last byte
		"swap r25\n\t"
		"andi r25,15\n\t"//only keep 4 lsb
		"st z, r25\n\t"//store in CHFPos8N(high byte)
		
	);
	u8 chWavPos=dutyTable[CHFPos8[0]+NRx1Duty[0]];
	if(CHENL[0]) outputL+=(NRx2Volume[0]*chWavPos);
	chWavPos=dutyTable[CHFPos8[1]+NRx1Duty[1]];
	if(CHENL[1]) outputL+=(NRx2Volume[1]*chWavPos);
	chWavPos=WAV[CHFPos8[2]];
	chWavPos=(chWavPos >> waveShift[NR32Volume]);
	if(CHENL[2]) outputL+=(chWavPos*NR30DAC);
	if(NR43Stage==1){//7-stage
		chWavPos=pgm_read_byte_near(lfsr7+(CHFPos8N&0x0F));
	}else{//15-stage
		chWavPos=pgm_read_byte_near(lfsr15+CHFPos8N);
	}
	chWavPos=(chWavPos>>(7-CHFPos8[3]))&1;
	if(CHENL[3]) outputL+=(NRx2Volume[2]*chWavPos);
	CHFPos[0]+=CHFreq[0];
	CHFPos[1]+=CHFreq[1];
	CHFPos[2]+=CHFreq[2];
	CHFPos[3]+=CHFreq[3];
	frame+=PRECISION_SCALER;
	frameSeq+=PRECISION_SCALER;
}
