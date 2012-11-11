// TagExchange.cpp

#include "TagExchange.h"




// constructor
TagExchange::TagExchange(void)
{
	_floatHandler_callback = NULL;
	_longHandler_callback = NULL;
	_textHandler_callback = NULL;
	
	// init transmit buffer
	initTagTxBuffer();
	
//	Serial.println("TagExchange::TagExchange()");
}


void TagExchange::registerFloatHandler(TagUpdateFloatCallbackFunction callbackFunction)
{
	_floatHandler_callback = callbackFunction;
}

void TagExchange::registerLongHandler(TagUpdateLongCallbackFunction callbackFunction)
{
	_longHandler_callback = callbackFunction;
}

void TagExchange::registerTextHandler(TagUpdateTextCallbackFunction callbackFunction)
{
	_textHandler_callback = callbackFunction;
}


void TagExchange::initTagTxBuffer(void)
{
	_tagtxpacket.msg_id = msg_tag;
	_tagtxpacket.tagcount = 0;
	_tagtxpacket.timestamp = 0;
}


void TagExchange::processInboundPacket(Packet *packet)	// handle inbound message
{
	//Serial.println(F("TagExchange::processInboundPacket()"));

	switch(packet->msg_id) {
		case msg_tag: 
			// process tag message
			for (byte i = 0; i < packet->payload.TagData.tagcount; ++i) {
			  
			   int  tagid = packet->payload.TagData.data[i].tagid & 0x3fff;              // lower 14 bits determine the tagid;
			   TagType tagtype = (TagType)((packet->payload.TagData.data[i].tagid & 0xc000) >> 14);   // upper 2 bits determine the data type
											   
			   switch(tagtype) {              
				 case tagtype_float:
					if (_floatHandler_callback != NULL) {
						(*_floatHandler_callback)(tagid, packet->payload.TagData.timestamp, packet->payload.TagData.data[i].value.float_value);
					}                   
				   break;
					 
				 case tagtype_long:
					if (_longHandler_callback != NULL) {
						(*_longHandler_callback)(tagid, packet->payload.TagData.timestamp, packet->payload.TagData.data[i].value.long_value);
					}                 
				   break;
				   
				 case tagtype_text:
					if (_textHandler_callback != NULL) {
					(*_textHandler_callback)(tagid, packet->payload.TagData.timestamp, packet->payload.TagData.data[i].value.text_value);
					}
				   
			   }               
			}
		break;
		
		
		case msg_tagrequest:
			Serial.println(F("Unhandled message: msg_tagrequest"));
			break;
			
		case msg_taggroup:
			Serial.println(F("Unhandled message: msg_taggroup"));
			break;

		case msg_taggrouprequest:
			Serial.println(F("Unhandled message: msg_taggrouprequest"));
			break;
			
		default:
			Serial.println(F("Unhandled message: user defined"));
	}

}



void TagExchange::publishFloat(int tagid, float value)
{
	publishFloat(tagid, 0, value);
}

void TagExchange::publishFloat(int tagid, unsigned long timestamp, float value)
{
	// if the timestamp does not match what is already enqueued or if there is no free space in the packet: send data immediately
	if (timestamp != _tagtxpacket.timestamp || _tagtxpacket.tagcount >= MAX_TAGS_PER_RF12_MSG) {
		publishNow(true);
	}

	if (_tagtxpacket.tagcount == 0) {
		// initialise timestamp	
		_tagtxpacket.timestamp = timestamp;
		
		// set timeout value for the transmission delay
		_tagtxpacket_timeout = millis() + MAX_TRANSMIT_DELAY;
	}
	
	// enqueue the tagvalue
	_tagtxpacket.data[_tagtxpacket.tagcount].tagid = (tagid & 0x3fff) | (tagtype_float << 14);
    _tagtxpacket.data[_tagtxpacket.tagcount].value.float_value = value;
	_tagtxpacket.tagcount++;

#ifdef TAGXCH_DEBUG_TXBUFFER	
	Serial.println(F("DBG: ENQUEUE FLOAT"));
#endif
}

void TagExchange::publishLong(int tagid, long value)
{
	publishLong(tagid, 0, value);
}

