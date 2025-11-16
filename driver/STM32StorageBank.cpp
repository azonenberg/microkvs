/***********************************************************************************************************************
*                                                                                                                      *
* microkvs                                                                                                             *
*                                                                                                                      *
* Copyright (c) 2021-2025 Andrew D. Zonenberg and contributors                                                         *
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
	@brief	Implementation of STM32StorageBank
 */
#include <stdint.h>
#include <memory.h>
#include <stm32.h>
#include <peripheral/Flash.h>
#include "STM32StorageBank.h"

#include <embedded-utils/Logger.h>
extern Logger g_log;

#ifndef NO_INTERNAL_FLASH

bool STM32StorageBank::Erase()
{
	bool err = Flash::BlockErase(m_baseAddress);
	#ifdef HAVE_FLASH_ECC
	Flash::ClearECCFaults();
	#endif
	return err;
}

bool STM32StorageBank::Write(uint32_t offset, const uint8_t* data, uint32_t len)
{
	bool err = Flash::Write(GetBase() + offset, data, len);
	#ifdef HAVE_FLASH_ECC
	Flash::ClearECCFaults();
	#endif
	return err;
}

//TODO: use CRC hard IP to speed this up!!
uint32_t STM32StorageBank::CRC(const uint8_t* ptr, uint32_t size)
{
	uint32_t poly = 0xedb88320;

	uint32_t crc = 0xffffffff;
	for(size_t n=0; n < size; n++)
	{
		uint8_t d = ptr[n];
		for(int i=0; i<8; i++)
		{
			bool b = ( crc ^ (d >> i) ) & 1;
			crc >>= 1;
			if(b)
				crc ^= poly;
		}
	}

	return ~(	((crc & 0x000000ff) << 24) |
				((crc & 0x0000ff00) << 8) |
				((crc & 0x00ff0000) >> 8) |
				 (crc >> 24) );
}

#endif
