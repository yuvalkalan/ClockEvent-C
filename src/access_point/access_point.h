#pragma once

// pico sdk libs ----------------------
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "dhcpserver/dhcpserver.h"
#include "dnsserver/dnsserver.h"
#include <tusb.h>
// ------------------------------------
// pure C/C++ libs ---------------------
#include <string>
#include <map>
#include <sstream>
#include <iomanip>
// ------------------------------------
// html c-style index file ------------
#include "htmldata.cpp"
// ------------------------------------
// storage control --------------------
#include "Settings/Settings.h"
// ------------------------------------

// http server settings ---------------
#define TCP_PORT 80
#define POLL_TIME_S 5
#define HTTP_GET "GET"
#define HTTP_RESPONSE_HEADERS "HTTP/1.1 %d OK\nContent-Length: %d\nContent-Type: text/html; charset=utf-8\nConnection: close\n\n"
#define PAGE_TITLE "/settings"
#define HTTP_RESPONSE_REDIRECT "HTTP/1.1 302 Redirect\nLocation: http://%s" PAGE_TITLE "\n\n"
// ------------------------------------
// AP Wifi settings -------------------
#define AP_WIFI_NAME "picow_test"
#define AP_WIFI_PASSWORD "password"
// ------------------------------------
// http req params --------------------
#define PARAM_CURRENT_DATE "currentDate"
#define PARAM_CURRENT_TIME "currentTime"
// ------------------------------------

struct TCPServer
{
    tcp_pcb *server_pcb;
    bool complete;
    ip_addr_t gw;
    Settings *settings;
    tm *current_time;
};
struct TCPConnect
{
    tcp_pcb *pcb;
    int sent_len;
    char headers[512];
    char result[8192];
    int header_len;
    int result_len;
    ip_addr_t *gw;
    Settings *settings;
    tm *current_time;
};

void tcp_server_close(TCPServer *state);
err_t tcp_server_recv(void *arg, tcp_pcb *pcb, pbuf *p, err_t err);
bool tcp_server_open(void *arg, const char *ap_name);
