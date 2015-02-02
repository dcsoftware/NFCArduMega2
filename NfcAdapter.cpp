/**************************************************************************/
/*!
 @file     emulatetag.cpp
 @author   Armin Wieser
 @license  BSD
 */
/**************************************************************************/

#include "MPN532_debug.h"
#include "MNdefMessage.h"
#include <string.h>
//#include <sha1.h>
//#include <TOTP.h>
#include <stdlib.h>
#include "NfcAdapter.h"

#define MAX_TGREAD

typedef enum { NONE, CC, NDEF} tag_file;   // CC ... Compatibility Container

typedef enum {S_DISCONNECTED, S_CONNECTED} SerialState;

SerialState serialState;
CardState cardState;
Event eventType = NOTHING;


String userId;
String userCredit;
String controlKeyReceived;
char amountBuf[6];
char timestampBuf[19];
uint8_t secretKey[] = "ABCDEFGHIJ"; //{0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a};
uint8_t controlKey[] = "";
uint8_t secret[20];
int intCount = 0;
long epoch = 0;
long transactionId = 10000000;
int led = 7;
int led1 = 6;
int led2 = 5;
char currentDate[8];
boolean keyIsUpdated = false;
String inputCommand = "";         // a string to hold incoming data
String inputValue = "";
String inputValue2 = "";
String otpIn;
String timestamp = "";
boolean commandComplete = false;  // whether the string is complete
boolean valueIn = false;
boolean valueIn2 = false;
boolean serialInt = false;
boolean request = false;
boolean loggedin = false;
boolean validIp = false;
boolean connectedToBackend = false;
boolean keyRequest = false;

char* otpReceived = "";
char otp[10];
char cardId[] = "00005678";
char cardName[] = "Macchina Prova 1";

//TOTP totp = TOTP(secretKey, 10);


/*void verifyOtpCode(String input) {
    char* newCode;
    char code[10];
    char prevCode[10];
    char nextCode[10];
    
    String a = input.substring(0, input.length() - 5);
    char buff[11];
    a.concat('0');
    a.toCharArray(buff, sizeof(buff));
    buff[sizeof(buff) - 1] = 0;
    epoch = atol(buff);
    //Serial.print("log: timestamp lenght = ");
    //Serial.print(a.length());
    //Serial.print(" - timestep: ");
    //Serial.print(timestep);
    //Serial.println(" ;");
    
    
    newCode = totp.getCode(epoch);
    strcpy(code, newCode);
    newCode = totp.getCode(epoch - 10);
    strcpy(prevCode, newCode);
    newCode = totp.getCode(epoch + 10);
    strcpy(nextCode, newCode);

    
    /*int f = strcmp(code, otp);
    Serial.print("log: generated otp = ");
    Serial.print(prevCode);
    Serial.print(" ");
    Serial.print(code);
    Serial.print(" ");
    Serial.print(nextCode);
    Serial.print(" ");
    Serial.print(" received otp = ");
    Serial.print(otp);
    Serial.print(" - compare = ");
    Serial.print(f);
    Serial.println(" ;");*/
    
    /*delay(50);
    
    
    
    if(strcmp(prevCode, otp) == 0) {
        cardState = AUTHENTICATED;
        //Serial.println("log: AUTHENTICATION OK con prevCode;");
    } else if(strcmp(code, otp) == 0) {
        cardState = AUTHENTICATED;
        //Serial.println("log: AUTHENTICATION OK con code;");
    } else if(strcmp(nextCode, otp) == 0) {
        cardState = AUTHENTICATED;
        //Serial.println("log: AUTHENTICATION OK con nextCode;");
    } else {
        cardState = ERROR_AUTH;
        //Serial.println("log: AUTHENTICATION ERROR;");
    }

    timestamp = input;
}*/

void convertValue(String amount, String time) {
	//String amount = inputS.substring(0, 5); //inputS.length() - 1);
	//String time = inputS.substring(6);
    //int z = inputS.indexOf('-');
    timestamp = time;
    amount.toCharArray(amountBuf, sizeof(amountBuf));
    amountBuf[sizeof(amountBuf) - 1] = 0;
    time.concat(String(transactionId));
    time.toCharArray(timestampBuf, sizeof(timestampBuf));
    timestampBuf[sizeof(timestampBuf) - 1] = 0;

    /*Serial.print("log:inputS=");
    Serial.print(timestampBuf);
    Serial.println(';');*/

}

