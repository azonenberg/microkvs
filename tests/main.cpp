/***********************************************************************************************************************
*                                                                                                                      *
* microkvs v0.1                                                                                                        *
*                                                                                                                      *
* Copyright (c) 2021 Andrew D. Zonenberg and contributors                                                              *
* All rights reserved.                                                                                                 *
*                                                                                                                      *
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the     *
* following conditions are met:                                                                                        *
*                                                                                                                      *
*    * Redistributions of source code must retain the above copyright notice, this list of conditions, and the         *
*      following disclaimer.                                                                                           *
*                                                                                                                      *
*    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the       *
*      following disclaimer in the documentation and/or other materials provided with the distribution.                *
*                                                                                                                      *
*    * Neither the name of the author nor the names of any contributors may be used to endorse or promote products     *
*      derived from this software without specific prior written permission.                                           *
*                                                                                                                      *
* THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   *
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL *
* THE AUTHORS BE HELD LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES        *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR       *
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
* POSSIBILITY OF SUCH DAMAGE.                                                                                          *
*                                                                                                                      *
***********************************************************************************************************************/

#include <kvs/KVS.h>
#include <driver/TestStorageBank.h>
#include <stdio.h>

void PrintState(KVS& kvs);

bool WriteAndVerify(KVS& kvs, const char* name, uint8_t* data, uint32_t len);
bool Verify(KVS& kvs, const char* name, uint8_t* data, uint32_t len);

int main(int argc, char* argv[])
{
	//Create the KVS
	//128 log entries = 3.5 kB used by log, about 28.5 kB free for data
	TestStorageBank left;
	TestStorageBank right;
	KVS kvs(&left, &right, 128);

	//Verify sane initial config
	printf("INITIAL STATE\n");
	PrintState(kvs);

	const char* data = "hello world";
	if(!WriteAndVerify(kvs, "OHAI", (uint8_t*)data, strlen(data)))
		return 1;

	printf("WITH ONE OBJECT\n");
	PrintState(kvs);

	//Write a second file
	const char* data2 = "lolcat";
	if(!WriteAndVerify(kvs, "shibe", (uint8_t*)data2, strlen(data2)))
		return 1;

	printf("WITH TWO OBJECTS\n");
	PrintState(kvs);

	//Write new content to the first object
	const char* data3 = "i herd u leik mudkipz";
	if(!WriteAndVerify(kvs, "OHAI", (uint8_t*)data3, strlen(data3)))
		return 1;
	if(!Verify(kvs, "shibe", (uint8_t*)data2, strlen(data2)))
		return 1;

	printf("MODIFIED 1\n");
	PrintState(kvs);

	//Write new content to the second object
	const char* data4 = "ceiling cat is watching";
	if(!WriteAndVerify(kvs, "shibe", (uint8_t*)data4, strlen(data4)))
		return 1;
	if(!Verify(kvs, "OHAI", (uint8_t*)data3, strlen(data3)))
		return 1;

	printf("MODIFIED 2\n");
	PrintState(kvs);

	//Write new content to a third object
	const char* data5 = "basement cat attacks!!!1!1!";
	if(!WriteAndVerify(kvs, "monorail", (uint8_t*)data5, strlen(data5)))
		return 1;
	if(!Verify(kvs, "OHAI", (uint8_t*)data3, strlen(data3)))
		return 1;
	if(!Verify(kvs, "shibe", (uint8_t*)data4, strlen(data4)))
		return 1;

	printf("THREE OBJECTS\n");
	PrintState(kvs);

	//Compact the array and verify both objects
	kvs.Compact();
	printf("COMPACTED\n");
	PrintState(kvs);
	if(!Verify(kvs, "OHAI", (uint8_t*)data3, strlen(data3)))
		return 1;
	if(!Verify(kvs, "shibe", (uint8_t*)data4, strlen(data4)))
		return 1;
	if(!Verify(kvs, "monorail", (uint8_t*)data5, strlen(data5)))
		return 1;

	return 0;
}

bool WriteAndVerify(KVS& kvs, const char* name, uint8_t* data, uint32_t len)
{
	if(!kvs.StoreObject(name, data, len))
	{
		printf("Failed to store object\n");
		return false;
	}

	return Verify(kvs, name, data, len);
}

bool Verify(KVS& kvs, const char* name, uint8_t* data, uint32_t len)
{
	//Read it back
	auto log = kvs.FindObject(name);
	if(!log)
	{
		printf("Object couldn't be found\n");
		return false;
	}
	if(log->m_len != len)
	{
		printf("Log entry length is wrong\n");
		return false;
	}
	if(memcmp(data, kvs.MapObject(log), log->m_len) != 0)
	{
		printf("Object content is wrong\n");
		return false;
	}
	return true;
}

void PrintState(KVS& kvs)
{
	if(kvs.IsLeftBankActive())
		printf("    Active bank:      left\n");
	else
		printf("    Active bank:      right\n");
	printf("    Free log entries: %d\n", kvs.GetFreeLogEntries());
	printf("    Free data space:  %d\n", kvs.GetFreeDataSpace());
}
