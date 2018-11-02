/*********************************************************************
  This is an example for our nRF52 based Bluefruit LE modules

  Pick one up today in the adafruit shop!

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  MIT license, check LICENSE for more information
  All text above, and the splash screen below must be included in
  any redistribution
*********************************************************************/

//https://github.com/vshymanskyy/miband2-python-test

#include <bluefruit.h>


#include <Crypto.h>  //https://github.com/rweather/arduinolibs
#include <AES.h>
#include <string.h>

AES128 aes128;
byte buffer[16];
byte enc_buffer[18];

uint8_t AUTH_SEND_KEY = 0x01;
uint8_t AUTH_REQUEST_RANDOM_AUTH_NUMBER  = 0x02;
uint8_t AUTH_SEND_ENCRYPTED_AUTH_NUMBER = 0x03;
uint8_t AUTH_RESPONSE  = 0x10;
uint8_t AUTH_SUCCESS = 0x01;
uint8_t AUTH_FAIL = 0x04;
uint8_t AUTH_BYTE = 0x8;

uint8_t SECRET_KEY[18]  = {0x01, 0x08, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45};
uint8_t SHORT_SECRET_KEY[16]  = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45};

uint8_t PAIRING_KEY[18] = {0x01, 0x08, 0xC8, 0x2A, 0xE3, 0x40, 0xEB, 0x1A, 0x5F, 0x1A, 0x36, 0x33, 0x59, 0x4B, 0x2B, 0xA4, 0x43, 0x4A};
int authenticated = 1;

uint8_t CONFIRM[2] = {0x02, 0x08};

uint8_t alertTest[9] = {0x05, 0x01, 0x52, 0x69, 0x63, 0x68, 0x61, 0x72, 0x64};  //sms
//uint8_t alertTest[9] = {0x03, 0x01, 0x52, 0x69, 0x63, 0x68, 0x61, 0x72, 0x64};  //sms acknowledge

/*UUID_CHARACTERISTIC_AUTH  "00000009-0000-3512-2118-0009af100700";*/
uint8_t AUTH_SERVICE_UUID[] = { 0x00, 0x07, 0x10, 0xaf, 0x09, 0x00, 0x18, 0x21, 0x12, 0x35, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00};

BLEClientService authService(0xFEE1);
BLEClientCharacteristic authNotif(AUTH_SERVICE_UUID);

BLEClientService       ImmediateAlertService(0x1802);
BLEClientCharacteristic AlertLevelCharacteristic(0x2a06);

//
BLEClientService       AlertNotifService(0x1811);
BLEClientCharacteristic NewAlertCharacteristic(0x2a46);

int Paired = 0;
uint16_t conn_handle1;

void setup()
{
  Serial.begin(115200);

  Serial.println("Bluefruit52 Central ADV Scan Example");
  Serial.println("------------------------------------\n");

  // Initialize Bluefruit with maximum connections as Peripheral = 0, Central = 1
  // SRAM usage required by SoftDevice will increase dramatically with number of connections
  Bluefruit.begin(0, 1);
  authService.begin();

  // set up callback for receiving measurement
  authNotif.setNotifyCallback(authNotif_callback);
  authNotif.begin();

  ImmediateAlertService.begin();
  AlertLevelCharacteristic.begin();

  AlertNotifService.begin();
  NewAlertCharacteristic.begin();

  // Set max power. Accepted values are: -40, -30, -20, -16, -12, -8, -4, 0, 4
  Bluefruit.setTxPower(4);

  /* Set the device name */
  Bluefruit.setName("Bluefruit52");

  /* Set the LED interval for blinky pattern on BLUE LED */
  Bluefruit.setConnLedInterval(250);

  /* Start Central Scanning
     - Enable auto scan if disconnected
     - Filter out packet with a min rssi
     - Interval = 100 ms, window = 50 ms
     - Use active scan (used to retrieve the optional scan response adv packet)
     - Start(0) = will scan forever since no timeout is given
  */
  Bluefruit.Scanner.setRxCallback(scan_callback);

  // Callbacks for Central
  Bluefruit.Central.setDisconnectCallback(disconnect_callback);
  Bluefruit.Central.setConnectCallback(connect_callback);

  // Bluefruit.Scanner.restartOnDisconnect(true);
  Bluefruit.Scanner.filterRssi(-80);
  // Bluefruit.Scanner.filterUuid(authService.uuid); // only invoke callback if detect bleuart service

  // Bluefruit.Scanner.filterUuid(alertClient.uuid);
  Bluefruit.Scanner.setInterval(160, 80);       // in units of 0.625 ms
  Bluefruit.Scanner.useActiveScan(true);        // Request scan response data
  Bluefruit.Scanner.start(0);                   // 0 = Don't stop scanning after n seconds

  Serial.println("Scanning ...");
}