void setCurrentDate(String input){

}

boolean readCommand() {
    boolean serialRead = false;
    if(Serial.available() > 0) {

        while(Serial.available() > 0) {
            // get the new byte:
            char inChar = (char)Serial.read();
            // add it to the inputString:
            if(!valueIn) {
                inputCommand += inChar;
            } else {
            	inputValue += inChar;
            }
            // if the incoming character is a newline, set a flag
            // so the main loop can do something about it:
            if (inChar == ':') {
                valueIn = true;
            }
            if (inChar == ';') {
                commandComplete = true;
                break;
            }
        }
    }
    if (commandComplete) {
        //Serial.println("log:" + inputCommand + "-" + inputValue);
        if(inputCommand.equals(SERIAL_COMMAND_CONNECTION)) {
            if(inputValue.equals(SERIAL_RESPONSE_OK)) {
                serialState = S_CONNECTED;
                digitalWrite(led, HIGH);
            }
        } else if (inputCommand.equals("set_data:")) {
            if(inputValue.equals(SERIAL_RESPONSE_OK)) {
                
            }
        } else if (inputCommand.equals(SERIAL_COMMAND_PURCHASE)) {
            //digitalWrite(led1, HIGH);
            //Serial.println(inputCommand.concat(SERIAL_RESPONSE_OK));
        	Serial.println("log: command purchase;");
        	transactionId++;
        	convertValue(inputValue.substring(0, 5), inputValue.substring(6, 16));
            cardState = PURCHASE;
            
        } else if (inputCommand.equals(SERIAL_COMMAND_RECHARGE)) {
            //digitalWrite(led1, HIGH);
            //Serial.println(inputCommand.concat(SERIAL_RESPONSE_OK));
        	Serial.println("log: command recharge;");
        	transactionId++;
        	convertValue(inputValue.substring(0, 5), inputValue.substring(6, 16));
            cardState = RECHARGE;
        } else if (inputCommand.equals(SERIAL_COMMAND_GET_TIME)) {
            
            //verifyOtpCode(inputValue);

        } else if (inputCommand.equals(SERIAL_COMMAND_GET_DATE)) {
        	setCurrentDate(inputValue);
        }
        inputCommand = "";
        inputValue = "";
        inputValue2 = "";
        valueIn = false;
        valueIn2 = false;
        commandComplete = false;
        serialRead = true;
    }
    return serialRead;
}

bool MyCard::init(){
    pn532.begin();
    return pn532.SAMConfig();
}

//boolean MyCard::initWifi(WizFi250 &shield) {
//	_wifishield = &shield;
//	boolean result = false;
//	Serial.println("log:connecting to wifi;");
//
//	if(_wifishield->join(SSID0, KEY0, AUTH)) {
//		Serial.println("log:connected to wifi;");
//		delay(50);
//		do{
//
//			validIp = _wifishield->ipSearch("www.google.com");
//		}while(!validIp);
//
//		return true;
//		/*if(_wifishield->ipSearch("www.google.com")) {
//			Serial.print("log:connecting to app engine ");
//			Serial.print((char*)_wifishield->m_peerIPAddr);
//			Serial.println(";");
//			if(_wifishield->connect((char*)_wifishield->m_peerIPAddr,TSN_REMOTE_PORT)) {
//				Serial.println("log:connected to app engine servlet;");
//				_wifishield->clear();
//				return true;
//			} else {
//				Serial.println("log:wifi error 1;");
//				return false;
//			}
//		} else {
//			Serial.println("log:wifi error 2;");
//			return false;
//		}*//*
//	} else {
//		Serial.println("log:wifi error 3;");
//		return false;
//	}
//}

//boolean MyCard::backendConnection() {
//	Serial.print("log:connecting to app engine ");
//	Serial.print((char*)_wifishield->m_peerIPAddr);
//	Serial.println(";");
//	do{
//
//		connectedToBackend = _wifishield->connect((char*)_wifishield->m_peerIPAddr,TSN_REMOTE_PORT);
//		delay(100);
//	}while(!connectedToBackend);
//
//	Serial.println("log:connected to app engine servlet;");
//	//_wifishield->clear();
//	return true;
//	/*if(_wifishield->connect((char*)_wifishield->m_peerIPAddr,TSN_REMOTE_PORT)) {
//		Serial.println("log:connected to app engine servlet;");
//		_wifishield->clear();
//		return true;
//	} else {
//		Serial.println("log:wifi error;");
//		return false;
//	}*/
//}

