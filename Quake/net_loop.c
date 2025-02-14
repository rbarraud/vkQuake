/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2010-2014 QuakeSpasm developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"
#include "q_stdinc.h"
#include "arch_def.h"
#include "net_sys.h"
#include "net_defs.h"
#include "net_loop.h"

static qboolean   localconnectpending = false;
static qsocket_t *loop_client = NULL;
static qsocket_t *loop_server = NULL;

int Loop_Init (void)
{
	if (cls.state == ca_dedicated)
		return -1;
	return 0;
}

void Loop_Shutdown (void) {}

void Loop_Listen (qboolean state) {}

qboolean Loop_SearchForHosts (qboolean xmit)
{
	if (!sv.active)
		return false;

	hostCacheCount = 1;
	if (strcmp (hostname.string, "UNNAMED") == 0)
		strcpy (hostcache[0].name, "local");
	else
		strcpy (hostcache[0].name, hostname.string);
	strcpy (hostcache[0].map, sv.name);
	hostcache[0].users = net_activeconnections;
	hostcache[0].maxusers = svs.maxclients;
	hostcache[0].driver = net_driverlevel;
	strcpy (hostcache[0].cname, "local");
	return false;
}

qsocket_t *Loop_Connect (const char *host)
{
	if (strcmp (host, "local") != 0)
		return NULL;

	localconnectpending = true;

	if (!loop_client)
	{
		if ((loop_client = NET_NewQSocket ()) == NULL)
		{
			Con_Printf ("Loop_Connect: no qsocket available\n");
			return NULL;
		}
		strcpy (loop_client->trueaddress, "localhost");
		strcpy (loop_client->maskedaddress, "localhost");
	}
	loop_client->receiveMessageLength = 0;
	loop_client->sendMessageLength = 0;
	loop_client->canSend = true;

	if (!loop_server)
	{
		if ((loop_server = NET_NewQSocket ()) == NULL)
		{
			Con_Printf ("Loop_Connect: no qsocket available\n");
			return NULL;
		}
		strcpy (loop_server->trueaddress, "LOCAL");
		strcpy (loop_server->maskedaddress, "LOCAL");
	}
	loop_server->receiveMessageLength = 0;
	loop_server->sendMessageLength = 0;
	loop_server->canSend = true;

	loop_client->driverdata = (void *)loop_server;
	loop_server->driverdata = (void *)loop_client;

	loop_client->proquake_angle_hack = loop_server->proquake_angle_hack = true;

	return loop_client;
}

qsocket_t *Loop_CheckNewConnections (void)
{
	if (!localconnectpending)
		return NULL;

	localconnectpending = false;
	loop_server->sendMessageLength = 0;
	loop_server->receiveMessageLength = 0;
	loop_server->canSend = true;
	loop_client->sendMessageLength = 0;
	loop_client->receiveMessageLength = 0;
	loop_client->canSend = true;
	return loop_server;
}

static int IntAlign (int value)
{
	return (value + (sizeof (int) - 1)) & (~(sizeof (int) - 1));
}

int Loop_GetMessage (qsocket_t *sock)
{
	int ret;
	int length;

	if (sock->receiveMessageLength == 0)
		return 0;

	ret = sock->receiveMessage[0];
	length = sock->receiveMessage[1] + (sock->receiveMessage[2] << 8);
	// alignment byte skipped here
	SZ_Clear (&net_message);
	if (ret == 2)
	{ // unreliables have sequences that we (now) care about so that clients can ack them.
		sock->unreliableReceiveSequence =
			sock->receiveMessage[4] | (sock->receiveMessage[5] << 8) | (sock->receiveMessage[6] << 16) | (sock->receiveMessage[7] << 24);
		sock->unreliableReceiveSequence++;
		SZ_Write (&net_message, &sock->receiveMessage[8], length);
		length = IntAlign (length + 8);
	}
	else
	{ // reliable
		SZ_Write (&net_message, &sock->receiveMessage[4], length);
		length = IntAlign (length + 4);
	}

	sock->receiveMessageLength -= length;

	if (sock->receiveMessageLength)
		memmove (sock->receiveMessage, &sock->receiveMessage[length], sock->receiveMessageLength);

	if (sock->driverdata && ret == 1)
		((qsocket_t *)sock->driverdata)->canSend = true;

	return ret;
}

qsocket_t *Loop_GetAnyMessage (void)
{
	if (loop_server)
	{
		if (Loop_GetMessage (loop_server) > 0)
			return loop_server;
	}
	return NULL;
}

int Loop_SendMessage (qsocket_t *sock, sizebuf_t *data)
{
	byte *buffer;
	int  *bufferLength;

	if (!sock->driverdata)
		return -1;

	bufferLength = &((qsocket_t *)sock->driverdata)->receiveMessageLength;

	if ((*bufferLength + data->cursize + NET_LOOPBACKHEADERSIZE) > NET_MAXMESSAGE * NET_LOOPBACKBUFFERS + NET_LOOPBACKHEADERSIZE)
		Sys_Error ("Loop_SendMessage: overflow");

	buffer = ((qsocket_t *)sock->driverdata)->receiveMessage + *bufferLength;

	// message type
	*buffer++ = 1;

	// length
	*buffer++ = data->cursize & 0xff;
	*buffer++ = data->cursize >> 8;

	// align
	buffer++;

	// message
	memcpy (buffer, data->data, data->cursize);
	*bufferLength = IntAlign (*bufferLength + data->cursize + 4);

	sock->canSend = false;
	return 1;
}

int Loop_SendUnreliableMessage (qsocket_t *sock, sizebuf_t *data)
{
	byte *buffer;
	int  *bufferLength;
	int   sequence = sock->unreliableSendSequence++;

	if (!sock->driverdata)
		return -1;

	bufferLength = &((qsocket_t *)sock->driverdata)->receiveMessageLength;

	// always leave one buffer for reliable messages
	if ((*bufferLength + data->cursize + NET_LOOPBACKHEADERSIZE) > NET_MAXMESSAGE * (NET_LOOPBACKBUFFERS - 1))
		return 0;

	buffer = ((qsocket_t *)sock->driverdata)->receiveMessage + *bufferLength;

	// message type
	*buffer++ = 2;

	// length
	*buffer++ = data->cursize & 0xff;
	*buffer++ = data->cursize >> 8;

	// align
	buffer++;

	*buffer++ = (sequence >> 0) & 0xff;
	*buffer++ = (sequence >> 8) & 0xff;
	*buffer++ = (sequence >> 16) & 0xff;
	*buffer++ = (sequence >> 24) & 0xff;

	// message
	memcpy (buffer, data->data, data->cursize);
	*bufferLength = IntAlign (*bufferLength + data->cursize + 8);
	return 1;
}

qboolean Loop_CanSendMessage (qsocket_t *sock)
{
	if (!sock->driverdata)
		return false;
	return sock->canSend;
}

qboolean Loop_CanSendUnreliableMessage (qsocket_t *sock)
{
	return true;
}

void Loop_Close (qsocket_t *sock)
{
	if (sock->driverdata)
		((qsocket_t *)sock->driverdata)->driverdata = NULL;
	sock->receiveMessageLength = 0;
	sock->sendMessageLength = 0;
	sock->canSend = true;
	if (sock == loop_client)
		loop_client = NULL;
	else
		loop_server = NULL;
}
