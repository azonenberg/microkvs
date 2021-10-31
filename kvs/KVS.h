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
	@brief	Declaration of KVS
 */

#ifndef KVS_h
#define KVS_h

#include <stdint.h>
#include "../driver/StorageBank.h"

/**
	@brief Top level KVS object
 */
class KVS
{
public:
	KVS(StorageBank* left, StorageBank* right, uint32_t defaultLogSize);

	LogEntry* FindObject(const char* name);
	uint8_t* MapObject(LogEntry* log);

	bool ReadObject(const char* name, uint8_t* data, uint32_t len);

	bool StoreObject(const char* name, const uint8_t* data, uint32_t len);

	uint32_t GetFreeLogEntries()
	{ return m_active->GetHeader()->m_logSize - m_firstFreeLogEntry; }

	uint32_t GetFreeDataSpace()
	{ return m_active->GetSize() - m_firstFreeData; }

	bool IsLeftBankActive()
	{ return (m_active == m_left); }

	bool IsRightBankActive()
	{ return (m_active == m_right); }

	bool Compact();

	void WipeInactive();
	void WipeAll();

protected:
	void FindCurrentBank();
	void ScanCurrentBank();

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
