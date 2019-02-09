
#include "game.h"
#include "resource.h"
#include "systemstub.h"
#include "util.h"

static bool compareSssLut(uint32_t flags_a, uint32_t flags_b) {
	// (flags_a & 0xFFF00FFF) == (flags_b & 0xFFF00FFF) ?
	if (((flags_a >> 20) & 15) == ((flags_b >> 20) & 15)) {
		if ((flags_a & 0xFFF) == (flags_b & 0xFFF)) {
			return (flags_a >> 24) == (flags_b >> 24);
		}
	}
	return false;
}

void Game::resetSound() {
	// TODO:
	clearSoundObjects();
}

void Game::removeSoundObject(SssObject *so) {
	so->pcm = 0;
	if ((so->flags & 1) != 0) {
		SssObject *next = so->nextPtr;
		SssObject *prev = so->prevPtr;
		if (next) {
			next->prevPtr = prev;
		}
		if (prev) {
			prev->nextPtr = next;
		} else {
			_sssObjectsList1 = next;
		}

		--_snd_fadeVolumeCounter;
		if (_sssObjectsList3 != so) {
			if (_snd_fadeVolumeCounter >= _snd_volumeMin) {
				return;
			}
			if (_sssObjectsList3 == 0) {
				return;
			}
		}
		_sssObjectsList3 = 0;
		if (_snd_fadeVolumeCounter < _snd_volumeMin) {
			return;
		}
		SssObject *current = _sssObjectsList1;
		if (current != 0) {
			SssObject *_ecx = 0;
			do {
				if (_ecx && _ecx->unk9 > current->unk9) {
					_ecx = current;
					_sssObjectsList3 = _ecx;
				}
			} while ((current = current->nextPtr) != 0);
		}
		return;
	}
	SssObject *next = so->nextPtr;
	SssObject *prev = so->prevPtr;
	if (next) {
		next->prevPtr = prev;
	}
	if (prev) {
		prev->nextPtr = next;
	} else {
		_sssObjectsList2 = next;
	}
}

void Game::updateSoundObject(SssObject *so) {
	_channelMixingTable[so->num] = 1;
	_sssObjectsChanged = so->filter->unk30;
	if ((so->flags & 4) == 0) {
//42B179:
		if (so->pcm == 0) {
			if (so->codeDataStage1) {
				so->codeDataStage1 = executeSssCode(so, so->codeDataStage1);
			}
			if (so->pcm == 0) {
				return;
			}
			if (so->codeDataStage4) {
				so->codeDataStage4 = executeSssCode(so, so->codeDataStage4);
			}
			if (so->pcm == 0) {
				return;
			}
			goto flag_case0;
		} else {
//42B1B9:
			if (so->codeDataStage1) {
				so->codeDataStage1 = executeSssCode(so, so->codeDataStage1);
			}
			if (so->pcm == 0) {
				return;
			}
			if (so->volumePtr) {
				const int volume = getSoundObjectVolumeByPos(so);
				if (volume != so->volume) {
					so->volume = volume;
					_sssObjectsChanged = 1;
				}
			} else {
				if (so->codeDataStage2) {
					so->codeDataStage2 = executeSssCode(so, so->codeDataStage2);
				}
			}
			if (so->pcm == 0) {
				return;
			}
			if (so->codeDataStage3) {
				so->codeDataStage3 = executeSssCode(so, so->codeDataStage3);
			}
			if (so->codeDataStage4) {
				so->codeDataStage4 = executeSssCode(so, so->codeDataStage4);
			}
			if (so->pcm == 0) {
				return;
			}
			if (_sssObjectsChanged == 0 || (so->flags & 1) == 0) {
				goto flag_case0;
			}
			goto flag_case1;
		}
	} else if ((so->flags & 1) == 0) {
		goto flag_case0;
	} else if (so->volumePtr) {
		const int volume = getSoundObjectVolumeByPos(so);
		if (volume != so->volume) {
			so->volume = volume;
			_sssObjectsChanged = 1;
			goto flag_case1;
		}
	}
	if (_sssObjectsChanged) {
		goto flag_case1;
	} else {
		goto flag_case0;
	}

flag_case1:
	setSoundObjectVolume(so);
flag_case0:
	if (so->unk2C != 0) {
		--so->unk2C;
		if ((so->flags & 2) == 0) {
			++so->unk6C;
		}
	} else {
		const int _edi = so->flags0;
		removeSoundObject(so);
		if (so->unk78 != -1) {
			LvlObject *tmp = _currentSoundLvlObject;
			_currentSoundLvlObject = so->lvlObject;
			prepareSoundObject(so->unk78, so->unk7C, _edi);
			_currentSoundLvlObject = tmp;
			return;
		}
		const uint32_t mask = 1 << (_edi >> 24);
		uint32_t *sssLut2 = _res->getSssLutPtr(2, _edi);
		if ((*sssLut2 & mask) == 0) {
			return;
		}
		SssObject *so = _sssObjectsList1;
		while (so) {
			if (compareSssLut(so->flags0, _edi)) {
                                *sssLut2 &= ~mask;
                                return;
			}
			so = so->nextPtr;
		}
		so = _sssObjectsList2;
		while (so) {
			if (compareSssLut(so->flags0, _edi)) {
                                *sssLut2 &= ~mask;
                                return;
			}
			so = so->nextPtr;
		}
	}
}