void TagExchange::publishLong(int tagid, unsigned long timestamp, long value)
{
	// if the timestamp does not match what is already enqueued or if there is no free space in the packet: send data immediately
	if (timestamp != _tagtxpacket.timestamp || _tagtxpacket.tagcount >= MAX_TAGS_PER_RF12_MSG) {
		publishNow(true);
	}
	
	if (_tagtxpacket.tagcount == 0) {
		// initialise timestamp
		_tagtxpacket.timestamp = timestamp;

		// set timeout value for the transmission delay
		_tagtxpacket_timeout = millis() + MAX_TRANSMIT_DELAY;
	}
	
	// enqueue the tagvalue
	_tagtxpacket.data[_tagtxpacket.tagcount].tagid = (tagid & 0x3fff) | (tagtype_long << 14);
    _tagtxpacket.data[_tagtxpacket.tagcount].value.long_value = value;
	_tagtxpacket.tagcount++;
	
#ifdef TAGXCH_DEBUG_TXBUFFER	
	Serial.println(F("DBG: ENQUEUE LONG"));
#endif
}



		
// this functions results in immediate publishing of the tags enqueued. returns the number of tag values send.
// if the force parameter is set to true the function will block until the packet can be send, otherwise the 
// function will give up if the RF12M modem is busy.
#if 0
int TagExchange::publishNow(bool force)
{            
	int res = 0;

	Serial.println(F("TagExchange::publishNow()"));

	
//	if ((_tagtxpacket.tagcount > 0) & (force | rf12_canSend())) {
//		while (!rf12_canSend())
//			rf12_recvDone();
//		rf12_sendStart(0, &_tagtxpacket, 6 + (_tagtxpacket.tagcount * sizeof(TagData)));  //sizeof packet
//		rf12_sendWait(0);
//	  
//#ifdef TAGXCH_DEBUG_TXBUFFER		  
//		Serial.println("DBG: TRANSMIT BUFFER");
//#endif
//
  		res = _tagtxpacket.tagcount;
//	  
		// init transmit buffer after sending
		initTagTxBuffer();
//	}
	return res;
}
#endif







// constructor
TagExchangeRF12::TagExchangeRF12(void) : TagExchange()
{
	
//	Serial.println("TagExchangeRF12::TagExchangeRF12()");
}



void TagExchangeRF12::poll(void) 
{
    if (rf12_recvDone() && rf12_crc == 0) {
		if (rf12_len >= 1) {		
			processInboundPacket((Packet *)rf12_data);
        }
    }
		
	// send queued outgoing packets when the timeout value is reached
	if ((_tagtxpacket.tagcount > 0 && millis() >= _tagtxpacket_timeout) | (_tagtxpacket.tagcount >= MAX_TAGS_PER_RF12_MSG)) {
		publishNow(false);
	}
}

		
		
// this functions results in immediate publishing of the tags enqueued. returns the number of tag values send.
// if the force parameter is set to true the function will block until the packet can be send, otherwise the 
// function will give up if the RF12M modem is busy.
int TagExchangeRF12::publishNow(bool force)
{            
	int res = 0;
	
	if ((_tagtxpacket.tagcount > 0) & (force | rf12_canSend())) {
		while (!rf12_canSend())
			rf12_recvDone();
		rf12_sendStart(0, &_tagtxpacket, 6 + (_tagtxpacket.tagcount * sizeof(TagData)));  //sizeof packet
		rf12_sendWait(0);
	  
#ifdef TAGXCH_DEBUG_TXBUFFER		  
		Serial.println(F("DBG: TRANSMIT BUFFER"));
#endif

  		res = _tagtxpacket.tagcount;
	  
		// init transmit buffer after sending
		initTagTxBuffer();
	}
	return res;
}




// constructor
TagExchangeStream::TagExchangeStream(Stream* s) : TagExchange()
{
	_stream = s;
	
	
	// initialise serial reception 
	rx_state = idle;
	rx_len = 0;
	rx_bytes = 0;
	
	

//	Serial.println("TagExchangeStream::TagExchangeStream()");
}



/*
                         11111111112222222222333333333344444444445555555
              00123456789012345678901234567890123456789012345678901234560000000
   Test data:   $PKT:#12,1,2,0,0,0,0,A,0,B2,3,D6,4B,B,40,64,7,AC,1,X346\  
                V    V   V                                         V   V
	idle	====|    |	 |                                         |   |====
	header		=====|   |                                         |   |
	count		     ====|                                         |   |
	payload		         ==========================================|   |
	checksum	                                                   ====|
	end  --> idle		                                                            
	
*/
	
	
	
