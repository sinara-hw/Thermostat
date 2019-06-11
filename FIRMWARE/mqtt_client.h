/*
 * mqtt_client.h
 *
 *
 */

#ifndef MQTT_CLIENT_H_
#define MQTT_CLIENT_H_

#include "lwip/apps/mqtt.h"

mqtt_client_t *client;

void do_connect();
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
static void mqtt_sub_request_cb(void *arg, err_t result);
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len);
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags);
void publish_onoff(mqtt_client_t *client, void *arg, const char *topic);
void publish_cal(mqtt_client_t *client, void *arg, const char *topic);
void publish_tec(mqtt_client_t *client, void *arg, const char *topic);
void publish_temp(mqtt_client_t *client, void *arg, const char *topic);
void publish_cloop(mqtt_client_t *client, void *arg, const char *topic);
static void mqtt_pub_request_cb(void *arg, err_t result);
int parse_topic(const char *text, char **result, int length);


#endif /* MQTT_CLIENT_H_ */
