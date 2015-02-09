#include <SPI.h>
#include "MPN532_SPI.h"
#include "MNdefMessage.h"
#include <EEPROMex.h>
#include "NfcAdapter.h"

#define SERIAL_COMMAND_CONNECTION "connection:"
#define SERIAL_COMMAND_RECHARGE "recharge:"
#define SERIAL_COMMAND_PURCHASE "purchase:"
#define SERIAL_COMMAND_SET_DATA "set_data:"
#define SERIAL_COMMAND_GET_TIME "get_time:"
#define SERIAL_COMMAND_SET_TIME "set_time:"
#define SERIAL_RESPONSE_OK "ok;"
#define SERIAL_RESPONSE_ERROR "err;"
#define SERIAL_VALUE_REQUEST "req;"

PN532_SPI pn532spi(SPI, 10);
MyCard nfc(pn532spi);

uint8_t ndefBuf[120];
NdefMessage message;
int messageSize;
int idAddress = 0;
char id[8] = "";

uint8_t uid[3] = { 0x12, 0x34, 0x56 };

void setup() {
	pinMode(53, OUTPUT);
	Serial.begin(115200);

	delay(1000);
    nfc.setId("00005678");

    message = NdefMessage();
    message.addMimeMediaRecord("application/coffeeap", "ciao");
    messageSize = message.getEncodedSize();
    if (messageSize > sizeof(ndefBuf)) {
        while (1) { }
    }

    message.encode(ndefBuf);

    // comment out this command for no ndef message
    nfc.setNdefFile(ndefBuf, messageSize);

    // uid must be 3 bytes!
    nfc.setUid(uid);
    nfc.init();
}

// Add loop code

void loop() {

	Serial.println("log:main loop;");
    nfc.emulate(5000);
    Serial.println("log:fine emulazione;");
}