//void MyCard::getSecureKey() {
//	Serial.println("log: get secure key;");
//
//	if(!keyIsUpdated) {
//		while(!connectedToBackend) {
//			backendConnection();
//		}
//		Serial1.print("GET /gettodaykey HTTP/1.1\r\n");
//		Serial1.print("Host:winged-standard-741.appspot.com\r\n\r\n");
//		keyRequest = true;
//		checkSerial();
//
//		Serial1.print("+++");
//		delay(1200);
//		_wifishield->closeAllSockets();
//		totp = TOTP(secret, 20);
//	}
//}

void MyCard::setId(char id[]) {
	//memcpy(id, cardId, 8);
}

void MyCard::setNdefFile(const uint8_t* ndef, const int16_t ndefLength){
    if(ndefLength >  (NDEF_MAX_LENGTH -2)){
        DMSG("ndef file too large (> NDEF_MAX_LENGHT -2) - aborting");
        return;
    }
    
    ndef_file[0] = ndefLength >> 8;
    ndef_file[1] = ndefLength & 0xFF;
    memcpy(ndef_file+2, ndef, ndefLength);
}

void MyCard::setUid(uint8_t* uid){
    uidPtr = uid;
}

bool MyCard::emulate(const uint16_t tgInitAsTargetTimeout){
	eventType = NOTHING;
	connectedToBackend = false;
    userCredit = "0.0";
    userId = "";
    loggedin = false;
    
    const uint8_t ndef_tag_application_name_v2[] = {0, 0x7, 0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01 };
    const uint8_t ndef_tag_application_name_priv[] = {0, 0x7, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34};
    
    uint8_t command[] = {
        PN532_COMMAND_TGINITASTARGET,
        5,                  // MODE: PICC only, Passive only
        
        0x04, 0x00,         // SENS_RES
        0x00, 0x00, 0x00,   // NFCID1
        0x20,               // SEL_RES
        
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,   // FeliCaParams
        0,0,
        
        0,0,0,0,0,0,0,0,0,0, // NFCID3t
        
        0, // length of general bytes
        0  // length of historical bytes
    };
    
    
    if(uidPtr != 0){  // if uid is set copy 3 bytes to nfcid1
        memcpy(command + 4, uidPtr, 3);
    }
    
    if(1 != pn532.tgInitAsTarget(command,sizeof(command), tgInitAsTargetTimeout)){
        DMSG("log:tgInitAsTarget failed or timed out!;");
        Serial.println("log: init as target timeout;");
        return false;
    }
    Serial.println("log:target inizializzato;");
    
    uint8_t base_capability_container[] = {
        0, 0x0F,    //CC length
        0x20,       //Mapping Version ---> version 2.0
        0, 0x54,    //Max data read
        0, 0xFF,    //Max data write
        0x04,       // T
        0x06,       // L
        0xE1, 0x04, // File identifier
        ((NDEF_MAX_LENGTH & 0xFF00) >> 8), (NDEF_MAX_LENGTH & 0xFF), // maximum NDEF file size
        0x00,       // read access 0x0 = granted
        0x00        // write access 0x0 = granted | 0xFF = deny
    };
    
    if(tagWriteable == false){
        base_capability_container[14] = 0xFF;
    }
    
    tagWrittenByInitiator = false;
    
    
    uint8_t rwbuf[128];
    uint8_t sendlen;
    int16_t status;
    tag_file currentFile = NONE;
    uint16_t cc_size = sizeof(base_capability_container);
    bool runLoop = true;
    
    while(runLoop){
        status = pn532.tgGetData(rwbuf, sizeof(rwbuf));
        if(status < 0){
            DMSG("tgGetData timed out\n");
            //pn532.inRelease();
            //return -1;
        }
        
        /*uint32_t field = pn532.getGeneralStatus();

        Serial.print("log: ");Serial.print((field>>24) & 0xFF, HEX);
        Serial.print(", ");Serial.print((field>>16) & 0xFF, DEC);
        Serial.print(", ");Serial.print((field>>8) & 0xFF, DEC);
        Serial.print(", ");Serial.print(field  & 0xFF, DEC);Serial.println(";");*/

        uint8_t p1 = rwbuf[C_APDU_P1];
        uint8_t p2 = rwbuf[C_APDU_P2];
        uint8_t lc = rwbuf[C_APDU_LC];
        uint16_t p1p2_length = ((int16_t) p1 << 8) + p2;

        
        switch(rwbuf[C_APDU_INS]){
            case SELECT_FILE:
                switch(p1){
                    case C_APDU_P1_SELECT_BY_ID:
                        if(p2 != 0x0c){
                            DMSG("C_APDU_P2 != 0x0c\n");
                            setResponse(COMMAND_COMPLETE, rwbuf, &sendlen);
                        } else if(lc == 2 && rwbuf[C_APDU_DATA] == 0xE1 && (rwbuf[C_APDU_DATA+1] == 0x03 || rwbuf[C_APDU_DATA+1] == 0x04)){
                            setResponse(COMMAND_COMPLETE, rwbuf, &sendlen);
                            if(rwbuf[C_APDU_DATA+1] == 0x03){
                                currentFile = CC;
                            } else if(rwbuf[C_APDU_DATA+1] == 0x04){
                                currentFile = NDEF;
                            }
                        } else {
                            setResponse(TAG_NOT_FOUND, rwbuf, &sendlen);
                        }
                        break;
                    case C_APDU_P1_SELECT_BY_NAME:
                        if(0 == memcmp(ndef_tag_application_name_v2, rwbuf + C_APDU_P2, sizeof(ndef_tag_application_name_v2))){
                            cardState = CONNECTED;
                            setResponse(COMMAND_COMPLETE, rwbuf, &sendlen);
                        } else if (0 == memcmp(ndef_tag_application_name_priv, rwbuf + C_APDU_P2, sizeof(ndef_tag_application_name_priv))){
                            DMSG("\nOK");
                            Serial.println("connection:req;");
                            while (!readCommand()) {
                                delay(10);
                            }
                            setResponse(PRIV_APPLICATION_SELECTED, rwbuf, &sendlen);
                            
                        } else {
                            DMSG("function not supported\n");
                            setResponse(FUNCTION_NOT_SUPPORTED, rwbuf, &sendlen);
                        }
                        break;
                }
                break;
            case READ_BINARY:
                switch(currentFile){
                    case NONE:
                        setResponse(TAG_NOT_FOUND, rwbuf, &sendlen);
                        break;
                    case CC:
                        if( p1p2_length > NDEF_MAX_LENGTH){
                            setResponse(END_OF_FILE_BEFORE_REACHED_LE_BYTES, rwbuf, &sendlen);
                        }else {
                            memcpy(rwbuf,base_capability_container + p1p2_length, lc);
                            setResponse(COMMAND_COMPLETE, rwbuf + lc, &sendlen, lc);
                        }
                        break;
                    case NDEF:
                        if( p1p2_length > NDEF_MAX_LENGTH){
                            setResponse(END_OF_FILE_BEFORE_REACHED_LE_BYTES, rwbuf, &sendlen);
                        }else {
                            memcpy(rwbuf, ndef_file + p1p2_length, lc);
                            setResponse(COMMAND_COMPLETE, rwbuf + lc, &sendlen, lc);
                        }
                        break;
                }
                break;
            /*case AUTHENTICATE:
                if((p1 == 0x00) && (p2 == 0x00)) {
                    //DMSG("\nAuthenticating... ");
                    for (int i = 0; i <= lc; i++) {
                        //DMSG_HEX(rwbuf[C_APDU_DATA + i]);
                        otp[i] = rwbuf[C_APDU_DATA + i];
                        //DMSG_WRT(rwbuf[C_APDU_DATA + i]);
                    }

                    Serial.println("get_time:req;");
                    while (!readCommand()) {
                        delay(10);
                    }
                    
                    switch (cardState) {
                        case AUTHENTICATED:
                            setResponse(COMMAND_COMPLETE, rwbuf, &sendlen);
                            break;
                        case ERROR_AUTH:
                            setResponse(AUTH_ERROR, rwbuf, &sendlen);
                            break;
                    }
                    
                    
                }
                break;*/
            case LOG_IN:
                if((p1 == 0x00) && (p2 == 0x00)) {
                    DMSG("\nLoggin in... ");
                    char string[20];
                    for (int i = 0; i <= lc; i++) {
                        string[i] = (char)rwbuf[C_APDU_DATA + i];
                        //Serial.write(rwbuf[C_APDU_DATA + i]);
                    }
                    string[lc] = '\0';
                    String s = string;
                    int x = s.indexOf(',');
                    int z = s.lastIndexOf(',');
                    int y = s.indexOf(';');
                    controlKeyReceived = s.substring(0, z - 1);
                    userId = s.substring(z + 1, x -1);
                    userCredit = s.substring(x + 1, y);
                    /*Serial.print("log:");
                    Serial.print(userCredit);
                    Serial.println(";");*/
                    Serial.print("set_data:");
                    Serial.print(userCredit);
                    Serial.println(";");
                    delay(100);

                    while (!readCommand()) {
                        delay(10);
                    }

                    Serial.print("log: control key=");
                    Serial.print(controlKeyReceived);
                    Serial.println(";");
                    cardState = WAITING;
                    setResponse(COMMAND_COMPLETE, rwbuf, &sendlen);
                    if(!loggedin) {
                        //eventType = LOGIN;
                    	Serial.println("log: login;");
                        //sendRequest(LOGIN);
                        loggedin = true;
                    }

                }
                break;
            case READING_STATUS:
                if((p1 == 0x00) && (p2 == 0x00)) {
                    
                    readCommand();
                    
                    //waitingSerial();
                    switch (cardState) {
                        case WAITING:
                            cardState = WAITING;
                            Serial.println("log: status WAITING;");
                            setResponse(STATUS_WAITING, rwbuf, &sendlen);
                            eventType = NOTHING;
                            break;
                        case RECHARGE:
                            cardState = WAITING;
                            Serial.println("log: status RECHARGED;");
                            setResponse(STATUS_RECHARGED, rwbuf, &sendlen);
                            //sendRequest(RECHARGE_TRANSACTION);
                            //eventType = RECHARGE_TRANSACTION;
                            break;
                        case PURCHASE:
                            cardState = WAITING;
                            Serial.println("log: status PURCHASE;");
                            setResponse(STATUS_PURCHASE, rwbuf, &sendlen);
                            //sendRequest(PURCHASE_TRANSACTION);
                            //eventType = PURCHASE_TRANSACTION;
                            break;
                    }
                    delay(50);
                    
                }
                break;
            default:
                DMSG("Command not supported!");
                DMSG_HEX(rwbuf[C_APDU_INS]);
                DMSG("\n");
                runLoop = false;
                Serial.println("log:command not supported;");
                setResponse(FUNCTION_NOT_SUPPORTED, rwbuf, &sendlen);
                break;
        }
        Serial.println("log:uscito da switch;");
        status = pn532.tgSetData(rwbuf, sendlen);
        if(status == 0){
            DMSG("tgSetData failed\n!");
            DMSG("\n In Release 1");
            Serial.println("log:set data failed in release;");
            pn532.inRelease();
            break;
        }
        //checkSerial();
        //sendRequest(eventType);
    }
    Serial.println("log:uscito da while;");
    DMSG("\nIn Release 2");
    Serial.println("log:in release;");
    pn532.inRelease();
    return true;
}


