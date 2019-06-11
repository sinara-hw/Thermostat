/*
 * mqtt_client.c
 *
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include "mqtt_client.h"
#include "lwip/apps/mqtt.h"
#include "lwip/err.h"
#include "lwip/ip_addr.h"
#include "string.h"
#include "mqtt/mqtt.c"
#include "stdlib.h"
#include "utils/uartstdio.h"
#include "config.h"
#include "send_command.h"
#include "driverlib/sysctl.h"
#include "utils/ustdlib.h"
#include "uart_th.h"

bool mqtt_conn;

void do_connect()
{
  struct mqtt_connect_client_info_t ci;
  err_t err;
  char msg[32];
  uint32_t msg_len;

  memset(&ci, 0, sizeof(ci));

  ci.client_id = "thermostat";
  ci.will_topic = "thermostat/unexpected_exit";

  /* Initiate client and connect to server*/

  ip_addr_t* ip_addr;
  ip_addr = (ip_addr_t*) mem_malloc(sizeof(ip_addr_t));
  ip_addr->addr = g_sParameters.ui32mqttip;

  err = mqtt_client_connect(client, ip_addr, MQTT_PORT, mqtt_connection_cb, 0, &ci);

  /* Print the result code if something goes wrong*/
  if(err != ERR_OK) {
      msg_len = usprintf(msg, "mqtt_connect return %d\n\r", err);
      UARTSend(msg, msg_len);
  }

  mem_free((void *) ip_addr);
}

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
  err_t err;
  char msg[32];
  uint32_t msg_len;

  if(status == MQTT_CONNECT_ACCEPTED) {
    msg_len = usprintf(msg, "MQTT: Successfully connected\n\r");
    UARTSend(msg, msg_len);

    mqtt_conn = true;

    /* Setup callback for incoming publish requests */
    mqtt_set_inpub_callback(client, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, arg);

    /* Subscribe to a topic "thermostat/#" with QoS level 1, call mqtt_sub_request_cb */
    err = mqtt_subscribe(client, "thermostat/#", 1, mqtt_sub_request_cb, arg);

    if(err != ERR_OK) {
        msg_len = usprintf(msg, "MQTT Error: mqtt_subscribe return: %d\n\r", err);
        UARTSend(msg, msg_len);
    }
  } else
  {
      msg_len = usprintf(msg, "MQTT: Disconnected, reason: %d\n\r", status);
      UARTSend(msg, msg_len);
      mqtt_conn = false;
  }
}

static void mqtt_sub_request_cb(void *arg, err_t result)
{
    char msg[32];
    uint32_t msg_len;
  /* Print the result code */
    if(result != ERR_OK) {
        msg_len = usprintf(msg, "MQTT: Subscribe error.\n\r");
        UARTSend(msg, msg_len);
    }

}

static int inpub_id;
static uint32_t ui32Port;
static uint32_t ans_len = 0;

