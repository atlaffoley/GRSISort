

#include "TDataParser.h"
#include "TChannel.h"
#include "Globals.h"

#include "TFragmentQueue.h"
#include "TGRSIStats.h"


TDataParser *TDataParser::fDataParser = 0;
bool TDataParser::no_waveforms = false;
bool TDataParser::record_stats = false;

const unsigned long TDataParser::fgMaxTriggerId = 1024 * 1024 * 16; // 24 bits internally
unsigned long TDataParser::fgLastMidasId	=	0;

unsigned long TDataParser::fgLastTriggerId	=	0;


ClassImp(TDataParser)

TDataParser *TDataParser::instance()	{
	if(!fDataParser)
		fDataParser = new TDataParser();
	return fDataParser;
}

TDataParser::TDataParser()	{

}

TDataParser::~TDataParser()	{	}


//std::vector<TFragment*> TDataParser::TigressDataToFragment(uint32_t *data, int size,unsigned int midasserialnumber, time_t midastime) {
int TDataParser::TigressDataToFragment(uint32_t *data, int size,unsigned int midasserialnumber, time_t midastime) {

   std::vector<TFragment*> FragsFound;
   int NumFragsFound = 0;
   TFragment *EventFrag = new TFragment();
  //FragsFound.push_back(EventFrag);
   //EventFrag->Clear();	
	bool badtimestamp = false;
	bool needtodeletelast = false;
   EventFrag->MidasTimeStamp = midastime;
	EventFrag->MidasId = midasserialnumber;	 
   
   int x = 0;
   uint32_t dword = *(data+x);
   uint32_t type = (dword & 0xf0000000)>>28;
   uint32_t value = (dword & 0x0fffffff);

   if(!SetTIGTriggerID(dword,EventFrag)) {
      delete EventFrag;
      printf(RED "Setting TriggerId (0x%08x) falied on midas event: " DYELLOW "%i" RESET_COLOR "\n",dword,midasserialnumber);
      return NumFragsFound;  
   }
   x+=1;
   if(!SetTIGTimeStamp((data+x),EventFrag)) { 
      delete EventFrag;
      printf(RED "Setting TimeStamp falied on midas event: " DYELLOW "%i" RESET_COLOR "\n",midasserialnumber);
      return NumFragsFound;
   }
   int temp_charge =  0;
   int temp_cfd    =  0;
   int temp_led    =  0;
   for(;x<size;x++) {
      dword = *(data+x);
      type = (dword & 0xf0000000)>>28;
      value = (dword & 0x0fffffff);
      switch(type) {
         case 0x0: // raw wave forms.
            if(!no_waveforms)
               SetTIGWave(value,EventFrag);
            break;
         case 0x1: // trapizodal wave forms.
            break;
         case 0x4: // cfd values.  This also ends the the fragment!
            SetTIGCfd(value,EventFrag);
            SetTIGCharge(temp_charge,EventFrag);
            SetTIGLed(temp_led,EventFrag);
            ///check whether the fragment is 'good'
            //FragsFound.push_back(EventFrag);
            TFragmentQueue::GetQueue("GOOD")->Add(EventFrag);
            NumFragsFound++;
            if( (*(data+x+1) & 0xf0000000) != 0xe0000000) {
               TFragment *transferfrag = EventFrag;
               EventFrag = new TFragment;
               EventFrag->MidasTimeStamp = transferfrag->MidasTimeStamp;
               EventFrag->MidasId        = transferfrag->MidasId;       	 
               EventFrag->TriggerId      = transferfrag->TriggerId;     
               EventFrag->TimeStampLow   = transferfrag->TimeStampLow;  
               EventFrag->TimeStampHigh  = transferfrag->TimeStampHigh; 
            }
            else
               EventFrag = 0;
            break;
         case 0x5: // raw charge evaluation.
            //SetTIGCharge(value,EventFrag);
            temp_charge = value;
            break;
         case 0x6:
            //SetTIGLed(value,EventFrag);
            temp_led = value;
            break;
         case 0xb:
            //SetTIGBitPattern
            break;
         case 0xc:
            SetTIGAddress(value,EventFrag);
            break;
         case 0xe: // this ends the bank!
            if(EventFrag)
               delete EventFrag;
            break;
         case 0xf:
            break;
         default:
            break;
      }
   }
   return NumFragsFound;
}

