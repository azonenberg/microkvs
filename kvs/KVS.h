/***********************************************************************************************************************
*                                                                                                                      *
* microkvs v0.1                                                                                                        *
*                                                                                                                      *
* Copyright (c) 2021-2023 Andrew D. Zonenberg and contributors                                                         *
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
	@brief	Declaration of KVS
 */

#ifndef KVS_h
#define KVS_h

#include <stdint.h>
#include "../driver/StorageBank.h"

//TODO: make this an optional component?
#include <stm32-cpp/src/util/StringBuffer.h>

/**
	@brief A list entry used for enumerating the content of the KVS
 */
struct KVSListEntry
{
	char key[KVS_NAMELEN+1];	//[KVS_NAMELEN] is always null for easy printing
								//even if original key is not null terminated
	uint32_t size;				//Size of the most recent copy of the object
	uint32_t revs;				//Number of copies (including the current one) stored in the current erase block
};

/**
	@brief Top level KVS object
 */
class KVS
{
public:
	KVS(StorageBank* left, StorageBank* right, uint32_t defaultLogSize);

	//Main API
	LogEntry* FindObject(const char* name);
	uint8_t* MapObject(LogEntry* log);
	bool ReadObject(const char* name, uint8_t* data, uint32_t len);
	bool StoreObject(const char* name, const uint8_t* data, uint32_t len);

	//Maintenance operations
	bool Compact();
	void WipeInactive();
	void WipeAll();

	//Enumeration
	uint32_t EnumObjects(KVSListEntry* list, uint32_t size);

	/**
		@brief Reads a value from the KVS, returning a default value if not found
	 */
	template<class T>
	T ReadObject(const char* name, T defaultValue)
	{
		auto hlog = FindObject(name);
		if(hlog)
			return *reinterpret_cast<const T*>(MapObject(hlog));
		else
			return defaultValue;
	}

	/**
		@brief Reads a value from the KVS, returning a default value if not found
	 */
	template<class T>
	T ReadObject(T defaultValue, const char* format, ...)
	{
		char objname[KVS_NAMELEN+1] = {0};
		StringBuffer sbuf(objname, sizeof(objname));

		__builtin_va_list list;
		__builtin_va_start(list, format);
		sbuf.Printf(format, list);
		__builtin_va_end(list);

		return ReadObject<T>(objname, defaultValue);
	}

	/*
		@brief Writes a value to the KVS if necessary.

		If the value is the same as the previous value in the KVS, or there is no previous value
		but the value being written is the same as the default value, no data is written.
	 */
	template<class T>
	bool StoreObjectIfNecessary(const char* name, T currentValue, T defaultValue)
	{
		//See if the value is already there
		auto hlog = FindObject(name);

		//If not found: write if non-default
		if(!hlog)
		{
			if(currentValue != defaultValue)
				return StoreObject(name, (const uint8_t*)&currentValue, sizeof(currentValue));
		}

		//If found: write if changed
		else
		{
			if(currentValue != *reinterpret_cast<const T*>(MapObject(hlog)))
				return StoreObject(name, (const uint8_t*)&currentValue, sizeof(currentValue));
		}
		return true;
	}

	/*
		@brief Writes a value to the KVS if necessary, using a sprintf'd object name

		If the value is the same as the previous value in the KVS, or there is no previous value
		but the value being written is the same as the default value, no data is written.
	 */
	template<class T>
	bool StoreObjectIfNecessary(T currentValue, T defaultValue, const char* format, ...)
	{
		char objname[KVS_NAMELEN+1] = {0};
		StringBuffer sbuf(objname, sizeof(objname));

		__builtin_va_list list;
		__builtin_va_start(list, format);
		sbuf.Printf(format, list);
		__builtin_va_end(list);

		return StoreObjectIfNecessary(objname, currentValue, defaultValue);
	}

	//Accessors
public:

	/**
		@brief Returns the number of log entries in the active block available for use
	 */
	uint32_t GetFreeLogEntries()
	{ return m_active->GetHeader()->m_logSize - m_firstFreeLogEntry; }

	/**
		@brief Returns the number of data bytes in the active block available for use
	 */
	uint32_t GetFreeDataSpace()
	{ return m_active->GetSize() - m_firstFreeData; }

	/**
		@brief Returns true if the left bank is active
	 */
	bool IsLeftBankActive()
	{ return (m_active == m_left); }

	/**
		@brief Returns true if the right bank is active
	 */
	bool IsRightBankActive()
	{ return (m_active == m_right); }

	/**
		@brief Returns the number of log spaces in the active block, both used and unused
	 */
	uint32_t GetLogCapacity()
	{ return m_active->GetHeader()->m_logSize; }

	/**
		@brief Returns the total number of bytes in the active block including header, log, and data
	 */
	uint32_t GetBlockSize()
	{ return m_active->GetSize(); }

	/**
		@brief Returns the total space allocated to data, both used and unused
	 */
	uint32_t GetDataCapacity()
	{ return GetBlockSize() - (sizeof(BankHeader) + GetLogCapacity()*sizeof(LogEntry)); }

protected:
	void FindCurrentBank();
	void ScanCurrentBank();

	static int ListCompare(const void* a, const void* b);

	void InitializeBank(StorageBank* bank);

	///@brief First storage bank ("left")
	StorageBank* m_left;

	///@brief Second storage bank ("right")
	StorageBank* m_right;

	///@brief The active bank (most recent copy). Points to either m_left or m_right.
	StorageBank* m_active;

	///@brief Log size to use when formatting a new bank (number of entries)
	uint32_t m_defaultLogSize;

	///@brief Index of the next log entry to write to
	uint32_t m_firstFreeLogEntry;

	///@brief Offset (from start of block) of the first free data byte
	uint32_t m_firstFreeData;
};

#endif