void MyCard::setResponse(responseCommand cmd, uint8_t* buf, uint8_t* sendlen, uint8_t sendlenOffset){
    switch(cmd){
        case COMMAND_COMPLETE:
            buf[0] = R_APDU_SW1_COMMAND_COMPLETE;
            buf[1] = R_APDU_SW2_COMMAND_COMPLETE;
            *sendlen = 2 + sendlenOffset;
            break;
        case TAG_NOT_FOUND:
            buf[0] = R_APDU_SW1_NDEF_TAG_NOT_FOUND;
            buf[1] = R_APDU_SW2_NDEF_TAG_NOT_FOUND;
            *sendlen = 2;
            break;
        case FUNCTION_NOT_SUPPORTED:
            buf[0] = R_APDU_SW1_FUNCTION_NOT_SUPPORTED;
            buf[1] = R_APDU_SW2_FUNCTION_NOT_SUPPORTED;
            *sendlen = 2;
            break;
        case MEMORY_FAILURE:
            buf[0] = R_APDU_SW1_MEMORY_FAILURE;
            buf[1] = R_APDU_SW2_MEMORY_FAILURE;
            *sendlen = 2;
            break;
        case END_OF_FILE_BEFORE_REACHED_LE_BYTES:
            buf[0] = R_APDU_SW1_END_OF_FILE_BEFORE_REACHED_LE_BYTES;
            buf[1] = R_APDU_SW2_END_OF_FILE_BEFORE_REACHED_LE_BYTES;
            *sendlen= 2;
            break;
        case PRIV_APPLICATION_SELECTED:
            buf[0] = R_SW1_PRIV_APP_SELECTED;
            buf[1] = R_SW2_PRIV_APP_SELECTED;
            for (int y = 0; y < sizeof(cardId); y++) {
                buf[y + 2] = (uint8_t)cardId[y];
            }
            buf[10] = (uint8_t)',';
            for (int z = 0; z < sizeof(secretKey); z++) {
            	buf[z + 2 + 9] = (uint8_t)secretKey[z];
            }
            *sendlen = 2 + sizeof(cardId) + sizeof(secretKey)- 1;
        	break;
        case STATUS_WAITING:
            buf[0] = R_SW1_STATUS_WAITING;
            buf[1] = R_SW2_STATUS_WAITING;
            *sendlen= 2;
            break;
        case STATUS_RECHARGED:
            buf[0] = R_SW1_STATUS_RECHARGED;
            buf[1] = R_SW2_STATUS_RECHARGED;
            for (int y = 0; y < sizeof(amountBuf); y++) {
                buf[y + 2] = (uint8_t)amountBuf[y];
            }
            buf[7] = (uint8_t)',';
            for (int z = 0; z < sizeof(timestampBuf); z++) {
            	buf[z + 2 + 6] = (uint8_t)timestampBuf[z];
            }
            *sendlen= 2 + (sizeof(amountBuf) + sizeof(timestampBuf)- 1);
            break;
        case STATUS_PURCHASE:
            buf[0] = R_SW1_STATUS_PURCHASE;
            buf[1] = R_SW2_STATUS_PURCHASE;
            for (int y = 0; y < sizeof(amountBuf); y++) {
                buf[y + 2] = (uint8_t)amountBuf[y];
            }
            buf[7] = (uint8_t)',';
            for (int z = 0; z < sizeof(timestampBuf); z++) {
            	buf[z + 2 + 6] = (uint8_t)timestampBuf[z];
            }
            *sendlen= 2 + (sizeof(amountBuf) + sizeof(timestampBuf)- 1);
            break;
        case STATUS_DATA_UPDATED:
            buf[0] = R_SW1_STATUS_DATA_UPDATED;
            buf[1] = R_SW2_STATUS_DATA_UPDATED;
            *sendlen= 2;
            break;
        case AUTH_ERROR:
            buf[0] = R_SW1_ERROR_AUTH;
            buf[1] = R_SW2_ERROR_AUTH;
            *sendlen= 2;
            break;
    }

}

