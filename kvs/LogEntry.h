/***********************************************************************************************************************
*                                                                                                                      *
* microkvs                                                                                                             *
*                                                                                                                      *
* Copyright (c) 2021-2024 Andrew D. Zonenberg and contributors                                                         *
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
	@brief	Declaration of LogEntry
 */

#ifndef LogEntry_h
#define LogEntry_h

//Block writable flash
#ifdef MICROKVS_WRITE_BLOCK_SIZE

	//User-specified block size? Make sure it's valid
	#ifdef KVS_NAMELEN

		//Name length must be at least one write block
		#if ( KVS_NAMELEN < MICROKVS_WRITE_BLOCK_SIZE )
			#error KVS_NAMELEN must be an integer multiple of MICROKVS_WRITE_BLOCK_SIZE
		#endif

		//Block size must evenly divide name length
		#if ( (KVS_NAMELEN % MICROKVS_WRITE_BLOCK_SIZE) != 0 )
			#error KVS_NAMELEN must be an integer multiple of MICROKVS_WRITE_BLOCK_SIZE
		#endif


	//Use write block size as default name length
	#else
		#define KVS_NAMELEN MICROKVS_WRITE_BLOCK_SIZE
	#endif

//Byte writable flash
#else
	#define KVS_NAMELEN 16
#endif

/**
	@brief A single entry in the flash log
 */
class LogEntry
{
public:
	char		m_key[KVS_NAMELEN];
	uint32_t	m_start;
	uint32_t	m_len;
	uint32_t	m_crc;			//crc32 of packet content
	uint32_t	m_headerCRC;	//crc32 of {key, start, len}

	//pad to write block size
	#ifdef MICROKVS_WRITE_BLOCK_SIZE
		#if MICROKVS_WRITE_BLOCK_SIZE >= 16
			uint8_t		m_padding[MICROKVS_WRITE_BLOCK_SIZE - (16 % MICROKVS_WRITE_BLOCK_SIZE)];
		#endif
	#endif
};

#endif