void TDataParser::SetTIGAddress(uint32_t value,TFragment *currentfrag) {
	currentfrag->ChannelAddress = (int32_t)(0x00ffffff&value);   ///the front end number is not in the tig odb!
   return;
}

void TDataParser::SetTIGWave(uint32_t value,TFragment *currentfrag) {
   //if(!currentfrag->wavebuffer)
   //   currentfrag->wavebuffer = new std::vector<short>;
   if(currentfrag->wavebuffer.size() > (100000) ) {printf("number of wave samples found is to great\n"); return;}       
   if (value & 0x00002000) {
      int temp =  value & 0x00003fff;
      temp = ~temp;
      temp = (temp & 0x00001fff) + 1;
      currentfrag->wavebuffer.push_back((int16_t)-temp);	//eventfragment->SamplesFound++;
   } else {
      currentfrag->wavebuffer.push_back((int16_t)(value & 0x00001fff)); //eventfragment->SamplesFound++;
   }
   if ((value >> 14) & 0x00002000) {
      int temp =  (value >> 14) & 0x00003fff;
      temp = ~temp;
      temp = (temp & 0x00001fff) + 1;
      currentfrag->wavebuffer.push_back((int16_t)-temp);	//eventfragment->SamplesFound++;
   } else {
      currentfrag->wavebuffer.push_back((int16_t)((value >> 14) & 0x00001fff) );	//eventfragment->SamplesFound++;
   }
   return;
}

void TDataParser::SetTIGCfd(uint32_t value,TFragment *currentfrag) {
   //currentfragment->SlowRiseTime = value & 0x08000000;
   currentfrag->Cfd.push_back( int32_t(value & 0x07ffffff));
   //std::string dig_type = "Tig64";
   std::string dig_type = (TChannel::GetChannel(currentfrag->ChannelAddress))->GetDigitizerType();

   // remove vernier for now and calculate the time to the trigger
   int32_t tsBits;
   int32_t cfdBits;
   if ( dig_type.compare(0,5,"Tig10")==0) {
      cfdBits = (currentfrag->Cfd.back() >> 4);
      tsBits  = currentfrag->TimeStampLow & 0x007fffff;
      // probably should check that there hasn't been any wrap around here
      currentfrag->TimeToTrig = tsBits - cfdBits;
   } else if ( dig_type.compare(0,5,"Tig64")==0 ) {
      currentfrag->TimeToTrig = (currentfrag->Cfd.back() >> 5);
      // cfdBits	= (eventfragment->Cfd >> 5);
      // tsBits  = eventfragment->TimeStampLow & 0x003fffff;
   } else {
      //printf(DYELLOW "Address: 0x%08x | " RESET_COLOR); (TChannel::GetChannel(currentfrag->ChannelAddress))->Print();
      //printf("CFD obtained without knowing digitizer type with midas Id = %d!\n",currentfrag->MidasId );
   }
   return;
}

void TDataParser::SetTIGLed(uint32_t value, TFragment *currentfrag) {
   currentfrag->Led.push_back( int32_t(value & 0x07ffffff) );
   return;
}