//void MyCard::checkSerial() {
//	//Serial.println("log: check serial;");
//	boolean available = false;
//	/*int i = 0;
//	boolean serialAvailable = false;
//	//char m[512];
//	String msg = "";
//	char c;
//
//	//delay(200);
//
//	if(Serial1.available() > 0) {
//		Serial.println("log: serial available ");
//
//
//		while(Serial1.available() > 0) {
//			c = (char)Serial1.read();
//			if((c == ';')  || (c == '\n') || (c == '\r') || (c == '\r\n') || (c == ':')) {
//				//m[i] = ' ';
//				Serial.print("|");
//			} else {
//				//m[i] = (char)c;
//				Serial.print(c);
//			}
//			i++;
//			serialAvailable = true;
//		}
//
//		Serial.println(";");
//
//	}*/
//	delay(500);
//	int h;
//	do {
//		if (_wifishield->available() > 0) {
//			available = true;
//			h = _wifishield->available();
//		}
//	}while(!available);
//
//	//Serial.print("log: serial ");
//	//Serial.print(h);
//
//	char c;
//	char m[256];
//	uint8_t i = 0;
//	while(_wifishield->available() > 0) {
//		c = (char)_wifishield->read();
//		if((c == ';')  || (c == '\n') || (c == '\r') || (c == '\r\n') || (c == ':')) {
//			m[i] = ' ';
//			//Serial.print(" ");
//		} else {
//			m[i] = c;
//			//Serial.print(c);
//		}
//		i++;
//	}
//
//	//Serial.print(m);
//
//	//Serial.println(";");
//
//	delay(200);
//
//	if(strstr(m, "200 OK") > 0) {
//		Serial.println("log: 200 OK;");
//		if(keyRequest) {
//			char * p;
//			char * q;
//			p = strstr(m, "date");
//			q = strstr(m, "key");
//
//			memcpy(secret, q + 4, 20);
//			Serial.print("log: key ");
//			//Serial.print(secret);
//			Serial.println(";");
//			memcpy(currentDate, p+5, 8);
//			/*Serial.print("log: date ");
//			Serial.print(currentDate);
//			Serial.println(";");*/
//		}
//		return;
//	} else if(strstr(m, "202 Accepted") > 0) {
//		Serial.println("log: 202 Accepted;");
//		return;
//	} else if(strstr(m, "500") > 0) {
//		Serial.println("log: Internal Server Error;");
//		return;
//	}
//
//}