bool TagExchangeStream::isHexNumber(char c)
{
	return ((c>='0' && c<='9') || (c>='A' && c<='F') || (c>='a' && c<='f'));
}

void TagExchangeStream::parseHexNumber(char c)
{
	c=toupper(c);
	
	if (c>='0' && c<='9') {
		rx_value = (rx_value * 16) + (c-'0');
	} else 
	if (c>='A' && c<='F') {
		rx_value = (rx_value * 16) + 10 + (c-'A');		
	} else {
		rx_value = 0;
	}
}	
	

// handle data coming in over serial stream
bool TagExchangeStream::handleInput(char c)
{
	bool ret = false;
	rx_len++;

	switch (rx_state) {
		case idle:
			if (c=='$') {
				// start of our packet
				rx_len = 1;
				rx_bytes = 0;			
				rx_bytes_expected = 0;
				rx_checksum = 0;
				rx_value = 0;
				
				rx_state = header;				
				ret = true;

				//Serial.println(F("Input idle->header"));
			} else {
				rx_state = idle;
				ret = false;
			}
			break;
		
		case header:			
			if ((rx_len==2 && c=='P')
			  ||(rx_len==3 && c=='K')
			  ||(rx_len==4 && c=='T')
			  ||(rx_len==5 && c==':')) {
					rx_state = header;				
					ret = true;
					
					//Serial.print(F("Input in header len="));
					//Serial.println(rx_len);
			} else
			if (rx_len==6 && c=='#') {
				rx_state = count;
				ret = true;
				
				//Serial.println(F("Input header->count"));				
			} else {
				rx_state = idle;
				ret = false;
				
				//Serial.println(F("Input unexpected; header->idle"));
			}
			break;
		
		case count:
			if (isHexNumber(c)) {
				parseHexNumber(c);
				ret = true;
			} else 
			if (c==',') {
				// end if count detected; move on to the payload
				rx_bytes_expected = rx_value;
				rx_value = 0;	
				rx_state = payload;
				ret = true;
				
				//Serial.println(F("Input count->payload"));	
				//Serial.print(F("expected bytes = "));	
				//Serial.println(rx_bytes_expected);	
				
			} else {
				rx_state = idle;
				ret = false;
				
				//Serial.println(F("Input unexpected; count->idle"));	
			}
		
			break;

		case payload:
			if (isHexNumber(c)) {
				parseHexNumber(c);
				ret = true;
			} else 
			if (c==',') {
				// complete value for payload packet detected; process and move on next
				//rx_bytes_expected = rx_value;
				
				if (rx_bytes >= 0 && rx_bytes < MAX_SERIAL_BYTES && rx_bytes < rx_bytes_expected) {

					rx_buffer[rx_bytes] = rx_value;
					rx_checksum += rx_value;

				//	Serial.print(F("Payload byte received = "));	
				//	Serial.println(rx_value);	
				//	Serial.print(F("        buffer index  = "));	
				//	Serial.println(rx_bytes);	
				//	Serial.print(F("        calc checksum = "));	
				//	Serial.println(rx_checksum);	
									
					rx_value = 0;	
					rx_bytes++;
					ret = true;
				} else {
					// buffer overflow --> reject and go to idle state
					
					//Serial.print(F("TagExchangeStream::handleInput>> Incorrect payload length. rx_bytes="));
					//Serial.print(rx_bytes);
					//Serial.print(F(" expected="));
					//Serial.println(rx_bytes);
										
					rx_state = idle;
					ret = true;
				}
			} else 
			if (c=='X') {
				// start of checksum 
								
				rx_state = checksum;
				rx_value = 0;	
				ret = true;
				
				//Serial.println(F("Input payload"));	
				//Serial.print(F("expected bytes = "));	
				//Serial.println(rx_bytes_expected);					
				//Serial.print(F("received bytes = "));	
				//Serial.println(rx_bytes);	
				
			} else {
				rx_state = idle;
				ret = false;
				
				//Serial.println(F("Input unexpected; payload->idle"));	
			}		
			break;
		
		case checksum:
			if (isHexNumber(c)) {
				parseHexNumber(c);
				ret = true;
			} else 
			if (c == '\\') {
				// end of checksum

				//Serial.println(F("Input checksum"));	
				//Serial.print(F("received checksum   = "));	
				//Serial.println(rx_value);	
				//Serial.print(F("calculated checksum = "));	
				//Serial.println(rx_checksum);	

				if (rx_bytes == rx_bytes_expected && rx_value == rx_checksum) {
					// received payload matches header structure, number of bytes and checksum
				
					//Serial.println(F("invoking processInboundPacket()"));	
					_stream->println("OK");
					processInboundPacket((Packet *)&rx_buffer);
				}
				rx_state = idle;
				ret = true;
			} else {
				rx_state = idle;
				ret = false;
				
				//Serial.println(F("Input unexpected; checksum->idle"));	
			}
			
			break;
	}
	
	return ret;
}