void Game::executeSssCodeOp12(int num, uint8_t lut, uint8_t c) { // expireSoundObjects
	const uint32_t mask = (1 << c);
	_res->_sssLookupTable1[lut][num] &= ~mask;
	SssObject *so = _sssObjectsList1;
	while (so) {
		if (so->unk6 == 0 && ((so->flags1 >> 20) & 15) == lut && (so->flags1 >> 24) == lut) {
			so->codeDataStage3 = 0;
			if (so->codeDataStage4 == 0) {
				removeSoundObject(so);
			}
			so->unk78 = -1;
			so->unk50 = -2;
		}
		so = so->nextPtr;
	}
	so = _sssObjectsList2;
	while (so) {
		if (so->unk6 == 0 && ((so->flags1 >> 20) & 15) == lut && (so->flags1 >> 24) == lut) {
			so->codeDataStage3 = 0;
			if (so->codeDataStage4 == 0) {
				removeSoundObject(so);
			}
			so->unk78 = -1;
			so->unk50 = -2;
		}
		so = so->nextPtr;
	}
	while (_sssObjectsCount > 0 && _sssObjectsTable[_sssObjectsCount - 1].pcm == 0) {
		--_sssObjectsCount;
	}
}

void Game::executeSssCodeOp17(SssObject *so) {
	if ((so->flags & 2) == 0) {
		SssPcm *pcm = so->pcm;
		SssObject *prev = so; // _edx
		SssObject *next = so; // _eax
		so->pcm = 0;
		if ((so->flags & 1) != 0) {
			if (next) {
				next->prevPtr = prev;
			}
			if (prev) {
				prev->nextPtr = next;
			}
			_sssObjectsList1 = next;
			--_snd_fadeVolumeCounter;
			if (so == _sssObjectsList3 || (_snd_fadeVolumeCounter < _snd_volumeMin && _sssObjectsList3)) {
				SssObject *prev3 = 0; // _esi
				_sssObjectsList3 = 0;
				if (_snd_fadeVolumeCounter >= _snd_volumeMin && _sssObjectsList1) {
					for (SssObject *cur = _sssObjectsList1; cur; cur = cur->nextPtr) {
						if (prev3 && prev3->unk9 <= cur->unk9) {
							continue;
						}
						prev3 = cur;
						_sssObjectsList2 = prev3;
					}
				}
			}
		} else {
			if (next) {
				next->prevPtr = prev;
			}
			if (prev) {
				prev->nextPtr = next;
			} else {
				_sssObjectsList2 = next;
			}
		}
		so->pcm = pcm;
		so->flags = (so->flags & ~1) | 2;
		duplicateSoundObject(so);
	}
}

void Game::executeSssCodeOp4(uint32_t flags) {
	const uint32_t mask = 1 << (flags >> 24);
	*_res->getSssLutPtr(1, flags) &= ~mask;
	SssObject *so = _sssObjectsList1;
	while (so) {
		if ((so->flags1 & 0xFFFF0FFF) == 0) {
			so->codeDataStage3 = 0;
			if (so->codeDataStage4 == 0) {
				removeSoundObject(so);
			}
			so->unk78 = -1;
			so->unk50 = -2;
		}
		so = so->nextPtr;
	}
	so = _sssObjectsList2;
	while (so) {
		if ((so->flags1 & 0xFFFF0FFF) == 0) {
			so->codeDataStage3 = 0;
			if (so->codeDataStage4 == 0) {
				removeSoundObject(so);
			}
			so->unk78 = -1;
			so->unk50 = -2;
		}
		so = so->nextPtr;
	}
	while (_sssObjectsCount > 0 && _sssObjectsTable[_sssObjectsCount - 1].pcm == 0) {
		--_sssObjectsCount;
	}
}

