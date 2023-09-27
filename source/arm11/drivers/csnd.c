/*
 *   This file is part of open_agb_firm
 *   Copyright (C) 2021 derrek, profi200
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "types.h"
#include "arm11/drivers/csnd.h"
#include "arm11/drivers/codec.h"



void CSND_init(void)
{
	static bool inited = false;
	if(inited) return;
	inited = true;

	CODEC_init();

	Csnd *const csnd = getCsndRegs();
	csnd->master_vol = 32768; // Full volume.
	csnd->cnt        = CSND_CNT_EN | CSND_CNT_RS_FILTER_EN;

	// Stop all channels.
	CsndCh *const csndCh = csnd->ch;
	for(u32 i = 0; i < 32; i++) csndCh[i].cnt = 0;

	// Stop all captures.
	CsndCap *const csndCap = csnd->cap;
	csndCap[0].cnt = 0;
	csndCap[1].cnt = 0;
}

void CSND_setupCh(const u8 ch, const u16 srFreq, const u32 vol, const u32 *const data,
                  const u32 *const data2, const u32 size, const u16 flags)
{
	CsndCh *const csndCh = getCsndChRegs(ch);
	csndCh->sr       = srFreq;
	csndCh->vol      = vol;
	csndCh->capvol   = vol;
	csndCh->st_addr  = (u32)data;
	csndCh->size     = size;
	csndCh->lp_addr  = (u32)data2;
	csndCh->st_adpcm = 0;                     // TODO: Hardcoded for now.
	csndCh->lp_adpcm = 0;                     // TODO: Hardcoded for now.
	csndCh->cnt      = CSND_CH_START | flags; // Start in paused state.
}


void CSND_startCap(const u8 ch, const u16 sr, u32 *const data, const u32 size, const u16 flags)
{
	CsndCap *const csndCap = getCsndCapRegs(ch);
	csndCap->sr   = sr;
	csndCap->size = size;
	csndCap->addr = (u32)data;
	csndCap->cnt  = CSND_CAP_START | flags;
}
