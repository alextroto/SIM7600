

// #define TINY_GSM_MODEM_SIM7000
#define TINY_GSM_MODEM_SIM7600

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
#define SerialAT Serial1

// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon

/*
 * Tests enabled
 */
#define TINY_GSM_TEST_GPRS true
#define TINY_GSM_TEST_WIFI false
#define TINY_GSM_TEST_CALL false
#define TINY_GSM_TEST_SMS false
#define TINY_GSM_TEST_USSD false
#define TINY_GSM_TEST_BATTERY false
#define TINY_GSM_TEST_GPS true
// powerdown modem after tests
#define TINY_GSM_POWERDOWN false

// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[] = "internet.vodafone.ro";
const char gprsUser[] = "internet.vodafone.ro";
const char gprsPass[] = "";
char request[30];
#include <TinyGsmClient.h>
#include <Ticker.h>
#include <ArduinoHttpClient.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
const char server[] = "ares-alpha.com";

const int port = 80;
HttpClient http(client, server, port);

#endif

Ticker tick;

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 60          /* Time ESP32 will go to sleep (in seconds) */

#define PIN_TX 27
#define PIN_RX 26
#define UART_BAUD 115200
#define PWR_PIN 4
#define LED_PIN 12
#define POWER_PIN 25
#define IND_PIN 36
#define PMU_IRQ 35
#define I2C_SDA 21
#define I2C_SCL 22

#include "axp20x.h"
AXP20X_Class PMU;
bool initPMU();
void IRAM_ATTR pmuInt()
{
}
void setup()
{
  // Set console baud rate
  SerialMon.begin(115200);
  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);
  delay(10);

  Wire.begin(I2C_SDA, I2C_SCL);
  initPMU();

  // Onboard LED light, it can be used freely
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(LED_PIN, HIGH);

  // POWER_PIN : This pin controls the power supply of the SIM7600
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, HIGH);
  delay(2000);
  // PWR_PIN ??? This Pin is the PWR-KEY of the SIM7600
  // The time of active low level impulse of PWRKEY pin to power on module , type 500 ms
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, HIGH);
  delay(500);
  digitalWrite(PWR_PIN, LOW);

  // IND_PIN: It is connected to the SIM7600 status Pin,
  // through which you can know whether the module starts normally.
  pinMode(IND_PIN, INPUT);

  DBG("Wait...");
  delay(8000);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  for (;;)
  {
    DBG("Initializing modem...");
    if (!modem.init())
    {
      DBG("Failed to restart modem, delaying 10s and retrying");
      delay(2000);
    }
    else
    {
      break;
    }
  }
  // Set to GSM mode, please refer to manual 5.11 AT+CNMP Preferred mode selection for more parameters
  /* String result;
   do {
       result = modem.setNetworkMode(13);
       delay(500);
   } while (result != "OK");*/
  String result;
  result = modem.setNetworkMode(2);
  DBG("setNetworkMode result:", result);
  /*
  if (modem.waitResponse(10000L) != 1)
  {
    DBG(" setNetworkMode fail");
    return;
  }*/
  modem.enableGPS();
#if 0
    //https://github.com/vshymanskyy/TinyGSM/pull/405
    uint8_t mode = modem.getGNSSMode();
    Serial.print("Get GNSS Mode:");
    Serial.println(mode);

    /**
     *  CGNSSMODE: <gnss_mode>,<dpo_mode>
     *  This command is used to configure GPS, GLONASS, BEIDOU and QZSS support mode.
     *  gnss_mode:
     *      0 : GLONASS
     *      1 : BEIDOU
     *      2 : GALILEO
     *      3 : QZSS
     *  dpo_mode :
     *      0 disable
     *      1 enable
     */
    Serial.print("Set GNSS Mode BEIDOU :");
    String res = modem.setGNSSMode(1, 1);
    Serial.println(res);
    delay(1000);