void TDataParser::SetTIGCharge(uint32_t value, TFragment *currentfragment) {
   TChannel *chan = TChannel::GetChannel(currentfragment->ChannelAddress);
   std::string dig_type = chan->GetDigitizerType();
   currentfragment->ChannelNumber = chan->GetNumber();

   //chan->Print();
   //printf("Channel NUmber = %i!!!!!!!!!!!\n",chan->GetNumber());
//   printf("Channel Address = 0x%08x\n",currentfragment->ChannelAddress);
//   chan->Print();
   //std::string dig_type = "Tig64";`

   //if( strncmp(eventfragment->DigitizerType.c_str(),"tig10",5) == 0)	{
   if( dig_type.compare(0,5,"Tig10") == 0)	{
      //currentfragment->PileUp = (value &  0x04000000);
      //currentcurrentfragment->Charge	= (value &	0x03ffffff);
      if(value & 0x02000000)	{
         currentfragment->Charge.push_back( -( (~((int32_t)value & 0x01ffffff)) & 0x01ffffff)+1);
		}
	}
	else if( dig_type.compare(0,5,"Tig64") == 0) {
		currentfragment->Charge.push_back((value &	0x003fffff));
		if(value & 0x00200000)	{
			currentfragment->Charge.push_back( -( (~((int32_t)value & 0x001fffff)) & 0x001fffff)+1);
		}
	}
	else{ //printf("%i  problem extracting charge, digitizer not set(?)\n", error++); IsBad = true;}
		//printf("%i  problem extracting charge, digitizer not set(?)\n", error++);  // IsBad = true;
		//eventfragment->DigitizerType = "unknown";
		currentfragment->Charge.push_back( ((int32_t)value &	0x03ffffff));
		if(value & 0x02000000)	{
			currentfragment->Charge.push_back( -( (~((int32_t)value & 0x01ffffff)) & 0x01ffffff)+1);
		}

	}

}

bool TDataParser::SetTIGTriggerID(uint32_t value, TFragment *currentfrag) {
   if( (value&0xf0000000) != 0x80000000) {
      return false;         
   }
	value = value & 0x0fffffff;   
   unsigned int LastTriggerIdHiBits = fgLastTriggerId & 0xFF000000; // highest 8 bits, remainder will be 
   unsigned int LastTriggerIdLoBits = fgLastTriggerId & 0x00FFFFFF;   // determined by the reported value
   if ( value < fgMaxTriggerId / 10 ) {  // the trigger id has wrapped around	
      if ( LastTriggerIdLoBits > fgMaxTriggerId * 9 / 10 ) {
         currentfrag->TriggerId = (uint64_t)(LastTriggerIdHiBits + value + fgMaxTriggerId);
         printf(DBLUE "We are looping new trigger id = %lu, last trigger hi bits = %d," 
                      " last trigger lo bits = %d, value = %d, 				midas = %d" 
                       RESET_COLOR "\n", currentfrag->TriggerId, LastTriggerIdHiBits, 
                       LastTriggerIdLoBits, value, 0);//	midasserialnumber);				
      } else {
         currentfrag->TriggerId = (uint64_t)(LastTriggerIdHiBits + value);
      }
   } else if ( value < fgMaxTriggerId * 9 / 10 ) {
      currentfrag->TriggerId = (uint64_t)(LastTriggerIdHiBits + value);
   } else {
      if ( LastTriggerIdLoBits < fgMaxTriggerId / 10 ) {
         currentfrag->TriggerId = (uint64_t)(LastTriggerIdHiBits + value - fgMaxTriggerId);
         printf(DRED "We are backwards looping new trigger id = %lu, last trigger hi bits = %d,"
                     " last trigger lo bits = %d, value = %d, midas = %d" 
                      RESET_COLOR "\n", currentfrag->TriggerId, LastTriggerIdHiBits, 
                      LastTriggerIdLoBits, value, 0);//midasserialnumber);
      } else {
         currentfrag->TriggerId = (uint64_t)(LastTriggerIdHiBits + value);			
      }
   }	
   //fragment_id_map[value]++;
   //currentfrag->FragmentId = fragment_id_map[value];
   fgLastTriggerId = (unsigned long)currentfrag->TriggerId;
   return true; 
}