void scan_callback(ble_gap_evt_adv_report_t* report)
{
  uint8_t len = 0;
  uint8_t buffer[32];
  memset(buffer, 0, sizeof(buffer));
  //mi band is FA:AB:33:E3:12:2D
  if (report->peer_addr.addr[5] == 0xFA) {
    // Connect to device
    Serial.println("Found device");
    Bluefruit.Central.connect(report);
  }
}

/**
   Callback invoked when an connection is established
   @param conn_handle
*/
void connect_callback(uint16_t conn_handle)
{
  Serial.println("Connected");
  conn_handle1 = conn_handle;
  if (AlertNotifService.discover(conn_handle)) {
    Serial.println("AlertNotifService discovered");
  }
  if (NewAlertCharacteristic.discover()) {
    Serial.println("NewAlertCharacteristic discovered");
  }

  if (authService.discover(conn_handle) )
  {
    Serial.println("Found Auth Service");
    if (authNotif.discover()) {
      Serial.println("AuthNotif found");

      if (authNotif.enableNotify()) {
        Serial.println("Nofitications enabled");


        //this starts the pairing request and the mi band will buzz and expect you to push the button
        if (authenticated) {
          if ( authNotif.write(CONFIRM, 2) )
          {
            Serial.println("Request Random");
          } else
          {
            Serial.println("Request Random Failed");
          }
        } else {
          if ( authNotif.write(SECRET_KEY, 18) )
          {
            Serial.println("Key Sent");
          } else
          {
            Serial.println("Alert Failed");
          }
        }

      } else {
        Serial.println("Notify enable Failed");
      }
    }
  }

  if (ImmediateAlertService.discover(conn_handle) )
  {
    Serial.println("Found Alert Service");

    if (AlertLevelCharacteristic.discover()) {
      Serial.println("Found alert characteristic");
    }

  }
}


/**
   Callback invoked when a connection is dropped
   @param conn_handle
   @param reason
*/
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;
  (void) reason;
  Paired = 0;
  Serial.println("Disconnected");
}


/**
   Hooked callback that triggered when a measurement value is sent from peripheral
   @param chr   Pointer client characteristic that even occurred,
                in this example it should be hrmc
   @param data  Pointer to received data
   @param len   Length of received data
*/
void authNotif_callback(BLEClientCharacteristic * chr, uint8_t* data, uint16_t len)
{
  Serial.print("AuthNotif: ");

  for (int i = 0; i < len; i++) {
    Serial.print(data[i], HEX); Serial.print(":");
  }
  Serial.println("");

  //step 1 of paring
  if (data[0] == 0x10 && data[1] == 0x01 && data[2] == 0x01) {
    Serial.println("Next Part");

    if ( authNotif.write(CONFIRM, 2) )
    {
      Serial.println("Confirm Sent");
    } else
    {
      Serial.println("Confirm Failed");
    }
  }

  //Step 2 of pairing
  if (data[0] == 0x10 && data[1] == 0x02 && data[2] == 0x01) {
    Serial.println("This is the key");


    //AES encryption
    char data1[] = {data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15], data[16], data[17], data[18], data[19]}; //16 chars == 16 bytes
    //

    for (int i = 0; i < 16; i++) {
      Serial.print(data1[i], HEX); Serial.print(":");
    }
    Serial.println("");

    byte plain[] = {data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15], data[16], data[17], data[18], data[19]}; //16 chars == 16 bytes

    crypto_feed_watchdog();
    aes128.setKey(SHORT_SECRET_KEY, aes128.keySize());
    aes128.encryptBlock(buffer, plain);

    Serial.println("AES ECB Encrypted");
    for (int i = 0; i < 16; i++) {
      Serial.print(buffer[i], HEX); Serial.print(":");
    }
    Serial.println("");

    //
    //

    enc_buffer[0] = 0x03; enc_buffer[1] = 0x08;
    for (int i = 0; i < 16; i++) {
      enc_buffer[i + 2] = buffer[i];
    }

    if ( authNotif.write(enc_buffer, 18) )
    {
      Serial.println("encrypted key Sent");
    } else
    {
      Serial.println("encrypted key Failed");
    }

  }

  //Step 3 of pairing
  if (data[0] == 0x10 && data[1] == 0x03 && data[2] == 0x01) {
    Paired = 1;
    Serial.println("Paired");
  }
}

void loop()
{
  // Toggle both LEDs every 1 second
  digitalToggle(LED_RED);
  delay(1000);
  if (Paired) {

    delay(4000);
    //if (AlertLevelCharacteristic.discover()) {
    //  Serial.println("Write alertLevelChar");
    //  Serial.println( AlertLevelCharacteristic.write8( 0x02 ) );
    //}

    Serial.println("Write alertTest");
    NewAlertCharacteristic.write_resp( alertTest, 9 ) ;

    delay(1000);

  }
  Paired = 0;

}
