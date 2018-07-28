//#######################################################################################################
//#################################### Plugin 201: Plantower PMS5003ST ####################################
//#######################################################################################################
//
// https://github.com/HaishengLiang/pms5003ST/raw/master/PMS5XXXST%E9%A1%86%E7%B2%92%E7%89%A9%E5%82%B3%E6%84%9F%E5%99%A8%E4%B8%AD%E6%96%87%E8%AA%AA%E6%98%8E%E6%9B%B8V2.0.pdf
//
// The PMS5003ST are particle sensors. Particles are measured by blowing air through the enclosure and,
// together with a laser, count the amount of particles. These sensors have an integrated microcontroller
// that counts particles and transmits measurement data over the serial connection.


#include <ESPeasySoftwareSerial.h>

#define PLUGIN_201
#define PLUGIN_ID_201 201
#define PLUGIN_NAME_201 "Dust - PMS5003ST"
#define PLUGIN_VALUENAME1_201 "pm1.0"
#define PLUGIN_VALUENAME2_201 "pm2.5"
#define PLUGIN_VALUENAME3_201 "pm10"
#define PLUGIN_VALUENAME4_201 "ug/m³"
#define PLUGIN_VALUENAME5_201 "temperature"
#define PLUGIN_VALUENAME6_201 "humidity"

#define PMS5003ST_SIG1 0X42
#define PMS5003ST_SIG2 0X4d
#define PMS5003ST_VALUE_COUNT 17
#define PMS5003ST_SIZE 40

ESPeasySoftwareSerial *swSerial201 = NULL;
boolean Plugin_201_init = false;
boolean Plugin_201_values_received = false;

uint16_t Plugin_201_last_values[PMS5003ST_VALUE_COUNT] = {0};
int      Plugin_201_ticks = 0;

void Plugin_201_ResetStatus()
{
  Plugin_201_ticks = 0;
  for (int i = 0; i < PMS5003ST_VALUE_COUNT; i++)
    Plugin_201_last_values[i] = 0;
}
// Read 2 bytes from serial and make an uint16 of it. Additionally calculate
// checksum for PMS5003ST. Assumption is that there is data available, otherwise
// this function is blocking.
void Plugin_201_SerialRead16(uint16_t* value, uint16_t* checksum)
{
  uint8_t data_high, data_low;

  // If swSerial201 is initialized, we are using soft serial
  if (swSerial201 != NULL)
  {
    data_high = swSerial201->read();
    data_low = swSerial201->read();
  }
  else
  {
    data_high = Serial.read();
    data_low = Serial.read();
  }

  *value = data_low;
  *value |= (data_high << 8);

  if (checksum != NULL)
  {
    *checksum += data_high;
    *checksum += data_low;
  }

#if 0
  // Low-level logging to see data from sensor
  String log = F("PMS5003ST : byte high=0x");
  log += String(data_high, HEX);
  log += F(" byte low=0x");
  log += String(data_low, HEX);
  log += F(" result=0x");
  log += String(*value, HEX);
  addLog(LOG_LEVEL_INFO, log);
#endif
}

void Plugin_201_SerialFlush() {
  if (swSerial201 != NULL) {
    swSerial201->flush();
  } else {
    Serial.flush();
  }
}

boolean Plugin_201_PacketAvailable(void)
{
  if (swSerial201 != NULL) // Software serial
  {
    // When there is enough data in the buffer, search through the buffer to
    // find header (buffer may be out of sync)
    if (!swSerial201->available()) return false;

    while ((swSerial201->peek() != PMS5003ST_SIG1) && swSerial201->available()) {
      swSerial201->read(); // Read until the buffer starts with the first byte of a message, or buffer empty.
    }
    if (swSerial201->available() < PMS5003ST_SIZE) return false; // Not enough yet for a complete packet
  }
  else // Hardware serial
  {
    // When there is enough data in the buffer, search through the buffer to
    // find header (buffer may be out of sync)
    if (!Serial.available()) return false;
    while ((Serial.peek() != PMS5003ST_SIG1) && Serial.available()) {
      Serial.read(); // Read until the buffer starts with the first byte of a message, or buffer empty.
    }
    if (Serial.available() < PMS5003ST_SIZE) return false; // Not enough yet for a complete packet
  }
  return true;
}