const uint8_t *Game::executeSssCode(SssObject *so, const uint8_t *code) {
	while (1) {
		debug(kDebug_SOUND, "executeSssCode() code %d", *code);
		switch (*code) {
		case 0:
			return 0;
		case 2: // start_sound
			if (so->unk50 >= -1) {
				LvlObject *tmp = _currentSoundLvlObject;
				_currentSoundLvlObject = so->lvlObject;
				prepareSoundObject(READ_LE_UINT16(code + 2), code[1], so->flags0);
				_currentSoundLvlObject = tmp;
			}
			code += 4;
			if (so->pcm == 0) {
				return code;
			}
			break;
		case 4: {
			// TODO: _ebx is (un)initialized with var4...
				const uint8_t _cl = code[1] & 0xF;
			// _eax = so->flags0 & 0xFF00F000;
			// _edx = (so->flags0 ^ _ebx) & 0x00F00000;
			// _ebx ^= _edx;
				const uint16_t _dx = READ_LE_UINT16(code + 2);
			// _ebx &= 0xF00000;
			// _edx &= 0xFFF;
			// _eax ^= _ebx;
			// _eax |= _ecx << 16;
			// _eax |= _edx;

				uint32_t flags = (so->flags0 & 0xFFF0F000);
				flags |= (_cl << 16) | (_dx & 0xFFF);
				executeSssCodeOp4(flags);
				code += 4;
			}
			break;
#if 0
		case 5: {
				int32_t _eax = READ_LE_UINT32(code + 4);
				if (so->unk6C < _eax) {
					so->unk6C = _eax;
					if (so->pcm) {
						const int16_t *ptr = so->pcm->ptr;
						if (ptr) {
							so->currentPcmPtr = ptr + READ_LE_UINT32(code + 8);
						}
					}
				}
				code += 12;
			}
			break;
		case 6: { // jump_ge
				--so->counter;
				if (so->counter < 0) {
					code += 8;
				} else {
					uint32_t offset = READ_LE_UINT32(code + 4);
					code -= offset;
				}
			}
			break;
#endif
		case 8: { // seek_sound
				int _eax = READ_LE_UINT32(code + 12);
				if (so->unk6C <= _eax) {
					return code;
				}
				--so->unk4C;
				if (so->unk4C < 0) {
					code += 16;
				} else {
					so->unk6C = READ_LE_UINT32(code + 8);
					if (so->pcm) {
						const int16_t *ptr = so->pcm->ptr;
						if (ptr) {
							so->currentPcmPtr = ptr + READ_LE_UINT32(code + 4);
						}
					}
					return code;
				}
			}
			break;
#if 0
		case 9: { // adjust_volume
				so->unk64 += so->unk68;
				const int volume = (so->unk64 + 0x8000) >> 16;
				if (volume != so->volume) {
					so->volume = volume;
					_sssObjectsChanged = 1;
				}
				--so->unk58;
				if (so->unk58 >= 0) {
					return code;
				}
				code += 4;
			}
		case 10: {
				// TODO:
				warning("executeSssCode case 10 unimplemented");
				code += 4;
			}
			break;
		case 11: {
				if (so->unk18 != code[1]) {
					so->unk18 = code[1];
					_sssObjectsChanged = 1;
				}
				code += 4;
			}
			break;
#endif
		case 12: { // fade_out_sound
				uint32_t _eax =  so->flags1 >> 24;
				uint32_t _edx = (so->flags1 >> 20) & 0xF;
				uint16_t _ecx = READ_LE_UINT16(code + 2);
				executeSssCodeOp12(_ecx, _edx, _eax);
				code += 4;
			}
			break;
#if 0
		case 13: {
				// TODO:
				warning("executeSssCode case 13 unimplemented");
				code += 8;
			}
			break;
		case 14: {
				// TODO:
				warning("executeSssCode case 14 unimplemented");
				code += 12;
			}
			break;
		case 16: { // stop_sound
				--so->unk4C;
				if (so->unk4C >= 0) {
					return code;
				}
				// executeSssCodeOp16(so);
				code += 4;
				if (so->pcm == 0) {
					return code;
				}
				_sssObjectsChanged = 1;
			}
			break;
#endif
		case 17: { // fade_in_sound
				executeSssCodeOp17(so);
				so->unk4C = READ_LE_UINT32(code + 4);
				return code + 8;
			}
			break;
#if 0
		case 18: {
				if (so->counter < 0) {
					so->counter = READ_LE_UINT32(code + 4);
				}
				code += 8;
			}
			break;
		case 19: { // set_volume
				if (so->volume != code[1]) {
					so->volume = code[1];
					_sssObjectsChanged = 1;
				}
				code += 4;
			}
			break;
#endif
		case 20: {
				so->unk4C = READ_LE_UINT16(code + 2);
				code += 4;
			}
			break;
#if 0
		case 21: { // dec_unk50
				--so->unk50;
				if (so->unk50 >= 0) {
					return code;
				}
				code += 4;
			}
			break;
		case 22: { // load_unk50
				so->unk50 = READ_LE_UINT32(code + 4);
				return code + 8;
			}
			break;
		case 23: { // dec_unk54
				--so->unk54;
				if (so->unk54 >= 0) {
					return code;
				}
				code += 4;
			}
			break;
		case 24: { // load_unk54
				so->unk54 = READ_LE_UINT32(code + 4);
				return code + 8;
			}
		case 25: { // dec_unk58
				--so->unk58;
				if (so->unk58 >= 0) {
					return code;
				}
				code += 4;
			}
			break;
		case 26: { // load_unk58
				so->unk58 = READ_LE_UINT32(code + 4);
				return code + 8;
			}
			break;
#endif
		case 27: {
				int _eax = READ_LE_UINT32(code + 12);
				if (so->unk6C <= _eax) {
					return code;
				}
				so->unk6C = _eax;
				if (so->pcm) {
					const int16_t *ptr = so->pcm->ptr;
					if (ptr) {
						so->currentPcmPtr = ptr + READ_LE_UINT32(code + 4);
					}
				}
				return code;
			}
			break;
#if 0
		case 28: { // jump
				uint32_t offset = READ_LE_UINT32(code + 4);
				code -= offset;
			}
			break;
		case 29:
			so->unk2C = 0;
			return 0;
#endif
		default:
			error("Invalid .sss opcode %d", *code);
			break;
		}
	}
	return code;
}

