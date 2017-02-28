void executeCommand(u8 channel){//the code to figure out what bytes do what actions
	songFile.seek(trackPos[channel]+songAddress);
	curCommand=songFile.read();
	if(curCommand<0xD0){//notes
    trackNoteLength[channel]+=((((curCommand&0x0F)+1)*trackSpeed[channel])&0xFF)*tempo;
		trackVibratoState[channel]|=2;
		trackVibratoDelayTimer[channel]=trackVibratoDelay[channel];
		if(curCommand>0x0F){//note
			trackNote[channel]=(curCommand>>4)-1;
			NRxFreq[channel]=pgm_read_word_near(freqTableGB+(((7-trackOctave[channel])*12)+trackNote[channel])+trackNoteOffset[channel])+trackTone[channel];
			if(channel!=2){
				writeNRx2(channel,trackEnvelope[channel]);
			}else{
				NR32Volume=(trackEnvelope[2]>>4);
			}
		}else{//rest
			NRxFreq[channel]=0;
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
				trackSpeed[channel]=songFile.read();
			case 0xDC://intensity
				trackPos[channel]++;
  				trackEnvelope[channel]=songFile.read();
  				if(channel==2) writeWAV((trackEnvelope[2]&0x0F)<<4);
			break;
			case 0xD9://set starting octave
				trackPos[channel]++;
				curCommand=songFile.read();
				trackNoteOffset[channel]=(curCommand&0x0F)-((curCommand>>4)*12);
			break;
			case 0xDA://tempo
				trackPos[channel]++;
				tempo=(songFile.read()<<8);
				trackPos[channel]++;
				tempo+=songFile.read();
			break;
			case 0xDB://duty cycle
				trackPos[channel]++;
				curCommand=songFile.read();
				if(channel<2){
					NRx1Duty[channel]=(curCommand<<3);
					trackUseArpDuty[channel]=false;
				}
			break;
			case 0xDD://update sound status

			break;
			case 0xDE://sfx duty
				trackPos[channel]++;
				curCommand=songFile.read();
				if(channel<2){
					trackArpDuty[(channel<<2)]=(curCommand>>6);
					trackArpDuty[(channel<<2)+1]=(curCommand>>4)&3;
					trackArpDuty[(channel<<2)+2]=(curCommand>>2)&3;
					trackArpDuty[(channel<<2)+3]=(curCommand&3);
					trackUseArpDuty[channel]=true;
				}
			break;
			case 0xDF://sound on/off

			break;
			case 0xE0://pitch wheel
				trackPos[channel]++;
				songFile.read();
				trackPos[channel]++;
				songFile.read();
			break;
			case 0xE1://vibrato
				trackPos[channel]++;
				trackVibratoDelay[channel]=trackVibratoDelayTimer[channel]=songFile.read();
				trackPos[channel]++;
				curCommand=songFile.read();
				trackVibratoSpeed[channel]=(curCommand&0x0F);
				trackVibratoDepth[channel]=trackVibratoTimer[channel]=(curCommand>>4);
				trackVibratoDepthAdd[channel]=trackVibratoDepthSub[channel]=(trackVibratoDepth[channel]>>1);
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
				trackTone[channel]=(songFile.read()<<8);
				trackPos[channel]++;
				trackTone[channel]+=songFile.read();
			break;
			case 0xE7://unused

			break;
			case 0xE8://unused

			break;
			case 0xE9://global tempo
				trackPos[channel]++;
				tempo+=(songFile.read()<<8);
				trackPos[channel]++;
				tempo+=songFile.read();

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
				songFile.read();
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
				trackPos[channel]++;
				trackLoopTo[channel]=songFile.read();
				trackPos[channel]++;
				trackLoopTo[channel]+=(songFile.read()<<8)-(songOffset+1);
				trackPos[channel]=trackLoopTo[channel];
			break;
			case 0xFD://loop
				trackPos[channel]++;
				curCommand=songFile.read();
				if(!trackLooping[channel]){
					trackLoopNumber[channel]=curCommand;
					if(trackLoopNumber[channel]>0) trackLooping[channel]=true;//don't set if it's an infinite loop
				}
				trackPos[channel]++;
				trackLoopTo[channel]=songFile.read();
				trackPos[channel]++;
				trackLoopTo[channel]+=(songFile.read()<<8)-(songOffset+1);
				trackLoopNumber[channel]--;
				if(!trackLooping[channel]){//infinite loop
					trackPos[channel]=trackLoopTo[channel];
				}else if(trackLoopNumber[channel]>0){
					trackPos[channel]=trackLoopTo[channel];
				}else{
					trackLooping[channel]=false;
				}
			break;
			case 0xFE://call
				trackPos[channel]++;
				trackLoopTo[channel]=songFile.read();
				trackPos[channel]++;
				trackLoopTo[channel]+=(songFile.read()<<8)-(songOffset+1);
				trackRetPos[channel]=trackPos[channel];
				trackPos[channel]=trackLoopTo[channel];
			break;
			case 0xFF://return
				if(trackRetPos[channel]>0){
					trackPos[channel]=trackRetPos[channel];
					trackRetPos[channel]=0;
				}else{
					trackDone[channel]=true;
					CHENL[channel]=CHENR[channel]=0;
				}
			break;
		}
	}
	trackPos[channel]++;
}