bool TDataParser::SetTIGTimeStamp(uint32_t *data,TFragment *currentfrag ) {

   for(int x=0;x<10;x++) {
      data = data + 1;
      //printf(DBLUE "data for x = %i  |  0x%08x  |  0x%08x  |  0x%08x" RESET_COLOR "\n",x,*data,(*data)>>28,0xa);
      //if((*(data)&0xf0000000) == 0xa0000000){
      //if(((*data)&0xf0000000) == 0xa0000000){
      if(((*data)>>28)==0xa) {
         //printf(DRED "data for x = %i |  0x%08x" RESET_COLOR "\n",x,*data);
         break;
      }
   }
   if(!((*data&0xf0000000) == 0xa0000000)) { 
      printf("here 0?\t0x%08x\n",*data);
      return false;
   }
   //printf("data = 0x%08x\n",*data);

   int time[5];
   int x = 0;
   
	time[0] = *(data + x);  //printf("time[0] = 0x%08x\n",time[0]);
	x += 1;
	time[1] =	*(data + x);	//& 0x0fffffff;
   //printf("time[1] = 0x%08x\n",time[1]);
   if( (time[1] & 0xf0000000) != 0xa0000000) {
      printf("here 1?\tx = %i\t0x%08x\n",x,time[1]);
      return false;
	   //break;
	} 
   x+=1;
   time[2] = *(data +x);
   if((time[2] & 0xf0000000) != 0xa0000000)   
          // this is ok, it always happens for tig64s.
     
   currentfrag->TimeStampLow = time[0] & 0x00ffffff;
	currentfrag->TimeStampHigh = time[1] & 0x00ffffff;
   //currentfrag->SetTimeStamp();
   return true;
   
   
   x+=1;
   time[3] = *(data +x);
   if((time[3] & 0xf0000000) != 0xa0000000) {  
      printf("here 2?\n");
      return false;
   }
   x+=1;
   time[4] = *(data +x);
   if((time[4] & 0xf0000000) != 0xa0000000) {  
      printf("here 3?\n");
      return false;
   }
   
   return true;
}





/////////////***************************************************************/////////////
/////////////***************************************************************/////////////
/////////////***************************************************************/////////////
/////////////***************************************************************/////////////
/////////////***************************************************************/////////////
/////////////***************************************************************/////////////

int TDataParser::GriffinDataToFragment(uint32_t *data, int size, unsigned int midasserialnumber, time_t midastime)	{

   int NumFragsFound = 1;
   TFragment *EventFrag = new TFragment();

   EventFrag->MidasTimeStamp = midastime;
	EventFrag->MidasId = midasserialnumber;	 

	int x = 0;  
 
	if(!SetGRIFHeader(data[x++],EventFrag)) {
		printf(DYELLOW "data[0] = 0x%08x" RESET_COLOR "\n",data[0]);
		delete EventFrag;
		return -1;	
	}

	if(!SetGRIFPPG(data[x++],EventFrag)) {
		delete EventFrag;
		return -2;;
	}

	if(!SetGRIFMasterFilterId(data[x++],EventFrag)) {
		delete EventFrag;
		return -3;
	}

	if(!SetGRIFMasterFilterPattern(data[x++],EventFrag)) {
		delete EventFrag;
		return -4;
	}


	if(!SetGRIFChannelTriggerId(data[x++],EventFrag)) {
		delete EventFrag;
		return -5;
	}

	if(!SetGRIFTimeStampLow(data[x++],EventFrag)) {
		delete EventFrag;
		return -6;
	}

	int  kwordcounter = 0;
	for(;x<size;x++) {
   	uint32_t dword  = *(data+x);
		uint32_t packet = dword & 0xf0000000;
		uint32_t value  = dword & 0x0fffffff; 
		switch(packet) {
			case 0xc0000000:
		            if(!no_waveforms)
		 		SetGRIFWaveForm(value,EventFrag);
		     		break;
			case 0xb0000000:
				SetGRIFDeadTime(value,EventFrag);
				break;
			case 0xe0000000:
				//if(value == EventFrag->TimeStampLow) {
				if(value == EventFrag->ChannelId) {
					if(record_stats)
						FillStats(EventFrag);
					TFragmentQueue::GetQueue("GOOD")->Add(EventFrag);				
					return NumFragsFound;
				} else 
					return -(x+1);
	    		break;
 		   default:				
	      	if((packet & 0x80000000) == 0x00000000) {
					EventFrag->KValue.push_back( (*(data+x) & 0x7c000000) >> 21 );
					EventFrag->Charge.push_back((*(data+x) & 0x03ffffff));	
					x = x + 1;
 					EventFrag->KValue.back() |= (*(data+x) & 0x7c000000) >> 26;
					EventFrag->Cfd.push_back( (*(data+x) & 0x03ffffff));
	          }
   	       break;
		};
	}
	return -(0x0fffffff);
}