void Game::duplicateSoundObject(SssObject *so) {
	if (so->pcm && so->pcm->ptr) {
		so->flags = (so->flags & ~1) | 2;
	}
	if (so->flags & 2) {
		so->prevPtr = 0;
		so->nextPtr = _sssObjectsList2;
		if (_sssObjectsList2) {
			_sssObjectsList2->prevPtr = so;
		}
		_sssObjectsList2 = so;
	} else {
// 429185
		SssObject *so2 = so; // _edi
		if (so->pcm && so->pcm->ptr) {
			if (_snd_fadeVolumeCounter >= _snd_volumeMin) {
				SssObject *cur = _sssObjectsList3; // _eax
				if (so->unk9 > cur->unk9) {
					so2 = cur;
					so->nextPtr = cur->nextPtr;
					so->prevPtr = _sssObjectsList3->prevPtr;
					if (so->nextPtr) {
						so->nextPtr->prevPtr = so;
					}
					if (so->prevPtr) {
						so->prevPtr->nextPtr = so;
// 429281
// TODO
					} else {
						_sssObjectsList1 = so;
// 42927B
// TODO
					}
				}
			} else {
// 4291E8
				++_snd_fadeVolumeCounter;
				so->nextPtr = _sssObjectsList1;
				so->prevPtr = 0;
				if (_sssObjectsList1) {
					_sssObjectsList1->prevPtr = so;
				}
				_sssObjectsList1 = so;

				if (_snd_fadeVolumeCounter < _snd_volumeMin) {
					_sssObjectsList3 = 0;
				} else {
					if (_sssObjectsList3 == 0) {
						_sssObjectsList3 = 0;
						// if (so == 0) goto 4292BE
					}
// 429269

				}

// 4292BE
			}
		}
// 4292C2
		if (so2) {
			so2->flags &= ~1;
			so2->pcm = 0;
			killSoundObject(so2->flags0);
		}
	}
// 4292DF
	if (so->num >= _sssObjectsCount) {
		_sssObjectsCount = so->num + 1;
	}
}

void Game::killSoundObject(uint32_t flags) {
	uint32_t mask = 1 << (flags >> 24);
	uint32_t *sssLut = _res->getSssLutPtr(2, flags);
	if ((*sssLut & mask) != 0) {
		SssObject *so = _sssObjectsList1;
		while (so) {
			if (compareSssLut(so->flags0, flags)) {
				return;
			}
			so = so->nextPtr;
		}
		so = _sssObjectsList2;
		while (so) {
			if (compareSssLut(so->flags0, flags)) {
				return;
			}
			so = so->nextPtr;
		}
// 42AC87
		*sssLut &= ~mask;
	}
}

