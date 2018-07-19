//#######################################################################################################
//########################### Controller Plugin 014: Domoticz MQTT ######################################
//#######################################################################################################

#define CPLUGIN_014
#define CPLUGIN_ID_014         14
#define CPLUGIN_NAME_014       "Baidu MQTT"

#include <ArduinoJson.h>

boolean CPlugin_014(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case CPLUGIN_PROTOCOL_ADD:
      {
        Protocol[++protocolCount].Number = CPLUGIN_ID_014;
        Protocol[protocolCount].usesMQTT = true;
        Protocol[protocolCount].usesTemplate = false;
        Protocol[protocolCount].usesClientId = true;
        Protocol[protocolCount].usesAccount = true;
        Protocol[protocolCount].usesPassword = true;
        Protocol[protocolCount].defaultPort = 1883;
        Protocol[protocolCount].usesID = true;
        break;
      }

    case CPLUGIN_GET_DEVICENAME:
      {
        string = F(CPLUGIN_NAME_014);
        break;
      }

    case CPLUGIN_PROTOCOL_TEMPLATE:
      {
        break;
      }

    case CPLUGIN_PROTOCOL_RECV:
      {
        break;
      }

    case CPLUGIN_PROTOCOL_SEND:
      {
        if (event->idx != 0)
        {
          ControllerSettingsStruct ControllerSettings;
          LoadControllerSettings(event->ControllerIndex, (byte*)&ControllerSettings, sizeof(ControllerSettings));
          if (!ControllerSettings.checkHostReachable(true)) {
            success = false;
            break;
          }
          StaticJsonBuffer<200> jsonBuffer;

          JsonObject& root = jsonBuffer.createObject();
          root[F("idx")] = event->idx;
          root[F("deviceName")] = ExtraTaskSettings.TaskDeviceName;
          JsonObject& data = root.createNestedObject("reported");
          switch (event->sensorType)
          {
            case SENSOR_TYPE_SWITCH:
              data[F("command")] = String(F("switchlight"));
              if (UserVar[event->BaseVarIndex] == 0)
                data[F("switchcmd")] = String(F("Off"));
              else
                data[F("switchcmd")] = String(F("On"));
              break;
            case SENSOR_TYPE_DIMMER:
              data[F("command")] = String(F("switchlight"));
              if (UserVar[event->BaseVarIndex] == 0)
                data[F("switchcmd")] = String(F("Off"));
              else
                data[F("Set%20Level")] = UserVar[event->BaseVarIndex];
              break;

            case SENSOR_TYPE_SINGLE:
            case SENSOR_TYPE_LONG:
            case SENSOR_TYPE_DUAL:
            case SENSOR_TYPE_TRIPLE:
            case SENSOR_TYPE_QUAD:
            case SENSOR_TYPE_TEMP_HUM:
            case SENSOR_TYPE_TEMP_BARO:
            case SENSOR_TYPE_TEMP_EMPTY_BARO:
            case SENSOR_TYPE_TEMP_HUM_BARO:
            case SENSOR_TYPE_WIND:
            default:
              {
                byte valueCount = getValueCountFromSensorType(event->sensorType);
                for (byte x = 0; x < valueCount; x++)
                {
                  String name = F("value");
                  name += x;
                  data[name]  = formatUserVarNoCheck(event, x);
                }
              }
              break;
          }

          String json;
          root.printTo(json);
          String log = F("MQTT : ");
          log += json;
          addLog(LOG_LEVEL_DEBUG, log);

          String pubname = ControllerSettings.Publish;
          parseControllerVariables(pubname, event, false);
          if (!MQTTpublish(event->ControllerIndex, pubname.c_str(), json.c_str(), Settings.MQTTRetainFlag))
          {
            connectionFailures++;
          }
          else if (connectionFailures)
            connectionFailures--;

        } // if ixd !=0
        else
        {
          String log = F("MQTT : IDX cannot be zero!");
          addLog(LOG_LEVEL_ERROR, log);
        }
        break;
      }

  }
  return success;
}
