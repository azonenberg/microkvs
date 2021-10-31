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
	@brief	Declaration of StorageBank
 */

#ifndef StorageBank_h
#define StorageBank_h

#include "../kvs/BankHeader.h"
#include "../kvs/LogEntry.h"

/**
	@brief A single "bank" of flash storage.

	There is typically a 1:1 mapping from banks to erase blocks, however a bank may span multiple erase blocks. No two
	StorageBank objects may occupy the same flash erase block, and no other code or data may occupy an erase block
	claimed by a StorageBank or it runs the risk of being unexpectedly erased.

	Requirements for underlying storage:
	* Memory mapped for reads
	* Block level erase
	* Byte level writes
 */
class StorageBank
{
public:
	StorageBank(uint8_t* base, uint32_t size)
	: m_baseAddress(base)
	, m_bankSize(size)
	{}

	//Raw block access API (needs to be implemented by derived driver class)
	virtual bool Erase() =0;
	virtual bool Write(uint32_t offset, const uint8_t* data, uint32_t len) =0;

	//Checksumming of block content (may be HW accelerated)
	virtual uint32_t CRC(const uint8_t* ptr, uint32_t size) =0;

	BankHeader* GetHeader()
	{ return reinterpret_cast<BankHeader*>(m_baseAddress); }

	uint32_t GetSize()
	{ return m_bankSize; }

	LogEntry* GetLog()
	{ return reinterpret_cast<LogEntry*>(m_baseAddress + sizeof(BankHeader)); }

	uint8_t* GetBase()
	{ return m_baseAddress; }

protected:
	///@brief Address of the start of this block
	uint8_t*	m_baseAddress;

	///@brief Number of bytes of storage available
	uint32_t	m_bankSize;
};

#endif