static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{

  char *topic_p[7];
  char *topic_pub = (char *) mem_calloc(64,sizeof(char));
  int i,topics_number;
  for(i = 0; i < 7; i = i+1) {
      topic_p[i] = (char *) mem_calloc(32,sizeof(char));
  }

  if(strcmp(topic, "thermostat/reset") == 0) {
      SysCtlReset();
  }

  if(strcmp(topic, "thermostat/saveall") == 0) {
      ConfigSaveAll();
  }

  topics_number = parse_topic(topic, topic_p, strlen(topic));

  inpub_id = -1;

  /* Decode topic string into a user defined reference */
  if(topics_number >= 3 && topics_number <= 7) {
      if(strcmp(topic_p[1], "ch0") == 0 || strcmp(topic_p[1], "ch1") == 0) {
          strncpy(topic_pub, topic, strlen(topic)-strlen(topic_p[topics_number-1])-1);
          ui32Port = atoi(topic_p[1]+2);
          if(strcmp(topic_p[2], "on") == 0) {
              if(ui32Port == 0) {
                  onoff0 = 1;
              } else {
                  onoff1 = 1;
              }
              OnOff(ui32Port, 1);
          } else if(strcmp(topic_p[2], "off") == 0) {
              if(ui32Port == 0) {
                  onoff0 = 0;
              } else {
                  onoff1 = 0;
              }
              OnOff(ui32Port, 0);
          } else if(strcmp(topic_p[2], "onoff?") == 0) {
              inpub_id = 11;
              publish_onoff(client, arg, topic_pub);
          } else if(strcmp(topic_p[2], "cal") == 0) {
              if(strcmp(topic_p[3], "t0") == 0 && strcmp(topic_p[4], "read") == 0) {
                  inpub_id = 0;
                  publish_cal(client, arg, topic_pub);
              } else if(strcmp(topic_p[3], "t0") == 0 && strcmp(topic_p[4], "set") == 0) {
                  inpub_id = 16;
              } else if(strcmp(topic_p[3], "beta") == 0 && strcmp(topic_p[4], "read") == 0) {
                  inpub_id = 1;
                  publish_cal(client, arg, topic_pub);
              } else if(strcmp(topic_p[3], "beta") == 0 && strcmp(topic_p[4], "set") == 0) {
                  inpub_id = 17;
              } else if(strcmp(topic_p[3], "ratio") == 0 && strcmp(topic_p[4], "read") == 0) {
                  inpub_id = 2;
                  publish_cal(client, arg, topic_pub);
              } else if(strcmp(topic_p[3], "ratio") == 0 && strcmp(topic_p[4], "set") == 0) {
                  inpub_id = 18;
              } else if(strcmp(topic_p[3], "tempmode") == 0 && strcmp(topic_p[4], "read") == 0) {
                  inpub_id = 4;
                  publish_cal(client, arg, topic_pub);
              } else if(strcmp(topic_p[3], "tempmode") == 0 && strcmp(topic_p[4], "set") == 0) {
                  inpub_id = 20;
              } else if(strcmp(topic_p[3], "pta") == 0 && strcmp(topic_p[4], "read") == 0) {
                  inpub_id = 5;
                  publish_cal(client, arg, topic_pub);
              } else if(strcmp(topic_p[3], "pta") == 0 && strcmp(topic_p[4], "set") == 0) {
                  inpub_id = 21;
              } else if(strcmp(topic_p[3], "ptb") == 0 && strcmp(topic_p[4], "read") == 0) {
                  inpub_id = 6;
                  publish_cal(client, arg, topic_pub);
              } else if(strcmp(topic_p[3], "ptb") == 0 && strcmp(topic_p[4], "set") == 0) {
                  inpub_id = 22;
              } else if(strcmp(topic_p[3], "freq") == 0 && strcmp(topic_p[4], "read") == 0) {
                  inpub_id = 3;
                  publish_cal(client, arg, topic_pub);
              } else if(strcmp(topic_p[3], "freq") == 0 && strcmp(topic_p[4], "set") == 0) {
                  inpub_id = 19;
              } else if(strcmp(topic_p[3], "rawtemp") == 0 && strcmp(topic_p[4], "read") == 0) {
                  inpub_id = 7;
                  publish_cal(client, arg, topic_pub);
              }
          } else if(strcmp(topic_p[2], "tec") == 0) {
              if(strcmp(topic_p[3], "curr") == 0 && strcmp(topic_p[4], "read") == 0) {
                  inpub_id = 2;
                  publish_tec(client, arg, topic_pub);
              } else if(strcmp(topic_p[3], "curr") == 0 && strcmp(topic_p[4], "lim") == 0 && strcmp(topic_p[5], "pos") == 0 && strcmp(topic_p[6], "set") == 0) {
                  inpub_id = 6;
              } else if(strcmp(topic_p[3], "curr") == 0 && strcmp(topic_p[4], "lim") == 0 && strcmp(topic_p[5], "pos") == 0 && strcmp(topic_p[6], "read") == 0) {
                  inpub_id = 0;
                  publish_tec(client, arg, topic_pub);
              } else if(strcmp(topic_p[3], "curr") == 0 && strcmp(topic_p[4], "lim") == 0 && strcmp(topic_p[5], "neg") == 0 && strcmp(topic_p[6], "set") == 0) {
                  inpub_id = 10;
              } else if(strcmp(topic_p[3], "curr") == 0 && strcmp(topic_p[4], "lim") == 0 && strcmp(topic_p[5], "neg") == 0 && strcmp(topic_p[6], "read") == 0) {
                  inpub_id = 3;
                  publish_tec(client, arg, topic_pub);
              }else if(strcmp(topic_p[3], "volt") == 0 && strcmp(topic_p[4], "read") == 0) {
                  inpub_id = 1;
                  publish_tec(client, arg, topic_pub);
              } else if(strcmp(topic_p[3], "volt") == 0 && strcmp(topic_p[4], "lim") == 0 && strcmp(topic_p[5], "read") == 0) {
                  inpub_id = 4;
                  publish_tec(client, arg, topic_pub);
              } else if(strcmp(topic_p[3], "volt") == 0 && strcmp(topic_p[4], "lim") == 0 && strcmp(topic_p[5], "set") == 0) {
                  inpub_id = 9;
              }
          } else if(strcmp(topic_p[2], "temp") == 0) {
              if(strcmp(topic_p[3], "read") == 0) {
                  inpub_id = 1;
                  publish_temp(client, arg, topic_pub);
              } else if(strcmp(topic_p[3], "set") == 0 && strcmp(topic_p[4], "set") == 0) {
                  inpub_id = 7;
              } else if(strcmp(topic_p[3], "set") == 0 && strcmp(topic_p[4], "read") == 0) {
                  inpub_id = 0;
                  publish_temp(client, arg, topic_pub);
              } else if(strcmp(topic_p[3], "win") == 0 && strcmp(topic_p[4], "set") == 0) {
                  inpub_id = 8;
              } else if(strcmp(topic_p[3], "win") == 0 && strcmp(topic_p[4], "read") == 0) {
                  inpub_id = 2;
                  publish_temp(client, arg, topic_pub);
              }
          } else if(strcmp(topic_p[2], "cloop") == 0) {
              if(strcmp(topic_p[3], "P") == 0 && strcmp(topic_p[4], "set") == 0) {
                  inpub_id = 13;
              } else if(strcmp(topic_p[3], "P") == 0 && strcmp(topic_p[4], "read") == 0) {
                  inpub_id = 0;
                  publish_cloop(client, arg, topic_pub);
              } else if(strcmp(topic_p[3], "I") == 0 && strcmp(topic_p[4], "set") == 0) {
                  inpub_id = 14;
              } else if(strcmp(topic_p[3], "I") == 0 && strcmp(topic_p[4], "read") == 0) {
                  inpub_id = 1;
                  publish_cloop(client, arg, topic_pub);
              } else if(strcmp(topic_p[3], "D") == 0 && strcmp(topic_p[4], "set") == 0) {
                  inpub_id = 15;
              } else if(strcmp(topic_p[3], "D") == 0 && strcmp(topic_p[4], "read") == 0) {
                  inpub_id = 2;
                  publish_cloop(client, arg, topic_pub);
              }
          }
      }
  }

  mem_free(topic_pub);
  for(i = 0; i < 7; i = i + 1) {
      mem_free(topic_p[i]);
  }

}

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    bool bRetcode;
    int32_t i32Value = 0;
    int i = 0;
    int j = 0;
    char *value;
    char msg[32];
    uint32_t msg_len;

    if(flags & MQTT_DATA_FLAG_LAST & (len > 0)) {
        value = (char *) mem_calloc(32,sizeof(char));
        strncpy(value, (char *) data, len);
        /* Last fragment of payload received */

        /* Call function or do action depending on reference, in this case inpub_id */
        if(inpub_id == 6) { //set pos current limit
            bRetcode = ConfigCheckDecimalParam(value, &i32Value);
            if(bRetcode) {
                if(!Set_TEC_limit(ui32Port, i32Value, 0)) {
                    msg_len = usprintf(msg, "MQTT: incorrect value\n\r");
                    UARTSend(msg, msg_len);
                }
            } else {
                msg_len = usprintf(msg, "MQTT: incorrect value\n\r");
                UARTSend(msg, msg_len);
            }
        } else if(inpub_id == 7) { //set temperature
            while(data[i] != '.' && data[i] != ',' && i < len) {
                value[i] = data[i];
                i = i + 1;
            }
            while(i < len - 1) {
                value[i] = data[i+1];
                if(isdigit(value[i])) {
                    j = j + 1;
                }
                i = i+1;
            }
            if(j > 3) {
                value[i-j+3] = NULL;
            }
            while(j < 3) {
                value[i] = '0';
                i = i + 1;
                j = j + 1;
            }
            value[i] = NULL;
            bRetcode = ConfigCheckDecimalParam(value, &i32Value);
            if(bRetcode) {
                if(!Set_Temp(ui32Port, i32Value)) {
                    msg_len = usprintf(msg, "MQTT: incorrect value\n\r");
                    UARTSend(msg, msg_len);
                }
            } else {
                msg_len = usprintf(msg, "MQTT: incorrect value\n\r");
                UARTSend(msg, msg_len);
            }
        } else if(inpub_id == 8) { //set temperature window
            bRetcode = ConfigCheckDecimalParam(value, &i32Value);
            if(bRetcode) {
                if(!Set_Temp_Window(ui32Port, i32Value)) {
                    msg_len = usprintf(msg, "MQTT: incorrect value\n\r");
                    UARTSend(msg, msg_len);
                }
            } else {
                msg_len = usprintf(msg, "MQTT: incorrect value\n\r");
                UARTSend(msg, msg_len);
            }
        } else if(inpub_id == 9) { //set max volt
            bRetcode = ConfigCheckDecimalParam(value, &i32Value);
            if(bRetcode) {
                if(!Set_MaxVolt(ui32Port, i32Value)) {
                    msg_len = usprintf(msg, "MQTT: incorrect value\n\r");
                    UARTSend(msg, msg_len);
                }
            } else {
                msg_len = usprintf(msg, "MQTT: incorrect value\n\r");
                UARTSend(msg, msg_len);
            }
        } else if(inpub_id == 10) { //set neg current limit
            bRetcode = ConfigCheckDecimalParam(value, &i32Value);
            if(bRetcode) {
                if(!Set_TEC_limit(ui32Port, i32Value, 1)) {
                    msg_len = usprintf(msg, "MQTT: incorrect value\n\r");
                    UARTSend(msg, msg_len);
                }
            } else {
                msg_len = usprintf(msg, "MQTT: incorrect value\n\r");
                UARTSend(msg, msg_len);
            }
        } else if(inpub_id >= 13 && inpub_id <= 15) { //set PID
            bRetcode = ConfigCheckDecimalParam(value, &i32Value);
            if(bRetcode) {
                if(!Set_PID(ui32Port, inpub_id - 13, i32Value)) {
                    msg_len = usprintf(msg, "MQTT: incorrect value\n\r");
                    UARTSend(msg, msg_len);
                }
            } else {
                msg_len = usprintf(msg, "MQTT: incorrect value\n\r");
                UARTSend(msg, msg_len);
            }
        } else if(inpub_id == 16) { //set t0
            while(data[i] != '.' && data[i] != ',' && i < len) {
                value[i] = data[i];
                i = i + 1;
            }
            while(i < len - 1) {
                value[i] = data[i+1];
                if(isdigit(value[i])) {
                    j = j + 1;
                }
                i = i+1;
            }
            if(j > 3) {
                value[i-j+3] = NULL;
            }
            while(j < 3) {
                value[i] = '0';
                i = i + 1;
                j = j + 1;
            }
            value[i] = NULL;
            bRetcode = ConfigCheckDecimalParam(value, &i32Value);
            if(bRetcode) {
                if(!Set_Calib(ui32Port, 0, i32Value)) {
                    msg_len = usprintf(msg, "MQTT: incorrect value\n\r");
                    UARTSend(msg, msg_len);
                }
            } else {
                msg_len = usprintf(msg, "MQTT: incorrect value\n\r");
                UARTSend(msg, msg_len);
            }
        } else if(inpub_id == 17) { //set beta
            while(data[i] != '.' && data[i] != ',' && i < len) {
                value[i] = data[i];
                i = i + 1;
            }
            while(i < len - 1) {
                value[i] = data[i+1];
                if(isdigit(value[i])) {
                    j = j + 1;
                }
                i = i+1;
            }
            if(j > 3) {
                value[i-j+3] = NULL;
            }
            while(j < 3) {
                value[i] = '0';
                i = i + 1;
                j = j + 1;
            }
            value[i] = NULL;
            bRetcode = ConfigCheckDecimalParam(value, &i32Value);
            if(bRetcode) {
                if(!Set_Calib(ui32Port, 1, i32Value)) {
                    msg_len = usprintf(msg, "MQTT: incorrect value\n\r");
                    UARTSend(msg, msg_len);
                }
            } else {
                msg_len = usprintf(msg, "MQTT: incorrect value\n\r");
                UARTSend(msg, msg_len);
            }
        } else if(inpub_id == 18) { //set ratio
            while(data[i] != '.' && data[i] != ',' && i < len) {
                value[i] = data[i];
                i = i + 1;
            }
            while(i < len - 1) {
                value[i] = data[i+1];
                if(isdigit(value[i])) {
                    j = j + 1;
                }
                i = i+1;
            }
            if(j > 3) {
                value[i-j+3] = NULL;
            }
            while(j < 3) {
                value[i] = '0';
                i = i + 1;
                j = j + 1;
            }
            value[i] = NULL;
            bRetcode = ConfigCheckDecimalParam(value, &i32Value);
            if(bRetcode) {
                if(!Set_Calib(ui32Port, 2, i32Value)) {
                    msg_len = usprintf(msg, "MQTT: incorrect value\n\r");
                    UARTSend(msg, msg_len);
                }
            } else {
                msg_len = usprintf(msg, "MQTT: incorrect value\n\r");
                UARTSend(msg, msg_len);
            }
        } else if(inpub_id == 19) { //set freq
            bRetcode = ConfigCheckDecimalParam(value, &i32Value);
            if(bRetcode) {
                if(!Set_Calib(ui32Port, 3, i32Value)) {
                    msg_len = usprintf(msg, "MQTT: incorrect value\n\r");
                    UARTSend(msg, msg_len);
                }
            } else {
                msg_len = usprintf(msg, "MQTT: incorrect value\n\r");
                UARTSend(msg, msg_len);
            }
        } else if(inpub_id == 20) { //set tempmode
            bRetcode = ConfigCheckDecimalParam(value, &i32Value);
            if(bRetcode) {
                if(!Set_Calib(ui32Port, 4, i32Value)) {
                    msg_len = usprintf(msg, "MQTT: incorrect value\n\r");
                    UARTSend(msg, msg_len);
                }
            } else {
                msg_len = usprintf(msg, "MQTT: incorrect value\n\r");
                UARTSend(msg, msg_len);
            }
        } else if(inpub_id == 21) { //set ptA
            while(data[i] != '.' && data[i] != ',' && i < len) {
                value[i] = data[i];
                i = i + 1;
            }
            while(i < len - 1) {
                value[i] = data[i+1];
                if(isdigit(value[i])) {
                    j = j + 1;
                }
                i = i+1;
            }
            if(j > 3) {
                value[i-j+3] = NULL;
            }
            while(j < 3) {
                value[i] = '0';
                i = i + 1;
                j = j + 1;
            }
            value[i] = NULL;
            bRetcode = ConfigCheckDecimalParam(value, &i32Value);
            if(bRetcode) {
                if(!Set_Calib(ui32Port, 5, i32Value)) {
                    msg_len = usprintf(msg, "MQTT: incorrect value\n\r");
                    UARTSend(msg, msg_len);
                }
            } else {
                msg_len = usprintf(msg, "MQTT: incorrect value\n\r");
                UARTSend(msg, msg_len);
            }
        } else if(inpub_id == 22) { //set ptB
            while(data[i] != '.' && data[i] != ',' && i < len) {
                value[i] = data[i];
                i = i + 1;
            }
            while(i < len - 1) {
                value[i] = data[i+1];
                if(isdigit(value[i])) {
                    j = j + 1;
                }
                i = i+1;
            }
            if(j > 3) {
                value[i-j+3] = NULL;
            }
            while(j < 3) {
                value[i] = '0';
                i = i + 1;
                j = j + 1;
            }
            value[i] = NULL;
            bRetcode = ConfigCheckDecimalParam(value, &i32Value);
            if(bRetcode) {
                if(!Set_Calib(ui32Port, 6, i32Value)) {
                    msg_len = usprintf(msg, "MQTT: incorrect value\n\r");
                    UARTSend(msg, msg_len);
                }
            } else {
                msg_len = usprintf(msg, "MQTT: incorrect value\n\r");
                UARTSend(msg, msg_len);
            }
        }

        mem_free(value);
    }


}

