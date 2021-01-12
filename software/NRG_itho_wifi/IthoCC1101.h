/*
 * Author: Klusjesman, modified bij supersjimmie for Arduino/ESP8266
 */

#ifndef __ITHOCC1101_H__
#define __ITHOCC1101_H__

#include <stdio.h>
#include "CC1101.h"
#include "IthoPacket.h"


//pa table settings
const uint8_t ithoPaTableSend[8] = {0x6F, 0x26, 0x2E, 0x8C, 0x87, 0xCD, 0xC7, 0xC0};
const uint8_t ithoPaTableReceive[8] = {0x6F, 0x26, 0x2E, 0x7F, 0x8A, 0x84, 0xCA, 0xC4};

//rft message 1 commands
const uint8_t ithoMessage1HighCommandBytes[] = {1,84,213,85,50,203,52};
const uint8_t ithoMessage1MediumCommandBytes[] = {1,84,213,85,74,213,52};
const uint8_t ithoMessage1LowCommandBytes[] = {1,84,213,85,83,83,84};	
const uint8_t ithoMessage1Timer1CommandBytes[] = {1,83,83,84,204,202,180};	
const uint8_t ithoMessage1Timer2CommandBytes[] = {1,83,83,83,53,52,180};		
const uint8_t ithoMessage1Timer3CommandBytes[] = {1,83,83,82,173,82,180};	
const uint8_t ithoMessage1JoinCommandBytes[] = {0,170,171,85,84,202,180};
const uint8_t ithoMessage1LeaveCommandBytes[] = {0,170,173,85,83,43,84};		

//duco message1 commands
const uint8_t ducoMessage1HighCommandBytes[] = {1,84,213,85,51,45,52};
const uint8_t ducoMessage1MediumCommandBytes[] = {1,84,213,85,75,51,52};
const uint8_t ducoMessage1LowCommandBytes[] = {1,84,213,85,82,181,84};
const uint8_t ducoMessage1StandByCommandBytes[] = {1,85,53,84,205,85,52};
const uint8_t ducoMessage1JoinCommandBytes[] = {0,170,171,85,85,44,180};
const uint8_t ducoMessage1LeaveCommandBytes[] = {0,170,173,85,82,205,84};

//message 2 commands
const uint8_t ithoMessage2PowerCommandBytes[] = {6,89,150,170,165,101,90,150,85,149,101,90,102,85,150};
const uint8_t ithoMessage2HighCommandBytes[] = {6,89,150,170,165,101,90,150,85,149,101,89,102,85,150};
const uint8_t ithoMessage2MediumCommandBytes[] = {6,89,150,170,165,101,90,150,85,149,101,90,150,85,150};
const uint8_t ithoMessage2LowCommandBytes[] = {6,89,150,170,165,101,90,150,85,149,101,89,150,85,150};
const uint8_t ithoMessage2StandByCommandBytes[] = {6,89,150,170,165,101,90,150,85,149,101,90,86,85,150};
const uint8_t ithoMessage2Timer1CommandBytes[] = {6,89,150,170,169,101,90,150,85,149,101,89,86,85,153};		//10 minutes full speed
const uint8_t ithoMessage2Timer2CommandBytes[] = {6,89,150,170,169,101,90,150,85,149,101,89,86,149,150};	//20 minutes full speed
const uint8_t ithoMessage2Timer3CommandBytes[] = {6,89,150,170,169,101,90,150,85,149,101,89,86,149,154};	//30 minutes full speed
const uint8_t ithoMessage2JoinCommandBytes[] = {9,90,170,90,165,165,89,106,85,149,102,89,150,170,165};
const uint8_t ithoMessage2LeaveCommandBytes[] = {9,90,170,90,165,165,89,166,85,149,105,90,170,90,165};

//message 2, counter
const uint8_t counterBytes24a[] = {1,2};
const uint8_t counterBytes24b[] = {84,148,100,164,88,152,104,168};
const uint8_t counterBytes25[] = {149,165,153,169,150,166,154,170};
const uint8_t counterBytes26[] = {96,160};
const uint8_t counterBytes41[] = {5, 10, 6, 9};
const uint8_t counterBytes42[] = {90, 170, 106, 154};
const uint8_t counterBytes43[] = {154, 90, 166, 102, 150, 86, 170, 106};
//join/leave
const uint8_t counterBytes64[] = {154,90,166,102,150,86,169,105,153,89,165,101,149,85,170,106};
const uint8_t counterBytes65[] = {150,169,153,165,149,170,154,166};
const uint8_t counterBytes66[] = {170,106};


