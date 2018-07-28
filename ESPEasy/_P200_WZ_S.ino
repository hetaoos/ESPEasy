//#######################################################################################################
//#################################### Plugin 200: Dart WZ-S ####################################
//#######################################################################################################
//
// http://www.aqmd.gov/docs/default-source/aq-spec/resources-page/plantower-pms5003-manual_v2-3.pdf?sfvrsn=2
//
// The WZ-S are particle sensors. Particles are measured by blowing air through the enclosure and,
// together with a laser, count the amount of particles. These sensors have an integrated microcontroller
// that counts particles and transmits measurement data over the serial connection.


#include <ESPeasySoftwareSerial.h>

#define PLUGIN_200
#define PLUGIN_ID_200 200
#define PLUGIN_NAME_200 "Formaldehyde - Dart WZ-S"
#define PLUGIN_VALUENAME1_200 "ppb"
#define PLUGIN_VALUENAME2_200 "ug/m³"
#define PLUGIN_VALUENAME3_200 "mg/m³"
#define WZ_S_START_TAG 0Xff
#define WZ_S_SIZE 9

ESPeasySoftwareSerial *swSerial = NULL;
boolean Plugin_200_init = false;
boolean Plugin_200_values_received = false;
float Plugin_200_last_value = -1;
int Plugin_200_ticks = 0;


void Plugin_200_SerialFlush() {
  if (swSerial != NULL) {
    swSerial->flush();
  } else {
    Serial.flush();
  }
}

boolean Plugin_200_PacketAvailable(void)
{
  if (swSerial != NULL) // Software serial
  {
    // When there is enough data in the buffer, search through the buffer to
    // find header (buffer may be out of sync)
    if (!swSerial->available()) return false;
    while ((swSerial->peek() != WZ_S_START_TAG) && swSerial->available()) {
      swSerial->read(); // Read until the buffer starts with the first byte of a message, or buffer empty.
    }
    if (swSerial->available() < WZ_S_SIZE) return false; // Not enough yet for a complete packet
  }
  else // Hardware serial
  {
    // When there is enough data in the buffer, search through the buffer to
    // find header (buffer may be out of sync)
    if (!Serial.available()) return false;
    while ((Serial.peek() != WZ_S_START_TAG) && Serial.available()) {
      Serial.read(); // Read until the buffer starts with the first byte of a message, or buffer empty.
    }
    if (Serial.available() < WZ_S_SIZE) return false; // Not enough yet for a complete packet
  }
  return true;
}

boolean Plugin_200_process_data(struct EventStruct *event) {
  String log;
  uint8_t data[WZ_S_SIZE];
  if (swSerial != NULL)
  {
    for (int i = 0; i < WZ_S_SIZE; i++)
      data[i] = swSerial->read();
  }
  else
  {
    for (int i = 0; i < WZ_S_SIZE; i++)
      data[i] = Serial.read();
  }
  uint8_t checksum = 0;
  for (int i = 1; i < WZ_S_SIZE - 1; i++)
    checksum += data[i];

  checksum = (~checksum) + 1;

  if (checksum != data[WZ_S_SIZE - 1])
  {
    log = F("WZ-S : checksum error.");
    addLog(LOG_LEVEL_ERROR, log);
    return false;
  }

  uint8_t  high = data[4];
  uint8_t  low = data[5];
  float r1 =   high * 256.0 + low;
  // ignored if the value has not changed and reported time is less than 1 minute


  Plugin_200_SerialFlush(); // Make sure no data is lost due to full buffer.
  if (!(r1 != Plugin_200_last_value || Plugin_200_ticks >= 600))
  {
    return false;
  }
  Plugin_200_last_value = r1;
  Plugin_200_ticks = 0;
  float r2 = r1 * 30.03 / 22.4;
  float r3 = r2 / 1000.0;
  if (loglevelActiveFor(LOG_LEVEL_DEBUG_MORE)) {
    log = F("WZ-S : ");
    log += r1;
    log += F("ppb, ");
    log += r2;
    log += F("ug/m³, ");
    log += r3;
    log += F("mg/m³, ");
    addLog(LOG_LEVEL_DEBUG_MORE, log);
  }

  // fill in output
  UserVar[event->BaseVarIndex]     = r1;
  UserVar[event->BaseVarIndex + 1] = r2;
  UserVar[event->BaseVarIndex + 2] = r3;
  Plugin_200_values_received = true;
  return true;
}

boolean Plugin_200(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_200;
        Device[deviceCount].Type = DEVICE_TYPE_DUAL;
        Device[deviceCount].VType = SENSOR_TYPE_TRIPLE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 3;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        success = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_200);
        success = true;
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_200));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_200));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_200));
        success = true;
        break;
      }

    case PLUGIN_GET_DEVICEGPIONAMES:
      {
        event->String1 = F("GPIO &larr; TX");
        event->String2 = F("GPIO &rarr; RX");
        break;
      }

    case PLUGIN_INIT:
      {
        int rxPin = Settings.TaskDevicePin1[event->TaskIndex];
        int txPin = Settings.TaskDevicePin2[event->TaskIndex];

        String log = F("WZ-S : config ");
        log += rxPin;
        log += txPin;
        addLog(LOG_LEVEL_DEBUG, log);

        if (swSerial != NULL) {
          // Regardless the set pins, the software serial must be deleted.
          delete swSerial;
          swSerial = NULL;
        }

        // Hardware serial is RX on 3 and TX on 1
        if (rxPin == 3 && txPin == 1)
        {
          log = F("WZ-S : using hardware serial");
          addLog(LOG_LEVEL_INFO, log);
          Serial.begin(9600);
          Serial.flush();
        }
        else
        {
          log = F("WZ-S: using software serial");
          addLog(LOG_LEVEL_INFO, log);
          swSerial = new ESPeasySoftwareSerial(rxPin, txPin, false, WZ_S_SIZE * 10); // 90 Bytes buffer, enough for up to 10 packets..
          swSerial->begin(9600);
          swSerial->flush();
        }

        Plugin_200_init = true;
        success = true;
        Plugin_200_last_value = -1;
        Plugin_200_ticks = 0;
        break;
      }

    case PLUGIN_EXIT:
      {
        if (swSerial)
        {
          delete swSerial;
          swSerial = NULL;
        }
        Plugin_200_last_value = -1;
        Plugin_200_ticks = 0;
        break;
      }

    // The update rate from the module is 200ms .. multiple seconds. Practise
    // shows that we need to read the buffer many times per seconds to stay in
    // sync.
    case PLUGIN_TEN_PER_SECOND:
      {
        if (Plugin_200_init)
        {
          Plugin_200_ticks++;
          // Check if a complete packet is available in the UART FIFO.
          if (Plugin_200_PacketAvailable())
          {
            addLog(LOG_LEVEL_DEBUG_MORE, F("WZ-S : Packet available"));
            success = Plugin_200_process_data(event);
          }
        }
        break;
      }
    case PLUGIN_READ:
      {
        // When new data is available, return true
        success = Plugin_200_values_received;
        Plugin_200_values_received = false;
        break;
      }
  }
  return success;
}