void executeCommandNSE(){//the code to figure out what bytes do what actions in noise channel
	songFile.seek(trackPos[3]+songAddress);
	curCommand=songFile.read();
	if(curCommand<0xD0){//notes
		trackNoteLength[3]+=((((curCommand&0x0F)+1)*trackSpeed[3])&0xFF)*tempo;
		if(curCommand>0x0F){//note
			trackNote[3]=(curCommand>>4);
			drumTimer=0;
			drumIndex=drumTable[(drumSet*13)+trackNote[3]];
			NRxFreq[3]=0;
		}else{//rest
			NRxFreq[3]=0xFFFF;
			writeNRx2(2,0);
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
			break;
			case 0xD8://note length + intensity
				trackPos[3]++;
				trackSpeed[3]=songFile.read();
			case 0xDC://intensity
			break;
			case 0xD9://set starting octave
				trackPos[3]++;
			break;
			case 0xDA://tempo
				trackPos[3]++;
				tempo=(songFile.read()<<8);
				trackPos[3]++;
				tempo+=songFile.read();
			break;
			case 0xDB://duty cycle
				trackPos[3]++;;
			break;
			case 0xDD://update sound status

			break;
			case 0xDE://sfx duty
				trackPos[3]++;
			break;
			case 0xDF://sound on/off

			break;
			case 0xE0://pitch wheel
				trackPos[3]++;
				trackPos[3]++;
			break;
			case 0xE1://vibrato
				trackPos[3]++;
				trackPos[3]++;
			break;
			case 0xE2://unused

			break;
			case 0xE3://music noise sampling
				trackPos[3]++;
				if(drumSet<0xFE) drumSet=0xFE;
				if(drumSet==0xFF) drumSet=songFile.read();
			break;
			case 0xE4://force panning
				trackPos[3]++;
			break;
			case 0xE5://volume
				trackPos[3]++;
			break;
			case 0xE6://tone
				trackPos[3]++;
				trackPos[3]++;
			break;
			case 0xE7://unused

			break;
			case 0xE8://unused

			break;
			case 0xE9://global tempo
				trackPos[3]++;
				tempo+=(songFile.read()<<8);
				trackPos[3]++;
				tempo+=songFile.read();

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
				trackPos[3]++;
				songFile.read();
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
				trackPos[3]++;
				trackLoopTo[3]=songFile.read();
				trackPos[3]++;
				trackLoopTo[3]+=(songFile.read()<<8)-(songOffset+1);
				trackPos[3]=trackLoopTo[3];
			break;
			case 0xFD://loop
				trackPos[3]++;
				curCommand=songFile.read();
				if(!trackLooping[3]){
					trackLoopNumber[3]=curCommand;
					if(trackLoopNumber[3]>0) trackLooping[3]=true;//don't set if it's an infinite loop
				}
				trackPos[3]++;
				trackLoopTo[3]=songFile.read();
				trackPos[3]++;
				trackLoopTo[3]+=(songFile.read()<<8)-(songOffset+1);
				trackLoopNumber[3]--;
				if(!trackLooping[3]){//infinite loop
					trackPos[3]=trackLoopTo[3];
				}else if(trackLoopNumber[3]>0){
					trackPos[3]=trackLoopTo[3];
				}else{
					trackLooping[3]=false;
				}
			break;
			case 0xFE://call
				trackPos[3]++;
				trackLoopTo[3]=songFile.read();
				trackPos[3]++;
				trackLoopTo[3]+=(songFile.read()<<8)-(songOffset+1);
				trackRetPos[3]=trackPos[3];
				trackPos[3]=trackLoopTo[3];
			break;
			case 0xFF://return
				if(trackRetPos[3]>0){
					trackPos[3]=trackRetPos[3];
					trackRetPos[3]=0;
				}else{
					trackDone[3]=true;
					CHENL[3]=CHENR[3]=0;
				}
			break;
		}
	}
	trackPos[3]++;
}

void playerProcess(u8 channel){//main engine code
	while(trackNoteLength[channel]<=0) executeCommand(channel);
	if(NRxFreq[channel]>0){
		if(trackVibratoDepth[channel]>0){
			if(trackVibratoDelayTimer[channel]==0){
				if(trackVibratoTimer[channel]==0){
					trackVibratoTimer[channel]=trackVibratoSpeed[channel];
					trackVibratoState[channel]^=1;
					if(trackVibratoState[channel]==0){
						NRxFreq[channel]+=trackVibratoDepth[channel];
					}else if(trackVibratoState[channel]==1){
						NRxFreq[channel]-=trackVibratoDepth[channel];
					}else if(trackVibratoState[channel]==2){
						NRxFreq[channel]+=trackVibratoDepthAdd[channel];
						trackVibratoState[channel]=0;
					}else{
						NRxFreq[channel]-=trackVibratoDepthSub[channel];
						trackVibratoState[channel]=1;
					}
				}else{
					trackVibratoTimer[channel]--;
				}
			}else{
				trackVibratoDelayTimer[channel]--;
			}
		}
  		NRxFreq[channel]&=0x7FF;
  		CHFreq[channel]=getFreq(NRxFreq[channel]);
	}
	trackNoteLength[channel]-=0x100;
}

void playerProcessNSE(){//main engine code for noise channel
	while(trackNoteLength[3]<=0) executeCommandNSE();
	if(NRxFreq[3]<0xFFFF){
		if(drumTimer==0){
			if(drumData[drumIndex]<0xFF){
				drumTimer=(drumData[drumIndex++]&0xF)+1;
				trackEnvelope[2]=drumData[drumIndex++];
				writeNRx2(2,trackEnvelope[2]);
				NRxFreq[3]=drumData[drumIndex++];
			}else{
				NRxFreq[3]=0xFFFF;
			}
		}else{
			drumTimer--;
		}
		CHFreq[3]=getNSEFreq(NRxFreq[3]);
	}
	trackNoteLength[3]-=0x100;
}