boolean Plugin_201_process_data(struct EventStruct *event) {
  String log;
  uint16_t checksum = 0, checksum2 = 0;
  uint16_t framelength = 0;
  uint16 packet_header = 0;
  Plugin_201_SerialRead16(&packet_header, &checksum); // read PMS5003ST_SIG1 + PMS5003ST_SIG2
  if (packet_header != ((PMS5003ST_SIG1 << 8) | PMS5003ST_SIG2)) {
    // Not the start of the packet, stop reading.
    return false;
  }

  Plugin_201_SerialRead16(&framelength, &checksum);
  if (framelength != (PMS5003ST_SIZE - 4))
  {
    log = F("PMS5003ST : invalid framelength - ");
    log += framelength;
    addLog(LOG_LEVEL_ERROR, log);
    return false;
  }

  uint16_t data[PMS5003ST_VALUE_COUNT]; // byte data_low, data_high;
  for (int i = 0; i < PMS5003ST_VALUE_COUNT; i++)
    Plugin_201_SerialRead16(&data[i], &checksum);


  // Compare checksums
  Plugin_201_SerialRead16(&checksum2, NULL);
  Plugin_201_SerialFlush(); // Make sure no data is lost due to full buffer.
  if (checksum != checksum2)
  {
    log = F("PMS5003ST : checksum error.");
    addLog(LOG_LEVEL_ERROR, log);
    return false;
  }
  if (Plugin_201_ticks < 600)
  {
    boolean diff = false;
    for (int i = 0; i < PMS5003ST_VALUE_COUNT; i++)
    {
      if (Plugin_201_last_values[i] != data[i])
      {
        Plugin_201_last_values[i] = data[i];
        diff = true;
      }
    }
    if (diff == false)
    {
      log = F("PMS5003ST : value unchanged. ticks: ");
      log += Plugin_201_ticks;
      addLog(LOG_LEVEL_DEBUG_MORE, log);
      return false;
    }
  }
  if (loglevelActiveFor(LOG_LEVEL_DEBUG_MORE)) {
    // Data is checked and good, fill in output
    log = F("PMS5003ST : pm1.0=");
    log += data[0];
    log += F(", pm2.5=");
    log += data[1];
    log += F(", pm10=");
    log += data[2];
    log += F(", pm1.0a=");
    log += data[3];
    log += F(", pm2.5a=");
    log += data[4];
    log += F(", pm10a=");
    log += data[5];
    addLog(LOG_LEVEL_DEBUG_MORE, log);

    log = F("PMS5003ST : count/0.1L : 0.3um=");
    log += data[6];
    log += F(", 0.5um=");
    log += data[7];
    log += F(", 1.0um=");
    log += data[8];
    log += F(", 2.5um=");
    log += data[9];
    log += F(", 5.0um=");
    log += data[10];
    log += F(", 10um=");
    log += data[11];
    addLog(LOG_LEVEL_DEBUG_MORE, log);

    log = F("PMS5003ST : formaldehyde : ");
    log += (data[12] / 1000.0);
    log += F(" mg/m³ , temperature: ");
    log += (data[13] / 10.0);
    log += F(" ℃, humidity :");
    log += (data[14] / 10.0);
    log += F(" %");
    addLog(LOG_LEVEL_DEBUG_MORE, log);
  }

  UserVar[event->BaseVarIndex]     = data[3];
  UserVar[event->BaseVarIndex + 1] = data[4];
  UserVar[event->BaseVarIndex + 2] = data[5];
  UserVar[event->BaseVarIndex + 3] = data[12];
  UserVar[event->BaseVarIndex + 4] = data[13] / 10.0;
  UserVar[event->BaseVarIndex + 5] = data[14] / 10.0;
  Plugin_201_values_received = true;
  return true;

}

boolean Plugin_201(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_201;
        Device[deviceCount].Type = DEVICE_TYPE_TRIPLE;
        Device[deviceCount].VType = SENSOR_TYPE_HEXA;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 6;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        success = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_201);
        success = true;
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_201));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_201));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_201));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[3], PSTR(PLUGIN_VALUENAME4_201));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[4], PSTR(PLUGIN_VALUENAME5_201));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[5], PSTR(PLUGIN_VALUENAME6_201));
        success = true;
        break;
      }

    case PLUGIN_GET_DEVICEGPIONAMES:
      {
        event->String1 = F("GPIO &larr; TX");
        event->String2 = F("GPIO &rarr; RX");
        event->String3 = F("GPIO &rarr; Reset");
        break;
      }

    case PLUGIN_INIT:
      {
        int rxPin = Settings.TaskDevicePin1[event->TaskIndex];
        int txPin = Settings.TaskDevicePin2[event->TaskIndex];
        int resetPin = Settings.TaskDevicePin3[event->TaskIndex];

        String log = F("PMS5003ST : config ");
        log += rxPin;
        log += txPin;
        log += resetPin;
        addLog(LOG_LEVEL_DEBUG, log);

        if (swSerial201 != NULL) {
          // Regardless the set pins, the software serial must be deleted.
          delete swSerial201;
          swSerial201 = NULL;
        }

        // Hardware serial is RX on 3 and TX on 1
        if (rxPin == 3 && txPin == 1)
        {
          log = F("PMS5003ST : using hardware serial");
          addLog(LOG_LEVEL_INFO, log);
          Serial.begin(9600);
          Serial.flush();
        }
        else
        {
          log = F("PMS5003ST: using software serial");
          addLog(LOG_LEVEL_INFO, log);
          swSerial201 = new ESPeasySoftwareSerial(rxPin, txPin, false, PMS5003ST_SIZE * 30); // 1200 Bytes buffer, enough for up to 30 packets.
          swSerial201->begin(9600);
          swSerial201->flush();
        }

        if (resetPin >= 0) // Reset if pin is configured
        {
          // Toggle 'reset' to assure we start reading header
          log = F("PMS5003ST: resetting module");
          addLog(LOG_LEVEL_INFO, log);
          pinMode(resetPin, OUTPUT);
          digitalWrite(resetPin, LOW);
          delay(250);
          digitalWrite(resetPin, HIGH);
          pinMode(resetPin, INPUT_PULLUP);
        }

        Plugin_201_init = true;
        success = true;
        Plugin_201_ResetStatus();
        break;
      }

    case PLUGIN_EXIT:
      {
        if (swSerial201)
        {
          delete swSerial201;
          swSerial201 = NULL;
        }
        Plugin_201_ResetStatus();
        break;
      }

    // The update rate from the module is 200ms .. multiple seconds. Practise
    // shows that we need to read the buffer many times per seconds to stay in
    // sync.
    case PLUGIN_TEN_PER_SECOND:
      {
        if (Plugin_201_init)
        {
          Plugin_201_ticks++;
          // Check if a complete packet is available in the UART FIFO.
          if (Plugin_201_PacketAvailable())
          {
            addLog(LOG_LEVEL_DEBUG_MORE, F("PMS5003ST : Packet available"));
            success = Plugin_201_process_data(event);
          }
        }
        break;
      }
    case PLUGIN_READ:
      {
        // When new data is available, return true
        success = Plugin_201_values_received;
        Plugin_201_values_received = false;
        break;
      }
  }
  return success;
}
