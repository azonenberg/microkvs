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

/**
	@file
	@author	Andrew D. Zonenberg
	@brief	Implementation of KVS
 */
#include "KVS.h"
#include <string.h>
#include <stdio.h>

#include <stm32fxxx.h>
#include <util/Logger.h>
extern Logger g_log;

#define HEADER_MAGIC 0xc0def00d

char g_blankKey[KVS_NAMELEN];

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

KVS::KVS(StorageBank* left, StorageBank* right, uint32_t defaultLogSize)
	: m_left(left)
	, m_right(right)
	, m_active(nullptr)
	, m_defaultLogSize(defaultLogSize)
	, m_firstFreeLogEntry(0)
	, m_firstFreeData(0)
{
	memset(g_blankKey, 0xff, KVS_NAMELEN);

	FindCurrentBank();
	ScanCurrentBank();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Reading

/**
	@brief Scan the current bank looking for free space
 */
void KVS::ScanCurrentBank()
{
	//Find free space
	auto log = m_active->GetLog();
	auto logsize = m_active->GetHeader()->m_logSize;
	m_firstFreeLogEntry = logsize-1;
	LogEntry* lastlog = NULL;
	for(int64_t i = logsize-1; i >= 0; i --)
	{
		//Stop if this entry was used
		if( (log[i].m_start != 0xffffffff) || (log[i].m_len != 0xffffffff) )
		{
			lastlog = &log[i];
			break;
		}

		//Nope, this entry is blank (thus free for future use)
		m_firstFreeLogEntry = i;
	}

	//If nothing in the log, free data area starts right after the log area
	if(!lastlog)
		m_firstFreeData = sizeof(BankHeader) + logsize*sizeof(LogEntry);

	//If we have at least one log entry in the store, free data starts after the last log entry ends
	else
		m_firstFreeData = lastlog->m_start + lastlog->m_len;
}

/**
	@brief Determine which bank is active, and set m_active appropriately
 */
void KVS::FindCurrentBank()
{
	//See which block(s) have valid headers
	auto lh = m_left->GetHeader();
	auto rh = m_right->GetHeader();
	bool leftValid = (lh->m_magic == HEADER_MAGIC);
	bool rightValid = (rh->m_magic == HEADER_MAGIC);

	//If NEITHER bank is valid, we have a blank chip.
	//Initialize and declare the left one active.
	if(!leftValid && !rightValid)
	{
		InitializeBank(m_left);
		m_active = m_left;
	}

	//If only one one bank is active, mark that one as active
	else if(leftValid && !rightValid)
		m_active = m_left;
	else if(!leftValid && rightValid)
		m_active = m_right;

	//If BOTH banks are active, the higher version number is our active bank
	else if(lh->m_version > rh->m_version)
		m_active = m_left;
	else
		m_active = m_right;
}

/**
	@brief Find the latest version of an object in the active bank, if present.

	Returns NULL if no object by that name exists.
 */
LogEntry* KVS::FindObject(const char* name)
{
	//Actual lookup key: zero padded if too short, but not guaranteed to be null terminated
	char key[KVS_NAMELEN] = {0};
	strncpy(key, name, KVS_NAMELEN);

	//Start searching the log
	auto len = m_active->GetHeader()->m_logSize;
	auto base = m_active->GetLog();
	LogEntry* log = nullptr;
	for(uint32_t i=0; i<len; i++)
	{
		//If start address is blank, this log entry was never written.
		//We must be at the end of the log. Whatever we've found by this point is all there is to find.
		if(base[i].m_start == 0xffffffff)
			break;

		//Skip anything without the right name
		if(memcmp(base[i].m_key, key, KVS_NAMELEN) != 0)
			continue;

		//If CRC match, this is the latest log entry
		if(m_active->CRC(m_active->GetBase() + base[i].m_start, base[i].m_len) == base[i].m_crc)
			log = &base[i];

		//If CRC mismatch, entry is corrupted - fall back to the previous entry
	}
	return log;
}

/**
	@brief Returns a pointer to the object described by a log entry
 */
uint8_t* KVS::MapObject(LogEntry* log)
{
	return m_active->GetBase() + log->m_start;
}

/**
	@brief Reads an object into a provided buffer.

	If the object is more than len bytes in size, the readback is truncated but no error is returned.

	@param name		Name of the object to read
	@param data		Output buffer
	@param len		Size of the output buffer
 */
bool KVS::ReadObject(const char* name, uint8_t* data, uint32_t len)
{
	auto log = FindObject(name);
	if(!log)
		return false;

	uint32_t readlen = log->m_len;
	if(readlen > len)
		readlen = len;

	memcpy(data, MapObject(log), readlen);
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Writing

/**
	@brief Initializes a blank bank with a header
 */
void KVS::InitializeBank(StorageBank* bank)
{
	//Erase the bank just to be safe
	bank->Erase();

	//Write the content of the new block header
	BankHeader header;
	header.m_magic = HEADER_MAGIC;
	header.m_version = 0;
	header.m_logSize = m_defaultLogSize;
	bank->Write(0, (uint8_t*)&header, sizeof(header));
}

/**
	@brief Writes a new object to the store.

	Any existing object by the same name is overwritten.

	@param name		Name of the object.
					Names must be exactly KVS_NAMELEN bytes in size.
					Shorter names are padded to KVS_NAMELEN with 0x00 bytes.
					Longer names are truncated.
					The name 0xFF...FF is reserved and may not be used for an object.
	@param data		Object content
	@param len		Length of the object
 */
bool KVS::StoreObject(const char* name, uint8_t* data, uint32_t len)
{
	//Actual lookup key: zero padded if too short, but not guaranteed to be null terminated
	char key[KVS_NAMELEN] = {0};
	strncpy(key, name, KVS_NAMELEN);

	//If there's not enough space for the file, compact the store to make more room
	if(GetFreeDataSpace() < len)
		Compact();

	//If not enough space after compaction, we're out of flash. Give up.
	if(GetFreeDataSpace() < len)
		return false;

	//Write header data to reserve the log entry
	uint32_t logoff = sizeof(BankHeader) + m_firstFreeLogEntry*sizeof(LogEntry);
	uint32_t header[3] = { m_firstFreeData, len, m_active->CRC(data, len) };
	m_firstFreeLogEntry ++;
	if(!m_active->Write(logoff + KVS_NAMELEN, reinterpret_cast<uint8_t*>(&header[0]), sizeof(header)))
		return false;

	//Write and verify object content
	auto offset = m_firstFreeData;
	m_firstFreeData += len;
	if(!m_active->Write(offset, data, len))
		return false;
	if(memcmp(data, m_active->GetBase() + offset, len) != 0)
		return false;

	//Write and verify object name
	if(!m_active->Write(logoff, reinterpret_cast<uint8_t*>(key), KVS_NAMELEN))
		return false;
	if(memcmp(key, m_active->GetBase() + logoff, KVS_NAMELEN) != 0)
		return false;

	//All good!
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Compaction

/**
	@brief Moves all active objects to the inactive bank, reclaiming free space in the process
 */
bool KVS::Compact()
{
	const uint32_t cachesize = 16;
	char cache[cachesize][KVS_NAMELEN];
	memset(cache, 0xff, sizeof(cache));
	uint32_t nextCache = 0;

	//Find the INACTIVE storage bank
	StorageBank* inactive = NULL;
	if(m_active == m_left)
		inactive = m_right;
	else
		inactive = m_left;

	//Erase the inactive bank and give it a header, but do NOT write the version number yet.
	//If we're interrupted during the compaction, we want the block to read as invalid.
	if(!inactive->Erase())
		return false;
	uint32_t magic = HEADER_MAGIC;
	if(!inactive->Write(0, (uint8_t*)&magic, sizeof(uint32_t)))
		return false;
	if(!inactive->Write(2*sizeof(uint32_t), (uint8_t*)&m_defaultLogSize, sizeof(m_defaultLogSize)))
		return false;

	//Loop over the log and copy files one by one
	auto base = m_active->GetBase();
	auto log = m_active->GetLog();
	auto outlog = inactive->GetLog();
	uint32_t nextLog = 0;
	uint32_t nextData = sizeof(BankHeader) + m_defaultLogSize*sizeof(LogEntry);
	for(int64_t i = m_firstFreeLogEntry-1; i>=0; i--)
	{
		//See if this item is in the cache.
		//If so, it was already copied so no need to do a full search of the log
		bool found = false;
		for(uint32_t j=0; j<cachesize; j++)
		{
			if(memcmp(cache[j], log[i].m_key, KVS_NAMELEN) == 0)
			{
				found = true;
				break;
			}
		}

		//Not in cache. Search the output log to see if it's there
		if(!found)
		{
			for(uint32_t j=0; j<nextLog; j++)
			{
				if(memcmp(outlog[j].m_key, log[i].m_key, KVS_NAMELEN) == 0)
				{
					found = true;
					break;
				}
			}
		}

		//If found, we've already copied a newer version of the object.
		//Discard this copy because it's obsolete
		if(found)
			continue;

		//Not found. This is the most up to date version.
		//Copy the data first, then the log
		if(!inactive->Write(nextData, base + log[i].m_start, log[i].m_len))
			return false;
		LogEntry entry = log[i];
		entry.m_start = nextData;
		if(!inactive->Write(sizeof(BankHeader) + nextLog*sizeof(LogEntry), (uint8_t*)&entry, sizeof(entry)))
			return false;

		//Update pointers for next output
		nextData += log[i].m_len;
		nextLog ++;

		//Add this entry to the cache of recently copied stuff
		memcpy(cache[nextCache], log[i].m_key, KVS_NAMELEN);
		nextCache = (nextCache + 1) % cachesize;
	}

	//Update the block header with the new version number
	uint32_t version = m_active->GetHeader()->m_version + 1;
	if(!inactive->Write(sizeof(uint32_t), (uint8_t*)&version, sizeof(uint32_t)))
		return false;

	//Done, switch banks
	m_active = inactive;
	m_firstFreeLogEntry = nextLog;
	m_firstFreeData = nextData;

	return true;
}

/**
	@brief Destroys all data in the INACTIVE bank.

	Performing a compaction followed by zeroizing the inactive bank does not affect the CURRENT contents of any
	objects, but ensures that PREVIOUS content of all objects is destroyed.
 */
void KVS::WipeInactive()
{
	if(m_active == m_left)
		m_right->Erase();
	else
		m_left->Erase();
}

/**
	@brief Destroys the entire contents of the KVS, including both the active and inactive blocks.

	This would typically be done as part of a factory reset or to purge sensitive data such as keys prior to
	decommissioning a piece of equipment.
 */
void KVS::WipeAll()
{
	m_left->Erase();
	m_right->Erase();
}
