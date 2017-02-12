void executeCommand(u8 channel){//the code to figure out what bytes do what actions
	curCommand=song[trackPos[channel]];
	if(curCommand<0xD0){//notes
		trackNoteLength[channel]=((curCommand&0x0F)+1)*trackSpeed[channel];
		if(curCommand>0x0F){//note
			trackNote[channel]=(curCommand>>4)-1;
			trackBaseFreq[channel]=freqTable[((7-trackOctave[channel])*12)+trackNote[channel]];
			if(channel!=2){
				writeNRx2(channel,trackEnvelope[channel]);
			}else{
				NR32Volume=(trackEnvelope[2]>>4);
			}
		}else{//rest
			trackBaseFreq[channel]=0;
			if(channel!=2){
				writeNRx2(channel,0);
			}else{
				NR32Volume=0;
			}
		}
	}else{
		switch(curCommand){
			case 0xD0://octave 8
			case 0xD1://octave 7
			case 0xD2://octave 6
			case 0xD3://octave 5
			case 0xD4://octave 4
			case 0xD5://octave 3
			case 0xD6://octave 2
			case 0xD7://octave 1
				trackOctave[channel]=curCommand&7;
			break;
			case 0xD8://note length + intensity
				trackPos[channel]++;
				trackSpeed[channel]=song[trackPos[channel]];
			case 0xDC://intensity
				trackPos[channel]++;
				trackEnvelope[channel]=song[trackPos[channel]];
				if(channel==2) writeWAV(trackEnvelope[2]&0x0F);
			break;
			case 0xD9://set starting octave
				trackPos[channel]++;
			break;
			case 0xDA://tempo
				trackPos[channel]++;
				tempo=(song[trackPos[channel]]<<8);
				trackPos[channel]++;
				tempo+=song[trackPos[channel]];
			break;
			case 0xDB://duty cycle
				trackPos[channel]++;
				if(channel<2){
					NRx1Duty[channel]=song[trackPos[channel]];
					trackUseArpDuty[channel]=false;
				}
			break;
			case 0xDD://update sound status

			break;
			case 0xDE://sfx duty
				trackPos[channel]++;
				if(channel<2){
					curCommand=song[trackPos[channel]];
					trackArpDuty[(channel*4)]=(curCommand>>6);
					trackArpDuty[(channel*4)+1]=(curCommand>>4)&3;
					trackArpDuty[(channel*4)+2]=(curCommand>>2)&3;
					trackArpDuty[(channel*4)+3]=(curCommand&3);
					trackUseArpDuty[channel]=true;
				}
			break;
			case 0xDF://sound on/off

			break;
			case 0xE0://pitch wheel
				trackPos[channel]++;
				trackPos[channel]++;
			break;
			case 0xE1://vibrato
				trackPos[channel]++;
				trackVibratoDelay[channel]=trackVibratoDelayTimer[channel]=song[trackPos[channel]];
				trackPos[channel]++;
				curCommand=song[trackPos[channel]];
				trackVibratoSpeed[channel]=trackVibratoTimer[channel]=(curCommand>>4);
				trackVibratoDepthAdd[channel]=trackVibratoDepthSub[channel]=((curCommand&0x0F)>>1);
				trackVibratoDepthAdd[channel]+=(curCommand&1);//increase this if the mod was an odd number
			break;
			case 0xE2://unused

			break;
			case 0xE3://music noise sampling
				trackPos[channel]++;
			break;
			case 0xE4://force panning
				trackPos[channel]++;
			break;
			case 0xE5://volume
				trackPos[channel]++;
			break;
			case 0xE6://tone
				trackPos[channel]++;
				trackTone[channel]+=(song[trackPos[channel]]<<8);
				trackPos[channel]++;
				trackTone[channel]+=song[trackPos[channel]];
			break;
			case 0xE7://unused

			break;
			case 0xE8://unused

			break;
			case 0xE9://global tempo
				trackPos[channel]++;
				tempo+=(song[trackPos[channel]]<<8);
				trackPos[channel]++;
				tempo+=song[trackPos[channel]];

			break;
			case 0xEA://restart current channel from header

			break;
			case 0xEB://new song

			break;
			case 0xEC://sfx priority on

			break;
			case 0xED://sfx priority off

			break;
			case 0xEE://unused

			break;
			case 0xEF://stereo panning
				trackPos[channel]++;
			break;
			case 0xF0://sfx noise sampling

			break;
			case 0xF1://nothing

			break;
			case 0xF2://nothing

			break;
			case 0xF3://nothing

			break;
			case 0xF4://nothing

			break;
			case 0xF5://nothing

			break;
			case 0xF6://nothing

			break;
			case 0xF7://nothing

			break;
			case 0xF8://nothing

			break;
			case 0xF9://unused

			break;
			case 0xFA://setcondition

			break;
			case 0xFB://jumpif

			break;
			case 0xFC://jump

			break;
			case 0xFD://loop
				trackPos[channel]++;
				trackLoopNumber[channel]=song[trackPos[channel]];
				trackPos[channel]++;
				trackLoopTo[channel]=song[trackPos[channel]];
				trackPos[channel]++;
				trackLoopTo[channel]+=(song[trackPos[channel]]<<8)-0x4001;
				trackPos[channel]=trackLoopTo[channel];
			break;
			case 0xFE://call

			break;
			case 0xFF://return

			break;
		}
	}
	trackPos[channel]++;
}

void playerProcess(u8 channel){//main engine code
	//Serial.println(trackNoteLength[2], HEX);
	while(trackNoteLength[channel]==0) executeCommand(channel);
	if(trackBaseFreq[channel]>0){
		if(trackVibratoState[channel]==0) NRxFreq[channel]=trackBaseFreq[channel]+trackVibratoDepthAdd[channel]+trackTone[channel];
		if(trackVibratoState[channel]==1) NRxFreq[channel]=(trackBaseFreq[channel]-trackVibratoDepthSub[channel])+trackTone[channel];
		NRxFreq[channel]&=0x7FF;
		CHFreq[channel]=GetFreq(NRxFreq[channel]);
		
	}
	trackNoteLength[channel]--;
}