bool TDataParser::SetGRIFHeader(uint32_t value,TFragment *frag) {
	if( (value&0xf0000000) != 0x80000000) {
		return false;
	}
	frag->NumberOfFilters =  (value &0x0f000000)>> 24;
        frag->DataType        =  (value &0x00e00000)>> 21;
        frag->NumberOfPileups =  (value &0x001c0000)>> 18;
	frag->ChannelAddress  =  (value &0x0003fff0)>> 4;
        frag->DetectorType    =  (value &0x0000000f);
	return true;
};


bool TDataParser::SetGRIFPPG(uint32_t value,TFragment *frag) {
	if( (value & 0xc0000000) != 0x00000000) {
		return false;
	}
	frag->PPG = value & 0x3fffffff;
	return true;
};

bool TDataParser::SetGRIFMasterFilterId(uint32_t value,TFragment *frag) {
	if( (value &0x80000000) != 0x00000000) {
		return false;
	}
	frag->TriggerId = value & 0x7fffffff;
	return true;
}


bool TDataParser::SetGRIFMasterFilterPattern(uint32_t value, TFragment *frag) {
	if( (value &0xc0000000) != 0x00000000) {
		return false;
	}
	frag->TriggerBitPattern = value & 0x3fffffff;
	return true;
}


bool TDataParser::SetGRIFChannelTriggerId(uint32_t value, TFragment *frag) {
	if( (value &0xf0000000) != 0x90000000) {
		return false;
	}
	frag->ChannelId = value & 0x0fffffff;
	return true;
}


bool TDataParser::SetGRIFTimeStampLow(uint32_t value, TFragment *frag) {
	if( (value &0xf0000000) != 0xa0000000) {
		return false;
	}
	frag->TimeStampLow = value & 0x0fffffff;
	return true;
}


bool TDataParser::SetGRIFWaveForm(uint32_t value,TFragment *currentfrag) {

   if(currentfrag->wavebuffer.size() > (100000) ) {printf("number of wave samples found is to great\n"); return false;}       
   if (value & 0x00002000) {
      int temp =  value & 0x00003fff;
      temp = ~temp;
      temp = (temp & 0x00001fff) + 1;
      currentfrag->wavebuffer.push_back((int16_t)-temp);	//eventfragment->SamplesFound++;
   } else {
      currentfrag->wavebuffer.push_back((int16_t)(value & 0x00001fff)); //eventfragment->SamplesFound++;
   }
   if ((value >> 14) & 0x00002000) {
      int temp =  (value >> 14) & 0x00003fff;
      temp = ~temp;
      temp = (temp & 0x00001fff) + 1;
      currentfrag->wavebuffer.push_back((int16_t)-temp);	//eventfragment->SamplesFound++;
   } else {
      currentfrag->wavebuffer.push_back((int16_t)((value >> 14) & 0x00001fff) );	//eventfragment->SamplesFound++;
   }
   return true;;
}



bool TDataParser::SetGRIFDeadTime(uint32_t value, TFragment *frag) {
	frag->DeadTime      = (value & 0x0fffc000) >> 14;
	frag->TimeStampHigh = (value &  0x00003fff);
	return true;
}



void TDataParser::FillStats(TFragment *frag) {
	TGRSIStats *stat = TGRSIStats::GetStats(frag->ChannelAddress);
	//printf("Filling stats: 0x%08x\n",stat);
	stat->IncDeadTime(frag->DeadTime);
	if( (frag->MidasTimeStamp < TGRSIStats::GetLowestMidasTimeStamp()) ||
	    (TGRSIStats::GetLowestMidasTimeStamp() == 0)) {
		TGRSIStats::SetLowestMidasTimeStamp(frag->MidasTimeStamp);
	} else if (frag->MidasTimeStamp > TGRSIStats::GetHighestMidasTimeStamp()) {
		TGRSIStats::SetHighestMidasTimeStamp(frag->MidasTimeStamp);
	}
	return;
}









