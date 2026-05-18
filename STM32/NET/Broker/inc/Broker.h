#ifndef _ONENET_H_
#define _ONENET_H_
#include <stdio.h>

_Bool Broker_Link(void);

void Broker_SendData(void);

void Broker_Subscribe(void);

void Broker_RevPro(unsigned char *cmd);

void Broker_Ping(void);

/*ADD BEGIN*/
void Broker_SendKeepAlive(void);   /* 应用层保活 */
/*ADD END*/

#endif
