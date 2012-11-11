// TagExchange.h

#ifndef TAGEXCHANGE_h
#define TAGEXCHANGE_h

#include <JeeLib.h>

// maximum number of tags that can be stored in a packet transmitted over the RF12 network
#define MAX_TAGS_PER_RF12_MSG	10


// maximum number of bytes to handle by serial reception buffer
#define MAX_SERIAL_BYTES		80

// maximum transission delay; defines how many ms the system will wait for additional tags 
// to group in the same packet.
#define MAX_TRANSMIT_DELAY		200

//#define TAGXCH_DEBUG_TXBUFFER

extern "C" {
// callback function types
    typedef void (*TagUpdateFloatCallbackFunction)(int tagid, unsigned long timestamp, float value);
    typedef void (*TagUpdateLongCallbackFunction)(int tagid, unsigned long timestamp, long value);
    typedef void (*TagUpdateTextCallbackFunction)(int tagid, unsigned long timestamp, char *value);
}


class TagExchange {
	public:
		// data types
	
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
			byte tagcount;     // number of tags in packet
			unsigned long timestamp;	// seconds since 1970; or 0 if unset.
										// typical use is real-time messaging between nodes
										// in this case the timestamp is not set.
										// in case message will be buffered for extended amount of time
										// the timestamp should be used to keep track of time of the messages
  
			// payload packet max size = 66 bytes
			// 2+4=6 bytes for the header; leaves 60 bytes for the tag data
			// 6 bytes per message: maximum 10 tags 
			TagData data[MAX_TAGS_PER_RF12_MSG];		// FIXME -- max number of messages should be dynamic to allow different packet sizes 
														// for serial/ethernet?
														// 2nd thought... maybe not; 2nd alternative is to pack multiple of these packets in
														// a single ethernet packet. MTU can be different in function of AVR memory (328p vs. mega)
														// >> can we convert to a malloc'ed array; where is size defined in class invocation?														
		} TagDataMsg;

		
		typedef struct {
			byte data[5 + (6*MAX_TAGS_PER_RF12_MSG)];
		} GenericMsg;
		
		
		typedef struct {
			// header
			byte msg_id;      // identifier 
			
			union {
				GenericMsg	Generic;
				TagDataMsg	TagData;
			} payload;
		} Packet;
	
	
	
		typedef struct {
			// header
			byte msg_id;      // identifier 
  
			byte tagcount;     // number of tags in packet
			unsigned long timestamp;	// seconds since 1970; or 0 if unset.
										// typical use is real-time messaging between nodes
										// in this case the timestamp is not set.
										// in case message will be buffered for extended amount of time
										// the timestamp should be used to keep track of time of the messages
  
			// payload packet max size = 66 bytes
			// 2+4=6 bytes for the header; leaves 60 bytes for the tag data
			// 6 bytes per message: maximum 10 tags 
			TagData data[MAX_TAGS_PER_RF12_MSG];		// FIXME -- max number of messages should be dynamic to allow different packet sizes 
														// for serial/ethernet?
														// 2nd thought... maybe not; 2nd alternative is to pack multiple of these packets in
														// a single ethernet packet. MTU can be different in function of AVR memory (328p vs. mega)
														// >> can we convert to a malloc'ed array; where is size defined in class invocation?														
		} Packet_TagData;

//		union {
//		    Packet_Header		Header;	     // identifier 
//			Packet_TagData		TagData;
//		} Packet_t;
		
		
		// constructor
		TagExchange();
	
		
		void registerFloatHandler(TagUpdateFloatCallbackFunction);
		void registerLongHandler(TagUpdateLongCallbackFunction);
		void registerTextHandler(TagUpdateTextCallbackFunction);

		void publishFloat(int tagid, float value);
		void publishFloat(int tagid, unsigned long timestamp, float value);

		void publishLong(int tagid, long value);
		void publishLong(int tagid, unsigned long timestamp, long value);
		
		virtual int publishNow(bool force=true) =0;	// returns the number of tags transmitted
	

	protected:

		// callback handlers
		TagUpdateFloatCallbackFunction	_floatHandler_callback;
		TagUpdateLongCallbackFunction	_longHandler_callback;
		TagUpdateTextCallbackFunction	_textHandler_callback;
		
		void initTagTxBuffer(void);	
		void processInboundPacket(Packet *packet);	// handle inbound message
	

		Packet_TagData _tagtxpacket;			// buffer for transmitting tag updates
		unsigned long  _tagtxpacket_timeout;	// timeout value for the transmission delay; send when this ms value is reached

};



class TagExchangeRF12 : public TagExchange
{
	public:
		// constructor
		TagExchangeRF12();
		
		
		// call this often to keep the tag exchange of rf12 handling going
		void poll(void);
		
		int publishNow(bool force=true);	// returns the number of tags transmitted

	protected:
};



class TagExchangeStream : public TagExchange
{
	public:
		// constructor 
		// mandatory argument is the Serial port we want to use
		TagExchangeStream(Stream* s);
		
		void poll(void);
		
		int publishNow(bool force=true);	// returns the number of tags transmitted

		bool handleInput(char c);			// handle data coming in over serial stream
											// returns true of the input was handled, ie. it was recognised as valid serialised packet
		
	protected:
	
		bool isHexNumber(char c);	// return if c is any of "01234567890abcdefABCDEF"
		void parseHexNumber(char c);		// parse hexadecimal number from input stream
	
	
		Stream* _stream;
		
		
		// 
		
		typedef enum {
			idle		= 1,
			header		= 2,
			count		= 3,
			payload		= 4,
			checksum	= 5
		} RxState;
		
		
		
		byte rx_buffer[MAX_SERIAL_BYTES];
		RxState  rx_state;			// state for input parser
		unsigned int rx_value;				
		int  rx_len;				// position in stream (0=not in stream; 1=1st character etc.
		int  rx_bytes;				// number of bytes received
		int  rx_bytes_expected;		// number of bytes expected (according to length indicator in
		unsigned int rx_checksum;	// checksum
		
	
	
		};



// not needed as we can use the Stream class for serial communications
//
//class TagExchangeHardwareSerial : public TagExchangeStream
//{
//	public:
//		// constructor 
//		// mandatory argument is the Serial port we want to use
//		TagExchangeHardwareSerial(HardwareSerial* s);
//		
//		void poll(void);
//		
//		int publishNow(bool force=true);	// returns the number of tags transmitted
//
//		
//	protected:
//		HardwareSerial* _stream;		
//};





#endif
