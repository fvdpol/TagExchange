// TagExchange.h

#ifndef TAGEXCHANGE_h
#define TAGEXCHANGE_h

#include <JeeLib.h>

// maximum number of tags that can be stored in a packet transmitted over the RF12 network
#define MAX_TAGS_PER_RF12_MSG	10

// maximum transission delay; defines how many ms the system will wait for additional tags 
// to group in the same packet.
#define MAX_TRANSMIT_DELAY		200

//#define TAGXCH_DEBUG_TXBUFFER

class TagExchange {
	public:
	
		typedef enum {
			msg_tag             = 1,
			msg_tagrequest      = 2,
			msg_taggroup        = 3,
			msg_taggrouprequest = 4
		} MsgType;

		typedef enum {
			tagtype_float = 0,
			tagtype_long = 1,
			tagtype_text = 2
		} TagType;

		typedef struct {
			unsigned int tagid;
			union {
				float float_value;  	// type 0 - tagtype_float
				long  long_value;   	// type 1 - tagtype_long
				char  text_value[4];	// type 2 - tagtype_text
			} value; 
		} TagData;
	

		typedef struct {
			// header
			byte msg_id;      // identifier 
  
			byte tagcount;     // number of tags in packet
			unsigned long timestamp;	// seconds since 1970; or 0 if unset.
  
			// payload packet max size = 66 bytes
			// 2+4=6 bytes for the header; leaves 60 bytes for the tag data
			// 6 bytes per message: maximum 10 tags 
			TagData data[MAX_TAGS_PER_RF12_MSG];		// FIXME -- max number of messages should be dynamic to allow different packet sizes 
														// for serial/ethernet?
														// 2nd thought... maybe not; 2nd alternative is to pack multiple of these packets in
														// a single ethernet packet. MTU can be different in function of AVR memory (328p vs. mega)
														// >> can we convert to a malloc'ed array; where is size defined in class invocation?
		} Packet_TagData;


	private:

};




extern "C" {
// callback function types
    typedef void (*TagUpdateFloatCallbackFunction)(int tagid, unsigned long timestamp, float value);
    typedef void (*TagUpdateLongCallbackFunction)(int tagid, unsigned long timestamp, long value);
    typedef void (*TagUpdateTextCallbackFunction)(int tagid, unsigned long timestamp, char *value);
}


class TagExchangeRF12 : public TagExchange
{
	public:
		// constructor
		TagExchangeRF12();
		
		
		// call this often to keep the tag exchange of rf12 handling going
		void poll(void);
		
		void registerFloatHandler(TagUpdateFloatCallbackFunction);
		void registerLongHandler(TagUpdateLongCallbackFunction);
		void registerTextHandler(TagUpdateTextCallbackFunction);
		
		
		void publishFloat(int tagid, float value);
		void publishFloat(int tagid, unsigned long timestamp, float value);

		void publishLong(int tagid, long value);
		void publishLong(int tagid, unsigned long timestamp, long value);
		
		int publishNow(bool force=true);	// returns the number of tags transmitted


// when first tag is published; store in buffer
// add additional tags
// ==> until max buffer size reached OR maximum transmit delay reached (ms time since first msg)
//
// if tags are send *with* timestamp; break messages if timestamp timestamp differs from the timestamp of first message


	private:
		// callback handlers
		TagUpdateFloatCallbackFunction	_floatHandler_callback;
		TagUpdateLongCallbackFunction	_longHandler_callback;
		TagUpdateTextCallbackFunction	_textHandler_callback;
				
		void initTagTxBuffer(void);
	
		// private data
		Packet_TagData _tagtxpacket;			// buffer for transmitting tag updates
		unsigned long  _tagtxpacket_timeout;	// timeout value for the transmission delay; send when this ms value is reached
	
};



#endif