//void MyCard::sendRequest(Event event) {
//
//	String content;
//	int contentLength;
//	String machineId = cardId;
//	boolean available = false;
//	switch(event) {
//		case NOTHING:
//			break;
//		case LOGIN:
//			while(!connectedToBackend) {
//				backendConnection();
//			}
//			_wifishield->clear();
//			content = "event=0&machine_id=";
//			//content += machineId;
//			//content += "&user_id=";
//			//content += userId;
//			//content += "&timestamp=";
//			//content += timestamp;
//			contentLength = content.length();
//			Serial1.print("POST /eventlogger HTTP/1.1\r\n");
//			Serial1.print("Host:winged-standard-741.appspot.com\r\n");
//			Serial1.print("Content-Type: application/x-www-form-urlencoded\r\n");
//			Serial1.print("Content-Length:");
//			Serial1.print(contentLength);
//			Serial1.print("\r\n\r\n");
//			Serial1.print(content);
//			Serial1.print("\r\n\r\n");
//			Serial.print("log: ");
//			Serial.print(content);
//			Serial.println(";");
//			checkSerial();
//			Serial.println("log: logged in;");
//			break;
//		case LOGOUT:
//			break;
//		case PURCHASE_TRANSACTION:
//			content = "event=1&transaction_id=";
//			content += machineId;
//			content += transactionId;
//			content += "&user_id=";
//			content += userId;
//			content += "&amount=-";
//			content += amountBuf;
//			content += "&timestamp=";
//			content += timestamp;
//			contentLength = content.length();
//			Serial1.print("POST /eventlogger HTTP/1.1\r\n");
//			Serial1.print("Host:winged-standard-741.appspot.com\r\n");
//			Serial1.print("Content-Type: application/x-www-form-urlencoded\r\n");
//			Serial1.print("Content-Length:");
//			Serial1.print(contentLength);
//			Serial1.print("\r\n\r\n");
//			Serial1.print(content);
//			Serial1.print("\r\n\r\n");
//			checkSerial();
//			break;
//		case RECHARGE_TRANSACTION:
//			content = "event=1&transaction_id=";
//			content += machineId;
//			content += transactionId;
//			content += "&user_id=";
//			content += userId;
//			content += "&amount=+";
//			content += amountBuf;
//			content += "&timestamp=";
//			content += timestamp;
//			contentLength = content.length();
//			Serial1.print("POST /eventlogger HTTP/1.1\r\n");
//			Serial1.print("Host:winged-standard-741.appspot.com\r\n");
//			Serial1.print("Content-Type: application/x-www-form-urlencoded\r\n");
//			Serial1.print("Content-Length:");
//			Serial1.print(contentLength);
//			Serial1.print("\r\n\r\n");
//			Serial1.print(content);
//			Serial1.print("\r\n\r\n");
//			checkSerial();
//			break;
//		default:
//			break;
//	}
//}