void publish_onoff(mqtt_client_t *client, void *arg, const char *topic)
{
  char answer[32];
  err_t err;
  u8_t qos = 1; /* quality of service*/
  u8_t retain = 0; /* don't retain */
  char msg[32];
  uint32_t msg_len;

  if(ui32Port == 0) {
      if(onoff0 == 0) {
          ans_len = usprintf(answer, "OFF");
      } else {
          ans_len = usprintf(answer, "ON");
      }
  } else {
      if(onoff1 == 0) {
          ans_len = usprintf(answer, "OFF");
      } else {
          ans_len = usprintf(answer, "ON");
      }
  }

  err = mqtt_publish(client, topic, answer, ans_len, qos, retain, mqtt_pub_request_cb, arg);

  if(err != ERR_OK) {
      msg_len = usprintf(msg, "Publish error: %d\n\r", err);
      UARTSend(msg, msg_len);
  }
}

void publish_cal(mqtt_client_t *client, void *arg, const char *topic)
{
  char answer[32];
  err_t err;
  u8_t qos = 1; /* quality of service*/
  u8_t retain = 1; /* do retain */
  char msg[32];
  uint32_t msg_len;

  if(inpub_id >= 0 && inpub_id <= 6) {
      ans_len = Read_Calib(ui32Port, inpub_id, answer);
  } else {
      ans_len = Read_RawTemp(ui32Port, answer);
  }

  err = mqtt_publish(client, topic, answer, ans_len, qos, retain, mqtt_pub_request_cb, arg);

  if(err != ERR_OK) {
      msg_len = usprintf(msg, "Publish error: %d\n\r", err);
      UARTSend(msg, msg_len);
  }
}