#endif
  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  //   DBG("Initializing modem...");
  //    if (!modem.restart()) {
  //        DBG("Failed to restart modem, delaying 10s and retrying");
  //        return;
  //    }

  String name = modem.getModemName();
  DBG("Modem Name:", name);

  String modemInfo = modem.getModemInfo();
  DBG("Modem Info:", modemInfo);

  // Unlock your SIM card with a PIN if needed
  if (GSM_PIN && modem.getSimStatus() != 3)
  {
    modem.simUnlock(GSM_PIN);
  }

  DBG("Waiting for network...");
  if (!modem.waitForNetwork())
  {
    delay(10000);
    return;
  }

  if (modem.isNetworkConnected())
  {
    DBG("Network connected");
  }

  DBG("Connecting to", apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass))
  {
    delay(10000);
    return;
  }

  bool res = modem.isGprsConnected();
  DBG("GPRS status:", res ? "connected" : "not connected");

  String ccid = modem.getSimCCID();
  DBG("CCID:", ccid);

  String imei = modem.getIMEI();
  DBG("IMEI:", imei);

  String cop = modem.getOperator();
  DBG("Operator:", cop);

  IPAddress local = modem.localIP();
  DBG("Local IP:", local);

  int csq = modem.getSignalQuality();
  DBG("Signal quality:", csq);
}

void loop()
{

  float lat, lon;
  if (modem.getGPS(&lat, &lon))
  {
    Serial.printf("lat:%f lon:%f\n", lat, lon);
  }
  else
  {
    Serial.printf("No gps locked\n");
  }
  float battery = PMU.getBattVoltage();
  DBG("Battery level=", battery);
  sprintf(request, "/api/v1/battery?battery=%f", battery);
  int err = http.get(request);
  if (err != 0)
  {
    Serial.println(F("failed to http get!\n"));
  }

  int status = http.responseStatusCode();
  Serial.printf("Response status code: %d\n", status);
  if (status == 200)
  {
    digitalWrite(LED_PIN, LOW);
    delay(200);
    digitalWrite(LED_PIN, HIGH);
  }
  else
  {
    for (int i = 0; i < 3; i++)
    {

      digitalWrite(LED_PIN, LOW);
      delay(50);
      digitalWrite(LED_PIN, HIGH);
      delay(50);
    }
  }
  http.stop();
  delay(30 * 1000); // wait 30 seconds
}
bool initPMU()
{
  if (PMU.begin(Wire, AXP192_SLAVE_ADDRESS) == AXP_FAIL)
  {
    return false;
  }
  /*
   * The charging indicator can be turned on or off
   * * * */
  // PMU.setChgLEDMode(LED_BLINK_4HZ);

  /*
   * The default ESP32 power supply has been turned on,
   * no need to set, please do not set it, if it is turned off,
   * it will not be able to program
   *
   *   PMU.setDCDC3Voltage(3300);
   *   PMU.setPowerOutPut(AXP192_DCDC3, AXP202_ON);
   *
   * * * */

  /*
   *   Turn off unused power sources to save power
   * **/

  PMU.setPowerOutPut(AXP192_DCDC1, AXP202_OFF);
  PMU.setPowerOutPut(AXP192_DCDC2, AXP202_OFF);
  PMU.setPowerOutPut(AXP192_LDO2, AXP202_OFF);
  PMU.setPowerOutPut(AXP192_LDO3, AXP202_OFF);
  PMU.setPowerOutPut(AXP192_EXTEN, AXP202_OFF);

  PMU.setDCDC3Voltage(3300);
  PMU.setPowerOutPut(AXP192_DCDC3, AXP202_ON);

  // PMU.setLDO2Voltage(3300);
  // PMU.setLDO3Voltage(3300);
  // PMU.setDCDC1Voltage(3300);

  // PMU.setPowerOutPut(AXP192_DCDC1, AXP202_ON);
  // PMU.setPowerOutPut(AXP192_LDO2, AXP202_ON);
  // PMU.setPowerOutPut(AXP192_LDO3, AXP202_ON);

  pinMode(PMU_IRQ, INPUT_PULLUP);
  attachInterrupt(PMU_IRQ, pmuInt, FALLING);

  PMU.adc1Enable(AXP202_VBUS_VOL_ADC1 |
                     AXP202_VBUS_CUR_ADC1 |
                     AXP202_BATT_CUR_ADC1 |
                     AXP202_BATT_VOL_ADC1,
                 AXP202_ON);

  PMU.enableIRQ(AXP202_VBUS_REMOVED_IRQ |
                    AXP202_VBUS_CONNECT_IRQ |
                    AXP202_BATT_REMOVED_IRQ |
                    AXP202_BATT_CONNECT_IRQ,
                AXP202_ON);
  PMU.clearIRQ();

  return true;
}