void Game::prepareSoundObject(int num, int b, int c) {
	debug(kDebug_SOUND, "prepareSoundObject num %d b %d c 0x%x", num, b, c);
	if (b > 0) {
		if ((_res->_sssDataUnk3[num].unk1 & 1) != 0) {
			// int var8 = _res->_sssDataUnk3[num].sssFilter
			if (_res->_sssDataUnk3[num].unk0 <= 0) {
				return; // 0;
			}
// 42B81D
		}
// 42B865:
		// TODO:
		warning("prepareSoundObject b %d unimplemented", b);
	} else {
		SssObject *so = startSoundObject(num, b, c);
		if (so && (_res->_sssDataUnk3[num].unk0 & 4) != 0) {
			so->unk7C = b;
			so->unk78 = num;
		}
	}
}

SssObject *Game::startSoundObject(int num, int b, int flags) {
	debug(kDebug_SOUND, "startSoundObject num %d b %d flags 0x%x", num, b, flags);

	int codeOffset = _res->_sssDataUnk3[num].firstCodeOffset;
	debug(kDebug_SOUND, "startSoundObject codeOffset %d", codeOffset);
	assert(codeOffset < _res->_sssHdr.codeOffsetsCount);
	if (_res->_sssCodeOffsets[codeOffset].unk2 != 0) {
// 42B64C
//		return 0;
	}

	SssObject tmpObj;
	memset(&tmpObj, 0, sizeof(tmpObj));
	tmpObj.flags0 = flags;
	tmpObj.flags1 = flags;
	tmpObj.unk6 = num;
	tmpObj.counter = -1;
	tmpObj.unk4C = -1;
	tmpObj.lvlObject = _currentSoundLvlObject;
	tmpObj.volumePtr = 0;
	debug(kDebug_SOUND, "startSoundObject dpcm %d", _res->_sssCodeOffsets[codeOffset].pcm);
	tmpObj.pcm = &_res->_sssPcmTable[_res->_sssCodeOffsets[codeOffset].pcm];
	// TEMP: mixSounds
		const int dpcm = _res->_sssCodeOffsets[codeOffset].pcm;
		_res->loadSssDpcm(dpcm);
		const int16_t *pcm = _res->_sssPcmTable[dpcm].ptr;
		if (pcm) {
			uint32_t size = _res->getSssDpcmSize(dpcm);
			assert((size & 1) == 0);
			size /= sizeof(int16_t);
			_system->lockAudio();
			bool found = false;
			for (int i = 0; i < kMixerChannelsCount; ++i) {
				if (_mixerChannels[i].pcm == pcm) {
					assert(_mixerChannels[i].size == size);
					found = true;
					break;
				}
			}
			if (!found) {
				for (int i = 0; i < kMixerChannelsCount; ++i) {
					if (!_mixerChannels[i].pcm) {
						_mixerChannels[i].pcm = pcm;
						_mixerChannels[i].size = size;
						_mixerChannels[i].offset = 0;
						debug(kDebug_SOUND, "Adding PCM %d to channel %d size %d", dpcm, i, _mixerChannels[i].size);
						break;
					}
				}
			}
			_system->unlockAudio();
		}
	//
	const uint8_t *code = PTR_OFFS<uint8_t>(_res->_sssCodeData, _res->_sssCodeOffsets[codeOffset].codeOffset1);
	debug(kDebug_SOUND, "code %p", code);
	if (code) {
		// executeSssCode(&tmpObj, code);
	}

	const uint32_t mask = 1 << (flags >> 24);
	uint32_t *sssLut2 = _res->getSssLutPtr(2, flags);
	if ((*sssLut2 & mask) != 0) {
		SssObject *so = _sssObjectsList1;
		while (so) {
			if (compareSssLut(so->flags0, flags)) {
				*sssLut2 &= ~mask;
				return 0;
			}
			so = so->nextPtr;
		}
		so = _sssObjectsList2;
		while (so) {
			if (compareSssLut(so->flags, flags)) {
				*sssLut2 &= ~mask;
				return 0;
			}
			so = so->nextPtr;
		}
		*sssLut2 &= ~mask;
	}
	return 0;
}

