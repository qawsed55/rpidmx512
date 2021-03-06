/**
 * @file dmxsendparamssave.cpp
 *
 */
/* Copyright (C) 2019 by Arjan van Vught mailto:info@raspberrypi-dmx.nl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "dmxparams.h"
#include "dmxsendconst.h"

#include "propertiesbuilder.h"

#include "debug.h"

bool DMXParams::Builder(const struct TDMXParams *ptDMXParams, uint8_t *pBuffer, uint32_t nLength, uint32_t& nSize) {
	DEBUG_ENTRY

	assert(pBuffer != 0);

	if (ptDMXParams != 0) {
		memcpy(&m_tDMXParams, ptDMXParams, sizeof(struct TDMXParams));
	} else {
		m_pDMXParamsStore->Copy(&m_tDMXParams);
	}

	PropertiesBuilder builder(DMXSendConst::PARAMS_FILE_NAME, pBuffer, nLength);

	bool isAdded = builder.Add(DMXSendConst::PARAMS_BREAK_TIME, m_tDMXParams.nBreakTime, isMaskSet(DMX_SEND_PARAMS_MASK_BREAK_TIME));
	isAdded &= builder.Add(DMXSendConst::PARAMS_MAB_TIME, m_tDMXParams.nMabTime, isMaskSet(DMX_SEND_PARAMS_MASK_MAB_TIME));
	isAdded &= builder.Add(DMXSendConst::PARAMS_REFRESH_RATE, m_tDMXParams.nRefreshRate, isMaskSet(DMX_SEND_PARAMS_MASK_REFRESH_RATE));

	nSize = builder.GetSize();

	DEBUG_PRINTF("nSize=%d", nSize);

	DEBUG_EXIT
	return isAdded;
}

bool DMXParams::Save(uint8_t* pBuffer, uint32_t nLength, uint32_t& nSize) {
	DEBUG_ENTRY

	assert(pBuffer != 0);

	if (m_pDMXParamsStore == 0) {
		nSize = 0;
		DEBUG_EXIT
		return false;
	}

	return Builder(0, pBuffer, nLength, nSize);
}