void publish_tec(mqtt_client_t *client, void *arg, const char *topic)
{
  char answer[32];
  err_t err;
  u8_t qos = 1; /* quality of service*/
  u8_t retain = 1; /* do retain */
  char msg[32];
  uint32_t msg_len;

  if(inpub_id >= 0 && inpub_id <= 3) {
      ans_len = Read_TEC(ui32Port, inpub_id, answer);
      err = mqtt_publish(client, topic, answer, ans_len, qos, retain, mqtt_pub_request_cb, arg);
      if(err != ERR_OK) {
          msg_len = usprintf(msg, "Publish error: %d\n\r", err);
          UARTSend(msg, msg_len);
      }
  } else if(inpub_id == 4) {
      ans_len = Read_MaxVolt(ui32Port, answer);
      err = mqtt_publish(client, topic, answer, ans_len, qos, retain, mqtt_pub_request_cb, arg);
      if(err != ERR_OK) {
          msg_len = usprintf(msg, "Publish error: %d\n\r", err);
          UARTSend(msg, msg_len);
      }
  }
}

void publish_temp(mqtt_client_t *client, void *arg, const char *topic)
{
  char answer[32];
  err_t err;
  u8_t qos = 1; /* quality of service*/
  u8_t retain = 1; /* do retain */
  char msg[32];
  uint32_t msg_len;

  if(inpub_id >= 0 && inpub_id <= 2) {
      ans_len = Read_Temp(ui32Port, inpub_id, answer);
      if(inpub_id != 2 && ans_len > 3) {
          answer[ans_len] = answer[ans_len-1];
          answer[ans_len-1] = answer[ans_len-2];
          answer[ans_len-2] = answer[ans_len-3];
          answer[ans_len-3] = '.';
          answer[ans_len+1] = '\0';
          ans_len = ans_len + 1;
      } else if(inpub_id != 2 && ans_len == 3) {
          answer[ans_len+1] = answer[ans_len-1];
          answer[ans_len] = answer[ans_len-2];
          answer[ans_len-1] = answer[ans_len-3];
          answer[ans_len-2] = '.';
          answer[ans_len-3] = '0';
          answer[ans_len+2] = '\0';
          ans_len = ans_len + 2;
      } else if(inpub_id != 2 && ans_len == 2) {
          answer[ans_len+2] = answer[ans_len-1];
          answer[ans_len+1] = answer[ans_len-2];
          answer[ans_len] = '0';
          answer[ans_len-1] = '.';
          answer[ans_len-2] = '0';
          answer[ans_len+3] = '\0';
          ans_len = ans_len + 3;
      } else if(inpub_id != 2 && ans_len == 1) {
          answer[ans_len+3] = answer[ans_len-1];
          answer[ans_len+2] = '0';
          answer[ans_len+1] = '0';
          answer[ans_len] = '.';
          answer[ans_len-1] = '0';
          answer[ans_len+4] = '\0';
          ans_len = ans_len + 4;
      }
      err = mqtt_publish(client, topic, answer, ans_len, qos, retain, mqtt_pub_request_cb, arg);
      if(err != ERR_OK) {
          msg_len = usprintf(msg, "Publish error: %d\n\r", err);
          UARTSend(msg, msg_len);
      }
  }
}