//state machine
enum IthoReceiveStates {
	ExpectMessageStart,
	ExpectNormalCommand,
	ExpectJoinCommand,
	ExpectLeaveCommand
};



class IthoCC1101 : protected CC1101 {
	private:
		//receive
		IthoReceiveStates receiveState;											//state machine receive
		unsigned long lastMessage1Received;										//used for timeout detection
		CC1101Packet inMessage1;												//temp storage message1
		CC1101Packet inMessage2;												//temp storage message2
		IthoPacket inIthoPacket;												//stores last received message data
		
		//send
		IthoPacket outIthoPacket;												//stores state of "remote"

		//settings
		uint8_t sendTries;														//number of times a command is send at one button press
		
	//functions
	public:
		IthoCC1101(uint8_t counter = 0, uint8_t sendTries = 3);		//set initial counter value
		~IthoCC1101();
		
		//init
		void init() { CC1101::init(); }											//init,reset CC1101
		void initReceive();
		uint8_t getLastCounter() { return outIthoPacket.counter; }				//counter is increased before sending a command
		void setSendTries(uint8_t sendTries) { this->sendTries = sendTries; }
		
		//- deviceid should be a setting as well? random gen function? TODO
		
		//receive
		bool checkForNewPacket();												//check RX fifo for new data
		IthoPacket getLastPacket() { return inIthoPacket; }						//retrieve last received/parsed packet from remote
		IthoCommand getLastCommand() { return inIthoPacket.command; }						//retrieve last received/parsed command from remote
		uint8_t getLastInCounter() { return inIthoPacket.counter; }						//retrieve last received/parsed command from remote
		uint8_t ReadRSSI();
		bool checkID(const uint8_t *id);
		String getLastIDstr(bool ashex=true);
    int * getLastID();
		String getLastMessage2str(bool ashex=true);

				
		//send
		void sendCommand(IthoCommand command);
	protected:
	private:
		IthoCC1101( const IthoCC1101 &c);
		IthoCC1101& operator=( const IthoCC1101 &c);

		//init CC1101 for receiving
		void initReceiveMessage1();
		void initReceiveMessage2(IthoMessageType expectedMessageType);
		
		//init CC1101 for sending
		void initSendMessage1();
		void initSendMessage2(IthoCommand command);
		void finishTransfer();		
		
		//receive message validation
		bool isValidMessageStart();
		bool isValidMessageCommand();	
		bool isValidMessageJoin();
		bool isValidMessageLeave();
			
		//parse received message
		void parseReceivedPackets();
		void parseMessageStart();
		void parseMessageCommand();
		void parseMessageJoin();
		void parseMessageLeave();
		
		//send
		void createMessageStart(IthoPacket *itho, CC1101Packet *packet);
		void createMessageCommand(IthoPacket *itho, CC1101Packet *packet);
		void createMessageJoin(IthoPacket *itho, CC1101Packet *packet);
		void createMessageLeave(IthoPacket *itho, CC1101Packet *packet);
		uint8_t* getMessage1CommandBytes(IthoCommand command);
		uint8_t* getMessage2CommandBytes(IthoCommand command);
		
		//counter bytes calculation (send)
		uint8_t getMessage1Byte18(IthoCommand command);
		IthoCommand getMessage1PreviousCommand(uint8_t byte18);
		uint8_t calculateMessage2Byte24(uint8_t counter);
		uint8_t calculateMessage2Byte25(uint8_t counter);
		uint8_t calculateMessage2Byte26(uint8_t counter);
		uint8_t calculateMessage2Byte41(uint8_t counter, IthoCommand command);
		uint8_t calculateMessage2Byte42(uint8_t counter, IthoCommand command);
		uint8_t calculateMessage2Byte43(uint8_t counter, IthoCommand command);
		uint8_t calculateMessage2Byte49(uint8_t counter);
		uint8_t calculateMessage2Byte50(uint8_t counter);
		uint8_t calculateMessage2Byte51(uint8_t counter);
		uint8_t calculateMessage2Byte64(uint8_t counter);
		uint8_t calculateMessage2Byte65(uint8_t counter);
		uint8_t calculateMessage2Byte66(uint8_t counter);
		
		//counter calculation (receive)
		uint8_t calculateMessageCounter(uint8_t byte24, uint8_t byte25, uint8_t byte26);
		
		//general
		uint8_t getCounterIndex(const uint8_t *arr, uint8_t length, uint8_t value);		
		
		//test
		void testCreateMessage();

}; //IthoCC1101


extern volatile uint32_t data1[];

#endif //__ITHOCC1101_H__