void TagExchangeStream::poll(void) 
{
	// incoming data is handled via separate handleinput function
	
	
	// send queued outgoing packets when the timeout value is reached
	if ((_tagtxpacket.tagcount > 0 && millis() >= _tagtxpacket_timeout) | (_tagtxpacket.tagcount >= MAX_TAGS_PER_RF12_MSG)) {
		publishNow(false);
	}
}



// this functions results in immediate publishing of the tags enqueued. returns the number of tag values send.
// if the force parameter is set to true the function will block until the packet can be send, otherwise the 
// function will give up if the RF12M modem is busy.
int TagExchangeStream::publishNow(bool force)
{            
	int res = 0;
	
	// send data to the stream object
	_stream->print("$PKT");
	
	byte txsize = 6 + (_tagtxpacket.tagcount * sizeof(TagData));
	
	// #bytes in packet
	_stream->print(":#");	
	_stream->print(txsize,HEX);
	_stream->print(",");	
	
	
	// data packet
	byte* data = (byte*)(&_tagtxpacket);
	unsigned int csum = 0;
	
	for (byte i = 0; i < txsize; ++i) {
		byte b = *data++;
		_stream->print(b,HEX);
		_stream->print(",");	
		csum += b;
	}
	
	// add checksum code to detect if packet was received correctly
	// simple calculate a unsigned 16bit sum of all bytes in the data packet
	_stream->print("X");	
	_stream->print(csum,HEX);
	_stream->println("\\");	
		
	  
#ifdef TAGXCH_DEBUG_TXBUFFER		  
		Serial.println(F("DBG: TRANSMIT BUFFER"));
#endif

		res = _tagtxpacket.tagcount;
	  
		// init transmit buffer after sending
	initTagTxBuffer();

	return res;
}




// constructor
//TagExchangeHardwareSerial::TagExchangeHardwareSerial(HardwareSerial* s) : TagExchangeStream(s)
//{
//	_stream = s;
//
//
//	Serial.println("TagExchangeHardwareSerial::TagExchangeHardwareSerial()");
//	
//	
//}
//
//
//void TagExchangeHardwareSerial::poll(void) 
//{
//	// FIXME: handle incoming data
//	
//	
//	// send queued outgoing packets when the timeout value is reached
//	if ((_tagtxpacket.tagcount > 0 && millis() >= _tagtxpacket_timeout) | (_tagtxpacket.tagcount >= MAX_TAGS_PER_RF12_MSG)) {
//		publishNow(false);
//	}
//}


// this functions results in immediate publishing of the tags enqueued. returns the number of tag values send.
// if the force parameter is set to true the function will block until the packet can be send, otherwise the 
// function will give up if the RF12M modem is busy.
//int TagExchangeHardwareSerial::publishNow(bool force)
//{            
//	int res = 0;
//	
//	// send data to the stream object
//	
//	_stream->print(">>@HW-SERIAL:");
//	_stream->print("$PKT:");
//	
//	byte txsize = 6 + (_tagtxpacket.tagcount * sizeof(TagData));
//	byte* data = (byte*)(&_tagtxpacket);
//	unsigned int csum = 0;
//	
//	for (byte i = 0; i < txsize; ++i) {
//		byte b = *data++;
//		_stream->print(b,HEX);
//		_stream->print(",");	
//		csum += b;
//	}
//	
//	// add checksum code to detect if packet was received correctly
//	// simple calculate a unsigned 16bit sum of all bytes in the data packet
//	_stream->print("X:");	
//	_stream->print(csum,HEX);
//		
//	_stream->println(":END$");
//	
//	  
//#ifdef TAGXCH_DEBUG_TXBUFFER		  
//		Serial.println(F("DBG: TRANSMIT BUFFER"));
//#endif
//
//		res = _tagtxpacket.tagcount;
//	  
//		// init transmit buffer after sending
//	initTagTxBuffer();
//
//	return res;
//}

