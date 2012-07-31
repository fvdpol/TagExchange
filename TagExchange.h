// TagExchange.h

#ifndef TAGEXCHANGE_h
#define TAGEXCHANGE_h

#include <JeeLib.h>

//Class TagExchange {

typedef enum {
  msg_tag             = 1,
  msg_tagrequest      = 2,
  msg_taggroup        = 3,
  msg_taggrouprequest = 4
} MsgType;

typedef struct {
  unsigned int tagid;
  union {
    float float_value;  	// type 0 - tagtype_float
    long  long_value;   	// type 1 - tagtype_long
	char  text_value[4];	// type 2 - tagtype_text
  } value; 
} TagData;

#define MAX_TAGS_PER_RF12_MSG	10

typedef enum {
  tagtype_float = 0,
  tagtype_long = 1,
  tagtype_text = 2
} TagType;

typedef struct {
  // header
  byte msg_id;      // identifier 
  
  byte tagcount;     // number of tags in packet
  unsigned long timestamp;	// seconds since 1970; or 0 if unset.
  
  // payload packet max size = 66 bytes
  // 2+4=6 bytes for the header; leaves 60 bytes for the tag data
  // 6 bytes per message: maximum 10 tags 
  TagData data[MAX_TAGS_PER_RF12_MSG];
} Packet_TagData;

//}




extern "C" {
// callback function types
    typedef void (*TagUpdateFloatCallbackFunction)(int tagid, unsigned long timestamp, float value);
    typedef void (*TagUpdateLongCallbackFunction)(int tagid, unsigned long timestamp, long value);
    typedef void (*TagUpdateTextCallbackFunction)(int tagid, unsigned long timestamp, char *value);
}


class TagExchangeRF12
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
		
		int publishNow(void);	// returns the number of tags transmitted


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
		
		
		
		Packet_TagData _tagtxpacket;	// buffer for transmitting tag updates
		unsigned long  _tagtxpacket_ts;	// timestamp of oldest message in buffer
	
};



#endif
