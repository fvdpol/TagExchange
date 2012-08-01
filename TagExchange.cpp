// TagExchange.cpp

#include "TagExchange.h"




// constructor
TagExchangeRF12::TagExchangeRF12(void)
{
	_floatHandler_callback = NULL;
	_longHandler_callback = NULL;
	_textHandler_callback = NULL;
	
	// init transmit buffer
	initTagTxBuffer();

}

void TagExchangeRF12::registerFloatHandler(TagUpdateFloatCallbackFunction callbackFunction)
{
	_floatHandler_callback = callbackFunction;
}

void TagExchangeRF12::registerLongHandler(TagUpdateLongCallbackFunction callbackFunction)
{
	_longHandler_callback = callbackFunction;
}

void TagExchangeRF12::registerTextHandler(TagUpdateTextCallbackFunction callbackFunction)
{
	_textHandler_callback = callbackFunction;
}



void TagExchangeRF12::poll(void) 
{

    if (rf12_recvDone() && rf12_crc == 0) {
        
		if (rf12_len >= 1) {
			switch(rf12_data[0]==1) {
				case msg_tag: {
					// process tag message
					Packet_TagData *packet;
				   
					 packet = (Packet_TagData*)rf12_data;

					 for (byte i = 0; i < packet->tagcount; ++i) {
					  
					   int  tagid = packet->data[i].tagid & 0x3fff;              // lower 14 bits determine the tagid;
					   TagType tagtype = (TagType)((packet->data[i].tagid & 0xc000) >> 14);   // upper 2 bits determine the data type
													   
					   switch(tagtype) {              
						 case tagtype_float:
						   if (_floatHandler_callback != NULL) {
								(*_floatHandler_callback)(tagid, packet->timestamp, packet->data[i].value.float_value);
							}                   
						   break;
							 
						 case tagtype_long:
						   if (_longHandler_callback != NULL) {
								(*_longHandler_callback)(tagid, packet->timestamp, packet->data[i].value.long_value);
							}                 
						   break;
						   
					   }               
					}
				}
				break;
				
				
				case msg_tagrequest:
					Serial.println("Unhandled message: msg_tagrequest");
					break;
					
				case msg_taggroup:
					Serial.println("Unhandled message: msg_taggroup");
					break;

				case msg_taggrouprequest:
					Serial.println("Unhandled message: msg_taggrouprequest");
					break;
					
				default:
					Serial.println("Unhandled message: user defined");
			}
        }
    }
	
	
	// send queued outgoing packets when the timeout value is reached
	if (_tagtxpacket.tagcount > 0 && millis() >= _tagtxpacket_timeout) {
		publishNow();
	}
	
}



void TagExchangeRF12::publishFloat(int tagid, float value)
{
	publishFloat(tagid, 0, value);
}

void TagExchangeRF12::publishFloat(int tagid, unsigned long timestamp, float value)
{
	// if the timestamp does not match what is already enqueued or if there is no free space in the packet: send data immediately
	if (timestamp != _tagtxpacket.timestamp || _tagtxpacket.tagcount >= MAX_TAGS_PER_RF12_MSG) {
		publishNow();
	}
	
	if (_tagtxpacket.tagcount == 0) {
		_tagtxpacket.timestamp = timestamp;
		
		// set timeout value for the transmission delay
		_tagtxpacket_timeout = millis() + MAX_TRANSMIT_DELAY;
	}
	
	// enqueue the tagvalue
	_tagtxpacket.data[_tagtxpacket.tagcount].tagid = (tagid & 0x3fff) | (tagtype_float << 14);
    _tagtxpacket.data[_tagtxpacket.tagcount].value.float_value = value;
	_tagtxpacket.tagcount++;

#ifdef TAGXCH_DEBUG_TXBUFFER	
	Serial.println("DBG: ENQUEUE FLOAT");
#endif
}

void TagExchangeRF12::publishLong(int tagid, long value)
{
	publishLong(tagid, 0, value);
}

void TagExchangeRF12::publishLong(int tagid, unsigned long timestamp, long value)
{
	// if the timestamp does not match what is already enqueued or if there is no free space in the packet: send data immediately
	if (timestamp != _tagtxpacket.timestamp || _tagtxpacket.tagcount >= MAX_TAGS_PER_RF12_MSG) {
		publishNow();
	}
	
	if (_tagtxpacket.tagcount == 0) {
		_tagtxpacket.timestamp = timestamp;

		// set timeout value for the transmission delay
		_tagtxpacket_timeout = millis() + MAX_TRANSMIT_DELAY;
	}
	
	// enqueue the tagvalue
	_tagtxpacket.data[_tagtxpacket.tagcount].tagid = (tagid & 0x3fff) | (tagtype_long << 14);
    _tagtxpacket.data[_tagtxpacket.tagcount].value.long_value = value;
	_tagtxpacket.tagcount++;
	
#ifdef TAGXCH_DEBUG_TXBUFFER	
	Serial.println("DBG: ENQUEUE LONG");
#endif
}
		
		
// this functions results in immediate publishing of the tags enqueued. returns the number of tag values send.
int TagExchangeRF12::publishNow(void)
{            
	int res = 0;
	if (_tagtxpacket.tagcount > 0) {
		while (!rf12_canSend())
			rf12_recvDone();
		rf12_sendStart(0, &_tagtxpacket, 6 + (_tagtxpacket.tagcount * sizeof(TagData)));  //sizeof packet
		rf12_sendWait(0);
	  
#ifdef TAGXCH_DEBUG_TXBUFFER		  
		Serial.println("DBG: TRANSMIT BUFFER");
#endif

  		res = _tagtxpacket.tagcount;
	  
		// init transmit buffer after sending
		initTagTxBuffer();
	}
	return res;
}


void TagExchangeRF12::initTagTxBuffer(void)
{
	_tagtxpacket.msg_id = msg_tag;
	_tagtxpacket.tagcount = 0;
	_tagtxpacket.timestamp = 0;
}