void Game::setupSoundObject(SssUnk1 *s, int a, int b) {
	debug(kDebug_SOUND, "setupSound num %d a 0x%x b 0x%x", s->sssUnk3, a, b);
	const int num = _res->_sssDataUnk3[s->sssUnk3].sssFilter;
	debug(kDebug_SOUND, "sssFilter num %d", num);
	SssFilter *filter = &_res->_sssFilters[num];
	bool found = false;
	for (int i = 0; i < _sssObjectsCount; ++i) {
		SssObject *so = &_sssObjectsTable[i];
		if (so->pcm != 0 && so->filter == filter) {
			found = true;
			break;
		}
	}
	int _ecx = filter->unk4;
	int _eax = ((int8_t)s->unk3) << 16;
	if (_ecx != _eax) {
		if (!found) {
			filter->unk4 = _eax; // int32_t
		} else {
			filter->unkC = 4; // uint32_t
			filter->unk30 = 1; // uint32_t
			_eax = ((s->unk3 << 16) - _ecx) / 4;
			filter->unk8 = _eax; // uint32_t
		}
	}
// 42B9FD
	_eax = ((int8_t)s->unk5) << 16;
	_ecx = filter->unk14;
	if (_ecx != _eax) {
		if (!found) {
			filter->unk14 = _eax; // int32_t
		} else {
			filter->unk1C = 4;
			filter->unk30 = 1;
			_eax = ((s->unk5 << 16) - _ecx) / 4;
			filter->unk18 = _eax;
		}
	}
// 42BA37
	_eax = (int8_t)s->unk4;
	const int scale = filter->unk24;
	if (scale != _eax) {
		filter->unk24 = _eax;
		for (int i = 0; i < _sssObjectsCount; ++i) {
			SssObject *so = &_sssObjectsTable[i];
			if (so->pcm != 0 && so->filter == filter) {
				int _al = filter->unk24 & 255;
				_al += so->unk8;
				so->unk9 = _al < 0 ? 0 : _al;
				if (so->unk9 > 7) {
					so->unk9 = 7;
					fadeSoundObject(so);
				}
			}
		}
	}
// 42BA9D
	int _ebp = 0;
	int _edx = b << 4;
	int _al = s->unk6;
	_ebp = (a & 0xF) | _edx;
	_edx = s->unk2 & 0xF;
	_ecx = s->sssUnk3;
	_ebp <<= 4;
	_ebp |= _edx;
	_edx = _al;
	_ebp <<= 4;
	_edx &= 0xF;
	_ebp |= _edx;
	_edx = _ecx;
	_ebp <<= 0xC;
	_edx &= 0xFFF;
	_ebp |= _edx;
#if 0
	if (_al & 2) {
		_eax = (_ebp >> 20) & 15;
		_edx = sssLookupTable3[_eax];
		_esi = _edx + _ecx * 4;
		_ecx = _ebp >> 24
		_edi = *(uint32_t *)_esi;
		_edx = 1 << _ecx;
		_ecx = 0;
		_edi |= _edx;
		*(uint32_t *)_esi = _edi;
		_cx = *(uint16_t *)_ebx;
		_eax = sssLookupTable2[_eax];
		_ecx = _eax + _ecx * 4;
		_eax = *(uint32_t *)_ecx;
		if ((_eax & _edx) != 0) {
			return 0;
		}
		_eax |= _edx;
		*(uint32_t *)_ecx = _eax;
		goto 42BBDD
// 42BB26
	} else if ((_al & 1) != 0) {
		_eax = _ebp;
		_edx >>= 20;
		_edx &= 15;
		_eax = sssLookupTable1[_edx];
		_edx = _eax + _ecx * 4;
		_ecx =_ebp;
		ecx >>= 24;
		_eax = 1 << _ecx;
		_ecx = *(uint32_t *)edx;
		if ((_ecx & _eax) != 0) {
			return 0;
		}
		_ecx |= _eax;
		*(uint32_t *)_edx = _ecx;
		goto 42BBDD
// 42BB60
	} else if ((_al & 4) != 0) {
		_esi = (_ebp >> 20) & 15;
		_edi = _ebp >> 24;
		_eax = _sssObjectsList1;
		while (_eax != 0) {
			_edx = _eax->flags0;
			_ebx = _edx & 0xFFF;
			if (_ebx == _ecx) {
				_ebx = (_edx >> 20) & 15;
				if (_ebx == _esi) {
					_edx >>= 24;
					if (_edx == _edi) {
						_ebx = s;
						goto 42BBDD
				}
			}
			_eax = _eax->nextPtr;
		}
		_ebx = s;
		_eax = _sssObjectsList2;
		while (_eax != 0) {
			_edx = _eax->flags0;
			_ebx = _edx & 0xFFF;
			if (_ebx == _ecx) {
				_ebx = (_edx >> 20) & 15;
				if (_ebx == _esi) {
					_edx >>= 24;
					if (_edx == _edi) {
						_ebx = s;
						goto 42BBDD;
				}
			}
			_eax = _eax->nextPtr;
		}
	}
#endif
// 42BBDD
	prepareSoundObject(s->sssUnk3, (int8_t)s->unk2, _ebp);
}

void Game::clearSoundObjects() {
	memset(_sssObjectsTable, 0, sizeof(_sssObjectsTable));
	_sssObjectsList1 = 0;
	_sssObjectsList2 = 0;
	_sssObjectsList3 = 0;
	for (size_t i = 0; i < ARRAYSIZE(_sssObjectsTable); ++i) {
		_sssObjectsTable[i].num = i;
	}
	_sssObjectsCount = 0;
	_snd_fadeVolumeCounter = 0;
	// _snd_mixingQueueSize = 0;
	if (_res->_sssHdr.dataUnk1Count != 0) {
		const int size = _res->_sssHdr.dataUnk3Count * 4;
		for (int i = 0; i < 3; ++i) {
			memset(_res->_sssLookupTable1[i], 0, size);
			memset(_res->_sssLookupTable2[i], 0, size);
			memset(_res->_sssLookupTable3[i], 0, size);
		}
	}
	// TODO:
}

void Game::fadeSoundObject(SssObject *so) {
	if ((so->flags & 2) == 0) {
		_sssObjectsList3 = 0;
		if (_snd_fadeVolumeCounter >= _snd_volumeMin) {
			so = 0;
			SssObject *cur = _sssObjectsList1;
			for (; cur; cur = cur->nextPtr) {
				if (so) {
					if (so->unk9 <= cur->unk9) {
						continue;
					}
				}
				so = cur;
				_sssObjectsList2 = cur;
			}
		}
	}
}

int Game::getSoundObjectVolumeByPos(SssObject *so) const {
	LvlObject *obj = so->lvlObject;
	if (obj) {
		switch (obj->type) {
		case 8:
		case 2:
		case 0:
			if (obj->screenNum == _currentLeftScreen) {
				return -1;
			}
			if (obj->screenNum == _currentRightScreen) {
				return 129;
			}
			if (obj->screenNum == _currentScreen || (_currentLevel == 7 && obj->data0x2988 == 27) || (_currentLevel == 3 && obj->data0x2988 == 26)) {
				const int dist = (obj->xPos + obj->width / 2) / 2;
				return CLIP(dist, 0, 128);
			}
			// fall-through
		default:
			return -2;
		}
	}
	return so->volume;
}

void Game::setSoundObjectVolume(SssObject *so) {
	if ((so->flags & 2) == 0 && so->unk18 != 0 && _snd_masterVolume != 0) {
		int volume = so->filter->unk4;
		volume = ((volume >> 16) * so->unk18) >> 7;
		int _esi = 0;
		if (so->volumePtr) {
			int _eax = so->unk8;
			_eax += so->filter->unk24;
			_esi = so->volume;
			if (_eax < 0) {
				_eax = 0;
			} else if (_eax > 7) {
				_eax = 7;
			}
			if (_esi == -2) {
				volume = 0;
				_esi = 64;
				_eax = 0;
			} else {
				if (_esi < 0) {
					_esi = 0;
				} else if (_esi > 128) {
					_esi = 128;
				}
				volume >>= 2; // _edi
				_eax /= 2;
			}
			if (so->unk9 != _eax) {
				so->unk9 = _eax;
				_sssObjectsList3 = 0;
				if (_snd_fadeVolumeCounter >= _snd_volumeMin) {
					SssObject *o = _sssObjectsList1;
					SssObject *_ebp = 0;
					while (o) {
						if (_ebp && _ebp->unk9 <= o->unk9) {
							continue;
						}
						_ebp = o;
						_sssObjectsList3 = o;
						o = o->nextPtr;
					}
				}
			}

		} else {
// 429076:
			_esi = so->filter->unk14;
			_esi = CLIP(so->volume + (_esi >> 16), 0, 128);
		}
// 429094
		if (so->pcm == 0) {
			return;
		}
		int _edx = _dbVolumeTable[volume];
		int _edi = _esi;
		int _eax = _edx >> 7;
		if (_edi == 0) {
// 4290F0
			so->panL = _eax;
			so->panR = 0;
			so->panType = 2;
		} else if (_edi == 64) {
// 4290DF
			_eax /= 2;
			so->panL = _eax;
			so->panR = _eax;
			so->panType = 3;
		} else if (_edi == 128) {
// 4290D0
			so->panL = 0;
			so->panR = _eax;
			so->panType = 1;
		} else {
			_edx *= _esi;
			_eax -= _edx;
			so->panR = _edx;
			so->panL = _eax;
			so->panType = 4;
		}
// 4290F
		so->panR = (so->panR * _snd_masterVolume + 64) >> 7;
		so->panL = (so->panL * _snd_masterVolume + 64) >> 7;
	} else {
		so->panL = 0;
		so->panR = 0;
		so->panType = 0;
	}
}

void Game::expireSoundObjects(int flags) {
	const uint32_t mask = 1 << (flags >> 24);
	uint32_t *sssLut1 = _res->getSssLutPtr(1, flags);
	*sssLut1 &= ~mask;
	uint32_t *sssLut2 = _res->getSssLutPtr(2, flags);
	*sssLut2 &= ~mask;
	SssObject *so = _sssObjectsList1;
	while (so) {
		if ((so->flags & 0xFFFF0FFF) == 0) {
			so->codeDataStage3 = 0;
			if (so->codeDataStage4 == 0) {
				removeSoundObject(so);
			}
			so->unk78 = -1;
			so->unk50 = -2;
		}
		so = so->nextPtr;
	}
	so = _sssObjectsList2;
	while (so) {
		if ((so->flags & 0xFFFF0FFF) == 0) {
			so->codeDataStage3 = 0;
			if (so->codeDataStage4 == 0) {
				removeSoundObject(so);
			}
			so->unk78 = -1;
			so->unk50 = -2;
		}
		so = so->nextPtr;
	}
	while (_sssObjectsCount > 0 && _sssObjectsTable[_sssObjectsCount - 1].pcm == 0) {
		--_sssObjectsCount;
	}
}

void Game::mixSoundObjects17640(bool flag) {
	for (int i = 0; i < _res->_sssHdr.dataUnk2Count; ++i) {
		_res->_sssFilters[i].unk30 = 0;
		const int counter1 = _res->_sssFilters[i].unkC;
		if (counter1 != 0) {
			_res->_sssFilters[i].unkC = counter1 - 1;
		}
		_res->_sssFilters[i].unk4 += _res->_sssFilters[i].unk8;
		_res->_sssFilters[i].unk30 = 1;
		const int counter2 = _res->_sssFilters[i].unk1C;
		if (counter2 != 0) {
			_res->_sssFilters[i].unk1C = counter2 - 1;
		}
		_res->_sssFilters[i].unk14 += _res->_sssFilters[i].unk18;
		_res->_sssFilters[i].unk30 = 1;
	}
// 42B426
	for (int i = 0; i < _sssObjectsCount; ++i) {
		SssObject *so = &_sssObjectsTable[i];
		if (so->pcm) {
			if (_channelMixingTable[i] != 0) {
				continue;
			}
			if (flag) {
				const int mask = 1 << (so->flags1 >> 24);
				if ((*_res->getSssLutPtr(2, so->flags1) & mask) != 0) {
					if ((*_res->getSssLutPtr(3, so->flags1) & mask) == 0) {
						expireSoundObjects(so->flags1);
					}
				}
			}
			updateSoundObject(so);
		}
	}
// 42B4B2
	memset(_channelMixingTable, 0, sizeof(_channelMixingTable));
	if (flag) {
//		clearSssBuffer2();
	}
	mixSoundObjects();
}

void Game::mixSoundObjects() {
	SssObject *so = _sssObjectsList1;
	while (so) {
		if (so->pcm != 0) {
			const int16_t *ptr = so->pcm->ptr;
			if (ptr && so->currentPcmPtr >= ptr) {
				// TODO: append to mixingQueue
			}
		}
		so = so->nextPtr;
	}
}

static int clipS16(int sample) {
	return CLIP(sample, -32768, 32767);
}

void Game::mixSoundsCb(int16_t *buf, int len) {
	// TODO: pull from mixingQueue
	for (int i = 0; i < kMixerChannelsCount; ++i) {
		MixerChannel *ch = &_mixerChannels[i];
		if (ch->pcm) {
			for (int offset = 0; offset < len; ++offset) {
				if (ch->offset < ch->size) {
					buf[offset] = clipS16((int)buf[offset] + ch->pcm[ch->offset]);
					ch->offset++;
				} else {
					debug(kDebug_SOUND, "PCM sample finished size %d", ch->size);
					memset(ch, 0, sizeof(MixerChannel));
					break;
				}
			}
		}
	}
}
