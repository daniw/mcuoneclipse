/*
 * Application.c
 *
 *  Created on: 15.10.2014
 *      Author: tastyger
 */
#include "PE_Types.h"
#include "CLS1.h"
#include "WAIT1.h"
#include "AS2.h"
#include "UTIL1.h"

#define ESP_TIMOUT_MS 200

static void Send(unsigned char *str) {
  while(*str!='\0') {
    AS2_SendChar(*str);
    str++;
  }
}

static uint8_t RxResponse(unsigned char *rxBuf, size_t rxBufLength, uint16_t msTimeout, unsigned char *expectedTail)
{
  unsigned char ch;
  uint8_t res = ERR_OK;
  unsigned char *p;

  if (rxBufLength < sizeof("x\r\n")) {
    return ERR_OVERFLOW; /* not enough space in buffer */
  }
  p = rxBuf;
  p[0] = '\0';
  for(;;) { /* breaks */
    if (msTimeout == 0) {
      break; /* will decide outside of loop if it is a timeout. */
    } else if (rxBufLength == 0) {
      res = ERR_OVERFLOW; /* not enough space in buffer */
      break;
    } else if (AS2_GetCharsInRxBuf() > 0) {
      if (AS2_RecvChar(&ch) != ERR_OK) {
        res = ERR_RXEMPTY;
        break;
      }
      *p++ = ch;
      *p = '\0'; /* always terminate */
      rxBufLength--;
    } else if (expectedTail[0] != '\0'
          && UTIL1_strtailcmp(rxBuf, expectedTail) == 0) {
      break; /* finished */
    } else {
      WAIT1_Waitms(1);
      msTimeout--;
    }
  } /* for */
  if (msTimeout==0) { /* timeout! */
    if (expectedTail[0] != '\0' /* timeout, and we expected something: an error for sure */
        || rxBuf[0] == '\0' /* timeout, did not know what to expect, but received nothing? There has to be a response. */
       )
    {
      res = ERR_FAULT;
    }
  }
  return res;
}

uint8_t ESP_SendATCommand(uint8_t *cmd, uint8_t *rxBuf, size_t rxBufSize, uint8_t *expectedTailStr)
{
  uint16_t snt;
  uint8_t res;

  if (AS2_SendBlock(cmd, (uint16_t)UTIL1_strlen((char*)cmd), &snt) != ERR_OK) {
    return ERR_FAILED;
  }
  CLS1_SendStr(cmd, CLS1_GetStdio()->stdOut);
  res = RxResponse(rxBuf, rxBufSize, ESP_TIMOUT_MS, expectedTailStr);
  CLS1_SendStr(rxBuf, CLS1_GetStdio()->stdOut);
  return res;
}

uint8_t ESP_Test(void) {
  uint8_t rxBuf[sizeof("AT\r\r\n\r\nOK\r\n")];
  uint8_t res;

  res = ESP_SendATCommand("AT\r\n", rxBuf, sizeof(rxBuf), "AT\r\r\n\r\nOK\r\n");
  return res;
}

uint8_t ESP_Restart(void) {
  uint8_t rxBuf[sizeof("AT+RST\r\r\n\r\nOK\r\n")];
  uint8_t res;

//  return ESP_SendATCommand("AT+RST\r\n", rxBuf, sizeof(rxBuf), "\r\nready\r\n");
  res = ESP_SendATCommand("AT+RST\r\n", rxBuf, sizeof(rxBuf), "AT+RST\r\r\n\r\nOK\r\n");
  WAIT1_Waitms(500); /* wait after restart */
  AS2_ClearRxBuf();
  return res;
}

void APP_Run(void) {
  CLS1_ConstStdIOType *io;

  io = CLS1_GetStdio();
  if (ESP_Test()!=ERR_OK) {
    CLS1_SendStr("TEST failed!\r\n", io->stdErr);
  } else {
    CLS1_SendStr("TEST ok!\r\n", io->stdOut);
  }
  if (ESP_Restart()!=ERR_OK) {
    CLS1_SendStr("Restart failed!\r\n", io->stdErr);
  } else {
    CLS1_SendStr("Restart ok!\r\n", io->stdOut);
  }
  for(;;) {
    CLS1_SendStr("Hello ESP8266!\r\n", io->stdOut);
    ESP_Test();
    WAIT1_Waitms(1000);
  }
}