void publish_cloop(mqtt_client_t *client, void *arg, const char *topic)
{
  char answer[32];
  err_t err;
  u8_t qos = 1; /* quality of service*/
  u8_t retain = 1; /* do retain */
  char msg[32];
  uint32_t msg_len;

  if(inpub_id >= 0 && inpub_id <= 5) {
      ans_len = Read_PID(ui32Port, inpub_id, answer);
      err = mqtt_publish(client, topic, answer, ans_len, qos, retain, mqtt_pub_request_cb, arg);
      if(err != ERR_OK) {
          msg_len = usprintf(msg, "Publish error: %d\n\r", err);
          UARTSend(msg, msg_len);
      }
  }
}

/* Called when publish is complete either with sucess or failure */
static void mqtt_pub_request_cb(void *arg, err_t result)
{
  char msg[32];
  uint32_t msg_len;

  if(result != ERR_OK) {
      msg_len = usprintf(msg, "Publish error: %d\n\r", result);
      UARTSend(msg, msg_len);
  }
}

/* Function to parse whole topic into subtopics */
int parse_topic(const char *text, char **result, int length)
{
    int i,j;
    int n = 0;

    if(text[0] == '/') {
        i = 1;
    } else {
        i = 0;
    }

    j  = 0;

    while(i < length) {
        if(text[i] != '/') {
            result[n][j] = text[i];
            j = j + 1;
        } else {
            n = n + 1;
            j = 0;
        }
        i = i + 1;

    }

    return n+1;
}
