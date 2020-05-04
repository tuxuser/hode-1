
#include "game.h"
#include "menu.h"
#include "lzw.h"
#include "paf.h"
#include "resource.h"
#include "system.h"
#include "util.h"
#include "video.h"

enum {
	kTitleScreen_AssignPlayer = 0,
	kTitleScreen_Play = 1,
	kTitleScreen_Options = 2,
	kTitleScreen_Quit = 3,
	kTitleScreenPSX_Load = 3, // PSX does not have 'Quit'
	kTitleScreenPSX_Save = 4
};

enum {
	kMenu_NewGame = 0,
	kMenu_CurrentGame = 2,
	kMenu_Load = 4,
	kMenu_ResumeGame = 7,
	kMenu_Settings = 8,
	kMenu_Quit = 15,
	kMenu_Cutscenes = 17
};

enum {
	kSound_0x60 = 0x60 / 8, // test sound
	kSound_0x70 = 0x70 / 8, // move cursor
	kSound_0x78 = 0x78 / 8, // select
	kSound_0x80 = 0x80 / 8, // return
	kSound_0x88 = 0x88 / 8, // reset sound setting
	kSound_0x90 = 0x90 / 8, // increase/decrease volume
	kSound_0x98 = 0x98 / 8,
	kSound_0xA0 = 0xA0 / 8  // background sounds
};

enum {
	kCursor_Select = 0,
	kCursor_Left = 1,
	kCursor_Right = 2,
	kCursor_Up = 3,
	kCursor_Down = 4
};

enum {
	kSettingNum_Controls = 0,
	kSettingNum_Difficulty = 1,
	kSettingNum_Sound = 2,
	kSettingNum_Confirm = 3
};

enum {
	kSoundNum_Stereo = 0,
	kSoundNum_Volume = 1,
	kSoundNum_Confirm = 2,
	kSoundNum_Test = 3,
	kSoundNum_Cancel = 4,
	kSoundNum_Reset = 5
};

static void setDefaultsSetupCfg(SetupConfig *config, int num) {
	assert(num >= 0 && num < 4);
	memset(config->players[num].progress, 0, 10);
	config->players[num].levelNum = 0;
	config->players[num].checkpointNum = 0;
	config->players[num].cutscenesMask = 0;
	memset(config->players[num].controls, 0, 32);
	config->players[num].controls[0x0] = 0x11;
	config->players[num].controls[0x4] = 0x22;
	config->players[num].controls[0x8] = 0x84;
	config->players[num].controls[0xC] = 0x48;
	config->players[num].difficulty = 1;
	config->players[num].stereo = 1;
	config->players[num].volume = Game::kDefaultSoundVolume;
	config->players[num].lastLevelNum = 0;
}

static bool isEmptySetupCfg(SetupConfig *config, int num) {
	return config->players[num].levelNum == 0 && config->players[num].checkpointNum == 0 && config->players[num].cutscenesMask == 0;
}

Menu::Menu(Game *g, PafPlayer *paf, Resource *res, Video *video)
	: _g(g), _paf(paf), _res(res), _video(video) {

	_config = &_g->_setupConfig;
}

void Menu::setVolume() {
	const int volume = _config->players[_config->currentPlayer].volume;
	if (volume != _g->_snd_masterVolume) {
		_g->_snd_masterVolume = volume;
		if (volume == 0) {
			_g->muteSound();
		} else {
			_g->unmuteSound();
		}
	}
}

static uint32_t readBitmapsGroup(int count, DatBitmapsGroup *bitmapsGroup, uint32_t ptrOffset, int paletteSize) {
	const uint32_t baseOffset = ptrOffset;
	for (int i = 0; i < count; ++i) {
		int size;
		if (paletteSize == 0) { // PSX
			size = le32toh(bitmapsGroup[i].offset);
		} else {
			size = bitmapsGroup[i].w * bitmapsGroup[i].h;
		}
		bitmapsGroup[i].offset = ptrOffset - baseOffset;
		bitmapsGroup[i].palette = bitmapsGroup[i].offset + size;
		ptrOffset += size + paletteSize;
	}
	return ptrOffset - baseOffset;
}

static uint32_t readSoundData(uint8_t *soundData, uint32_t soundDataSize) {
	const int soundListsCount = READ_LE_UINT32(soundData + 4);
	const uint8_t *listData = soundData + 8 + soundListsCount * 8;
	for (int i = 0; i < soundListsCount; ++i) {
		WRITE_LE_UINT32(soundData + 8 + i * 8, listData - soundData);
		const int count = READ_LE_UINT32(soundData + 8 + i * 8 + 4);
		listData += count * sizeof(uint16_t);
	}
	assert((uint32_t)(listData - soundData) == soundDataSize);
	return soundDataSize;
}

void Menu::loadData() {
	_g->_mix._lock(1);
	_res->loadDatMenuBuffers();
	_g->clearSoundObjects();
	_g->_mix._lock(0);

	const int version = _res->_datHdr.version;

	const int paletteSize = _res->_isPsx ? 0 : 256 * 3;

	const uint8_t *ptr = _res->_menuBuffer1;
	uint32_t hdrOffset, ptrOffset = 0;

	if (version == 10) {

		_titleSprites = (DatSpritesGroup *)(ptr + ptrOffset);
		_titleSprites->size = le32toh(_titleSprites->size);
		_titleSprites->count = le16toh(_titleSprites->count);
		ptrOffset += sizeof(DatSpritesGroup) + _titleSprites->size;
		_titleSprites->firstFrameOffset = 0;

		_playerSprites = (DatSpritesGroup *)(ptr + ptrOffset);
		_playerSprites->size = le32toh(_playerSprites->size);
		_playerSprites->count = le16toh(_playerSprites->count);
		ptrOffset += sizeof(DatSpritesGroup) + _playerSprites->size;
		_playerSprites->firstFrameOffset = 0;

		_titleBitmapSize = READ_LE_UINT32(ptr + ptrOffset);
		_titleBitmapData = ptr + ptrOffset + sizeof(DatBitmap);
		ptrOffset += sizeof(DatBitmap) + _titleBitmapSize + paletteSize;

		_playerBitmapSize = READ_LE_UINT32(ptr + ptrOffset);
		_playerBitmapData = ptr + ptrOffset + sizeof(DatBitmap);
		ptrOffset += sizeof(DatBitmap) + _playerBitmapSize + paletteSize;

		const int size = READ_LE_UINT32(ptr + ptrOffset); ptrOffset += 4;
		assert((size % (16 * 10)) == 0);
		_digitsData = ptr + ptrOffset;
		ptrOffset += size;

		const int cutscenesCount = _res->_datHdr.cutscenesCount;
		_cutscenesBitmaps = (DatBitmapsGroup *)(ptr + ptrOffset);
		ptrOffset += cutscenesCount * sizeof(DatBitmapsGroup);
		_cutscenesBitmapsData = ptr + ptrOffset;
		ptrOffset += readBitmapsGroup(cutscenesCount, _cutscenesBitmaps, ptrOffset, paletteSize);

		for (int i = 0; i < kCheckpointLevelsCount; ++i) {
			DatBitmapsGroup *bitmapsGroup = (DatBitmapsGroup *)(ptr + ptrOffset);
			_checkpointsBitmaps[i] = bitmapsGroup;
			const int count = _res->_datHdr.levelCheckpointsCount[i];
			ptrOffset += count * sizeof(DatBitmapsGroup);
			_checkpointsBitmapsData[i] = ptr + ptrOffset;
			ptrOffset += readBitmapsGroup(count, bitmapsGroup, ptrOffset, paletteSize);
		}

		_soundData = ptr + ptrOffset;
		readSoundData(_res->_menuBuffer1 + ptrOffset, _res->_datHdr.soundDataSize);
		ptrOffset += _res->_datHdr.soundDataSize;

	} else if (version == 11) {

		hdrOffset = 4;

		ptrOffset = 4 + (2 + kOptionsCount) * sizeof(DatBitmap);
		ptrOffset += _res->_datHdr.cutscenesCount * sizeof(DatBitmapsGroup);
		for (int i = 0; i < kCheckpointLevelsCount; ++i) {
			ptrOffset += _res->_datHdr.levelCheckpointsCount[i] * sizeof(DatBitmapsGroup);
		}
		ptrOffset += _res->_datHdr.levelsCount * sizeof(DatBitmapsGroup);

		_titleBitmapSize = READ_LE_UINT32(ptr + hdrOffset);
		hdrOffset += sizeof(DatBitmap);
		_titleBitmapData = ptr + ptrOffset;
		ptrOffset += _titleBitmapSize + paletteSize;

		_playerBitmapSize = READ_LE_UINT32(ptr + hdrOffset);
		hdrOffset += sizeof(DatBitmap);
		_playerBitmapData = ptr + ptrOffset;
		ptrOffset += _playerBitmapSize + paletteSize;

		for (int i = 0; i < kOptionsCount; ++i) {
			_optionsBitmapSize[i] = READ_LE_UINT32(ptr + hdrOffset);
			hdrOffset += sizeof(DatBitmap);
			if (_optionsBitmapSize[i] != 0) {
				_optionsBitmapData[i] = ptr + ptrOffset;
				ptrOffset += _optionsBitmapSize[i] + paletteSize;
			} else {
				_optionsBitmapData[i] = 0;
			}
		}

		const int cutscenesCount = _res->_datHdr.cutscenesCount;
		_cutscenesBitmaps = (DatBitmapsGroup *)(ptr + hdrOffset);
		_cutscenesBitmapsData = ptr + ptrOffset;
		hdrOffset += cutscenesCount * sizeof(DatBitmapsGroup);
		ptrOffset += readBitmapsGroup(cutscenesCount, _cutscenesBitmaps, ptrOffset, paletteSize);

		for (int i = 0; i < kCheckpointLevelsCount; ++i) {
			DatBitmapsGroup *bitmapsGroup = (DatBitmapsGroup *)(ptr + hdrOffset);
			_checkpointsBitmaps[i] = bitmapsGroup;
			_checkpointsBitmapsData[i] = ptr + ptrOffset;
			const int count = _res->_datHdr.levelCheckpointsCount[i];
			hdrOffset += count * sizeof(DatBitmapsGroup);
			ptrOffset += readBitmapsGroup(count, bitmapsGroup, ptrOffset, paletteSize);
		}

		const int levelsCount = _res->_datHdr.levelsCount;
		_levelsBitmaps = (DatBitmapsGroup *)(ptr + hdrOffset);
		_levelsBitmapsData = ptr + ptrOffset;
		ptrOffset += readBitmapsGroup(levelsCount, _levelsBitmaps, ptrOffset, paletteSize);
	}

	ptr = _res->_menuBuffer0;
	ptrOffset = 0;

	if (version == 11) {

		_titleSprites = (DatSpritesGroup *)(ptr + ptrOffset);
		_titleSprites->size = le32toh(_titleSprites->size);
		ptrOffset += sizeof(DatSpritesGroup) + _titleSprites->size;
		_titleSprites->count = le16toh(_titleSprites->count);
		_titleSprites->firstFrameOffset = 0;

		_playerSprites = (DatSpritesGroup *)(ptr + ptrOffset);
		_playerSprites->size = le32toh(_playerSprites->size);
		ptrOffset += sizeof(DatSpritesGroup) + _playerSprites->size;
		_playerSprites->count = le16toh(_playerSprites->count);
		_playerSprites->firstFrameOffset = 0;

		_optionData = ptr + ptrOffset;
		ptrOffset += _res->_datHdr.menusCount * 8;

		const int size = READ_LE_UINT32(ptr + ptrOffset); ptrOffset += 4;
		assert((size % (16 * 10)) == 0);
		_digitsData = ptr + ptrOffset;
		ptrOffset += size;

		_soundData = ptr + ptrOffset;
		readSoundData(_res->_menuBuffer0 + ptrOffset, _res->_datHdr.soundDataSize);
		ptrOffset += _res->_datHdr.soundDataSize;
	}

	if (_res->_isPsx) {
		return;
	}

	hdrOffset = ptrOffset;
	_iconsSprites = (DatSpritesGroup *)(ptr + ptrOffset);
	const int iconsCount = _res->_datHdr.iconsCount;
	ptrOffset += iconsCount * sizeof(DatSpritesGroup);
	_iconsSpritesData = ptr + ptrOffset;
	for (int i = 0; i < iconsCount; ++i) {
		_iconsSprites[i].size = le32toh(_iconsSprites[i].size);
		_iconsSprites[i].count = le16toh(_iconsSprites[i].count);
		_iconsSprites[i].firstFrameOffset = ptr + ptrOffset - _iconsSpritesData;
		ptrOffset += _iconsSprites[i].size;
	}

	_optionsButtonSpritesCount = READ_LE_UINT32(ptr + ptrOffset); ptrOffset += 4;
	if (_optionsButtonSpritesCount != 0) {
		_optionsButtonSpritesData = ptr + ptrOffset;
		hdrOffset = ptrOffset;
		ptrOffset += _optionsButtonSpritesCount * 20;
		for (int i = 0; i < _optionsButtonSpritesCount; ++i) {
			DatSpritesGroup *spritesGroup = (DatSpritesGroup *)(ptr + hdrOffset + 4);
			spritesGroup->size = le32toh(spritesGroup->size);
			spritesGroup->count = le16toh(spritesGroup->count);
			spritesGroup->firstFrameOffset = ptr + ptrOffset - _optionsButtonSpritesData;
			hdrOffset += 20;
			ptrOffset += spritesGroup->size;
		}
	} else {
		_optionsButtonSpritesData = 0;
	}

	if (version == 10) {

		_optionData = ptr + ptrOffset;
		ptrOffset += _res->_datHdr.menusCount * 8;

		hdrOffset = ptrOffset;
		ptrOffset += kOptionsCount * sizeof(DatBitmap);
		for (int i = 0; i < kOptionsCount; ++i) {
			_optionsBitmapSize[i] = READ_LE_UINT32(ptr + hdrOffset);
			hdrOffset += sizeof(DatBitmap);
			if (_optionsBitmapSize[i] != 0) {
				_optionsBitmapData[i] = ptr + ptrOffset;
				ptrOffset += _optionsBitmapSize[i] + paletteSize;
			} else {
				_optionsBitmapData[i] = 0;
			}
		}

		const int levelsCount = _res->_datHdr.levelsCount;
		hdrOffset = ptrOffset;
		ptrOffset += levelsCount * sizeof(DatBitmapsGroup);
		_levelsBitmaps = (DatBitmapsGroup *)(ptr + hdrOffset);
		_levelsBitmapsData = ptr + ptrOffset;
		readBitmapsGroup(levelsCount, _levelsBitmaps, ptrOffset, paletteSize);
	}
}

int Menu::getSoundNum(int num, int index) const {
	const int soundListsCount = READ_LE_UINT32(_soundData + 4);
	if (num < soundListsCount) {
		const int count = READ_LE_UINT32(_soundData + 8 + num * 8 + 4);
		assert(index < count);
		const uint8_t *data = _soundData + READ_LE_UINT32(_soundData + 8 + num * 8);
		return (int16_t)READ_LE_UINT16(data + index * 2);
	}
	return -1;
}

void Menu::playSound(int num) {
	num = getSoundNum(num);
	debug(kDebug_MENU, "playSound %d", num);
	if (num != -1) {
		_g->playSound(num, 0, 0, 5);
	}
}

void Menu::drawSprite(const DatSpritesGroup *spriteGroup, const uint8_t *ptr, uint32_t num, int x, int y) {
	ptr += spriteGroup->firstFrameOffset;
	for (uint32_t i = 0; i < spriteGroup->count; ++i) {
		const uint16_t size = READ_LE_UINT16(ptr + 2);
		if (num == i) {
			if (_res->_isPsx) {
				_video->decodeBackgroundOverlayPsx(ptr);
			} else {
				if (x < 0) {
					x = ptr[0];
				}
				if (y < 0) {
					y = ptr[1];
				}
				_video->decodeSPR(ptr + 8, _video->_frontLayer, x, y, 0, READ_LE_UINT16(ptr + 4), READ_LE_UINT16(ptr + 6));
			}
			break;
		}
		ptr += size + 2;
	}
}

void Menu::drawSpriteAnim(DatSpritesGroup *spriteGroup, const uint8_t *ptr, uint32_t num) {
	if (spriteGroup[num].num == 0) {
		spriteGroup[num].currentFrameOffset = spriteGroup[num].firstFrameOffset;
	}
	ptr += spriteGroup[num].currentFrameOffset;
	if (_res->_isPsx) {
		_video->decodeBackgroundOverlayPsx(ptr);
	} else {
		_video->decodeSPR(ptr + 8, _video->_frontLayer, ptr[0], ptr[1], 0, READ_LE_UINT16(ptr + 4), READ_LE_UINT16(ptr + 6));
	}
	++spriteGroup[num].num;
	if (spriteGroup[num].num < spriteGroup[num].count) {
		const uint16_t size = READ_LE_UINT16(ptr + 2);
		spriteGroup[num].currentFrameOffset += size + 2;
	} else {
		spriteGroup[num].num = 0; // restart from frame #0
		spriteGroup[num].currentFrameOffset = spriteGroup[num].firstFrameOffset;
	}
}

void Menu::refreshScreen(bool updatePalette) {
	if (updatePalette) {
		g_system->setPalette(_paletteBuffer, 256, 6);
	}
	_video->updateGameDisplay(_video->_frontLayer);
	g_system->updateScreen(false);
}

void Menu::pafCallback(int frameNum, const uint8_t *frameData) {
	if (_currentOptionButtonSound != 0) {
		const int num = getSoundNum(_currentOptionButtonSound, frameNum);
		if (num != -1) {
			_g->playSound(num, 0, 0, 5);
		}
	}
	if (_currentOptionButtonSprite && frameNum == _currentOptionButtonSprite->num) {
		memcpy(_video->_frontLayer, frameData, Video::W * Video::H);
		drawSpriteAnim(_currentOptionButtonSprite, _optionsButtonSpritesData, 0);
		g_system->copyRect(0, 0, Video::W, Video::H, _video->_frontLayer, Video::W);
	}
}

static void menuPafCallback(void *userdata, int frame, const uint8_t *buffer) {
	((Menu *)userdata)->pafCallback(frame, buffer);
}

bool Menu::mainLoop() {
	bool ret = false;
	loadData();
	while (!g_system->inp.quit) {
		const int option = handleTitleScreen();
		if (option == kTitleScreen_AssignPlayer) {
			handleAssignPlayer();
			debug(kDebug_MENU, "currentPlayer %d", _config->currentPlayer);
		} else if (option == kTitleScreen_Play) {
			return true;
		} else if (option == kTitleScreen_Options) {
			PafCallback pafCb;
			pafCb.proc = menuPafCallback;
			pafCb.userdata = this;
			_paf->setCallback(&pafCb);
			playSound(kSound_0xA0);
			handleOptions();
			debug(kDebug_MENU, "optionNum %d", _optionNum);
			_g->resetSound();
			_paf->setCallback(0);
			if (_optionNum == kMenu_NewGame + 1 || _optionNum == kMenu_CurrentGame + 1 || _optionNum == kMenu_ResumeGame) {
				ret = true;
				break;
			} else if (_optionNum == kMenu_Quit + 1) {
				break;
			}
		} else if (option == kTitleScreen_Quit) {
			break;
		}
	}
	_res->unloadDatMenuBuffers();
	return ret;
}

void Menu::drawTitleScreen(int option) {
	if (_res->_isPsx) {
		_video->decodeBackgroundPsx(_titleBitmapData, _titleBitmapSize, Video::W, Video::H);
	} else {
		decodeLZW(_titleBitmapData, _video->_frontLayer);
		g_system->setPalette(_titleBitmapData + _titleBitmapSize, 256, 6);
	}
	drawSprite(_titleSprites, (const uint8_t *)&_titleSprites[1], option);
	refreshScreen(false);
}

int Menu::handleTitleScreen() {
	const int firstOption = kTitleScreen_AssignPlayer;
	const int lastOption = _res->_isPsx ? kTitleScreen_Play : kTitleScreen_Quit;
	int currentOption = kTitleScreen_Play;
	while (!g_system->inp.quit) {
		g_system->processEvents();
		if (g_system->inp.keyReleased(SYS_INP_UP)) {
			if (currentOption > firstOption) {
				playSound(kSound_0x70);
				--currentOption;
			}
		}
		if (g_system->inp.keyReleased(SYS_INP_DOWN)) {
			if (currentOption < lastOption) {
				playSound(kSound_0x70);
				++currentOption;
			}
		}
		if (g_system->inp.keyReleased(SYS_INP_SHOOT) || g_system->inp.keyReleased(SYS_INP_JUMP)) {
			playSound(kSound_0x78);
			break;
		}
		drawTitleScreen(currentOption);
		g_system->sleep(kDelayMs);
	}
	return currentOption;
}

void Menu::drawDigit(int x, int y, int num) {
	assert(x >= 0 && x + 16 < Video::W && y >= 0 && y + 10 < Video::H);
	assert(num < 16);
	uint8_t *dst = _video->_frontLayer + y * Video::W + x;
	const uint8_t *src = _digitsData + num * 16;
	for (int j = 0; j < 10; ++j) {
		for (int i = 0; i < 16; ++i) {
			if (src[i] != 0) {
				dst[i] = src[i];
			}
		}
		dst += Video::W;
		src += Video::W;
	}
}

void Menu::drawBitmap(const DatBitmapsGroup *bitmapsGroup, const uint8_t *bitmapData, int x, int y, int w, int h, uint8_t baseColor) {
	if (_res->_isPsx) {
		const int size = bitmapsGroup->palette - bitmapsGroup->offset;
		_video->decodeBackgroundPsx(bitmapData, size, w, h, x, y);
		return;
	}
	const int srcPitch = w;
	if (x < 0) {
		bitmapData -= x;
		w += x;
		x = 0;
	}
	if (x + w > Video::W) {
		w = Video::W - x;
	}
	if (y < 0) {
		bitmapData -= y * srcPitch;
		h += y;
		y = 0;
	}
	if (y + h > Video::H) {
		h = Video::H - y;
	}
	uint8_t *dst = _video->_frontLayer + y * Video::W + x;
	for (int j = 0; j < h; ++j) {
		for (int i = 0; i < w; ++i) {
			dst[i] = baseColor - bitmapsGroup->colors + bitmapData[i];
		}
		dst += Video::W;
		bitmapData += srcPitch;
	}
}

void Menu::setCurrentPlayer(int num) {
	debug(kDebug_MENU, "setCurrentPlayer %d", num);
	setLevelCheckpoint(num);
	_levelNum = _config->players[num].levelNum;
	if (_levelNum > kLvl_dark) {
		_levelNum = kLvl_dark;
	}
	if (_levelNum == kLvl_dark) {
		_levelNum = 7;
		_checkpointNum = 11;
	} else {
		_checkpointNum = _config->players[num].checkpointNum;
		if (_checkpointNum >= _res->_datHdr.levelCheckpointsCount[_levelNum]) {
			_checkpointNum = _res->_datHdr.levelCheckpointsCount[_levelNum] - 1;
		}
	}
// 422CE0
	const DatBitmapsGroup *bitmap = &_checkpointsBitmaps[_levelNum][_checkpointNum];
	memcpy(_paletteBuffer + 205 * 3, _checkpointsBitmapsData[_levelNum] + bitmap->palette, 50 * 3);
	g_system->setPalette(_paletteBuffer, 256, 6);
}

void Menu::setLevelCheckpoint(int num) {
	for (int i = 0; i < kCheckpointLevelsCount; ++i) {
		uint8_t checkpoint = _config->players[num].progress[i] + 1;
		if (checkpoint >= _res->_datHdr.levelCheckpointsCount[i]) {
			checkpoint = _res->_datHdr.levelCheckpointsCount[i];
		}
		_lastLevelCheckpointNum[i] = checkpoint;
	}
}

void Menu::drawPlayerProgress(int state, int cursor) {
	if (_res->_isPsx) {
		_video->decodeBackgroundPsx(_playerBitmapData, _playerBitmapSize, Video::W, Video::H);
	} else {
		decodeLZW(_playerBitmapData, _video->_frontLayer);
	}
	int player = 0;
	for (int y = 96; y < 164; y += 17) {
		if (isEmptySetupCfg(_config, player)) {
			drawSprite(_playerSprites, (const uint8_t *)&_playerSprites[1], 3, 82, y - 3); // empty
		} else {
			int levelNum = _config->players[player].levelNum;
			int checkpointNum;
			if (levelNum == kLvl_dark) {
				levelNum = 7;
				checkpointNum = 11;
			} else {
				checkpointNum = _config->players[player].checkpointNum;
			}
			drawDigit(145, y, levelNum + 1);
			drawDigit(234, y, checkpointNum + 1);
		}
		++player;
	}
// 422DD7
	player = (cursor == 0 || cursor == 5) ? _config->currentPlayer : (cursor - 1);
	uint8_t *p = _video->_frontLayer;
	const int offset = (player * 17) + 92;
	for (int i = 0; i < 16; ++i) { // player
		memcpy(p + i * Video::W +  6935, p + i * Video::W + (offset + 1) * 256 +   8, 72);
	}
// 422E47
	for (int i = 0; i < 16; ++i) { // level
		memcpy(p + i * Video::W + 11287, p + i * Video::W + (offset + 1) * 256 +  83, 76);
	}
// 422E87
	for (int i = 0; i < 16; ++i) { // checkpoint
		memcpy(p + i * Video::W + 15639, p + i * Video::W + (offset + 1) * 256 + 172, 76);
	}
// 422EC3
	if (!isEmptySetupCfg(_config, player)) {
		DatBitmapsGroup *bitmap = &_checkpointsBitmaps[_levelNum][_checkpointNum];
		const uint8_t *src = _checkpointsBitmapsData[_levelNum] + bitmap->offset;
		drawBitmap(bitmap, src, 132, 0, bitmap->w, bitmap->h, 205);
	}
// 422EFE
	if (cursor > 0) {
		if (cursor <= 4) { // highlight one player
			drawSprite(_playerSprites, (const uint8_t *)&_playerSprites[1], 8, 2, cursor * 17 + 74);
			drawSprite(_playerSprites, (const uint8_t *)&_playerSprites[1], 6);
		} else if (cursor == 5) { // cancel
			drawSprite(_playerSprites, (const uint8_t *)&_playerSprites[1], 7);
		}
	}
// 422F49
	drawSprite(_playerSprites, (const uint8_t *)&_playerSprites[1], state); // Yes/No
	if (state > 2) {
		drawSprite(_playerSprites, (const uint8_t *)&_playerSprites[1], 2); // MessageBox
	}
	refreshScreen(false);
}

void Menu::handleAssignPlayer() {
	memcpy(_paletteBuffer, _playerBitmapData + _playerBitmapSize, 256 * 3);
	int state = 1;
	int cursor = 0;
	setCurrentPlayer(_config->currentPlayer);
	drawPlayerProgress(state, cursor);
	while (!g_system->inp.quit) {
		g_system->processEvents();
		if (g_system->inp.keyReleased(SYS_INP_SHOOT) || g_system->inp.keyReleased(SYS_INP_JUMP)) {
			if (state != 0 && cursor == 5) {
				playSound(kSound_0x80);
			} else {
				playSound(kSound_0x78);
			}
// 42301D
			if (state == 0) {
				// return to title screen
				return;
			}
			if (cursor == 0) {
				cursor = _config->currentPlayer + 1;
			} else {
				if (cursor == 5) {
					cursor = 0;
				} else if (state == 1) { // select
					--cursor;
					_config->currentPlayer = cursor;
					// setVolume();
					cursor = 0;
				} else if (state == 2) { // clear
					state = 5; // 'No'
				} else {
					if (state == 4) { // 'Yes', clear confirmation
						--cursor;
						setDefaultsSetupCfg(_config, cursor);
						// setVolume();
					}
					setCurrentPlayer(_config->currentPlayer);
					state = 2;
					cursor = 0;
				}
			}
// 423125
		}
		if (cursor != 0 && state < 3) {
			if (g_system->inp.keyReleased(SYS_INP_UP)) {
				if (cursor > 1) {
					playSound(kSound_0x70);
					--cursor;
					setCurrentPlayer(cursor - 1);
				}
			}
			if (g_system->inp.keyReleased(SYS_INP_DOWN)) {
				if (cursor < 5) {
					playSound(kSound_0x70);
					++cursor;
					setCurrentPlayer((cursor == 5) ? _config->currentPlayer : (cursor - 1));
				}
			}
		} else {
			if (g_system->inp.keyReleased(SYS_INP_LEFT)) {
				if (state == 1 || state == 2 || state == 5) {
					playSound(kSound_0x70);
					--state;
				}
			}
			if (g_system->inp.keyReleased(SYS_INP_RIGHT)) {
				if (state == 0 || state == 1 || state == 4) {
					playSound(kSound_0x70);
					++state;
				}
			}
		}
// 4231FF
		drawPlayerProgress(state, cursor);
		g_system->sleep(kDelayMs);
	}
}

void Menu::updateBitmapsCircularList(const DatBitmapsGroup *bitmapsGroup, const uint8_t *bitmapData, int num, int count) {
	_bitmapCircularListIndex[1] = num;
	if (count > 1) {
		_bitmapCircularListIndex[2] = (num + 1) % count;
	} else {
		_bitmapCircularListIndex[2] = -1;
	}
	if (count > 2) {
		_bitmapCircularListIndex[0] = (num + count - 1) % count;
	} else {
		_bitmapCircularListIndex[0] = -1;
	}
	if (bitmapsGroup == _cutscenesBitmaps) {
		for (int i = 0; i < 3; ++i) {
			const int num = _bitmapCircularListIndex[i];
			if (num != -1 && num < _cutsceneIndexesCount) {
				_bitmapCircularListIndex[i] = _cutsceneIndexes[num];
			}
		}
	}
// 423F05
	for (int i = 0; i < 3; ++i) {
		if (_bitmapCircularListIndex[i] != -1) {
			const DatBitmapsGroup *bitmap = &bitmapsGroup[_bitmapCircularListIndex[i]];
			memcpy(_paletteBuffer + (105 + 50 * i) * 3, bitmapData + bitmap->palette, 50 * 3);
		}
	}
	g_system->setPalette(_paletteBuffer, 256, 6);
}

void Menu::drawBitmapsCircularList(const DatBitmapsGroup *bitmapsGroup, const uint8_t *bitmapData, int num, int count, bool updatePalette) {
	memcpy(_paletteBuffer, _optionsBitmapData[_optionNum] + _optionsBitmapSize[_optionNum], 768);
	if (updatePalette) {
		g_system->setPalette(_paletteBuffer, 256, 6);
	}
	updateBitmapsCircularList(bitmapsGroup, bitmapData, num, count);
	static const int xPos[] = { -60, 68, 196 };
	for (int i = 0; i < 3; ++i) {
		if (_bitmapCircularListIndex[i] != -1) {
			const DatBitmapsGroup *bitmap = &bitmapsGroup[_bitmapCircularListIndex[i]];
			drawBitmap(bitmap, bitmapData + bitmap->offset, xPos[i], 14, bitmap->w, bitmap->h, 105 + 50 * i);
		}
	}
}

void Menu::drawCheckpointScreen() {
	decodeLZW(_optionsBitmapData[_optionNum], _video->_frontLayer);
	drawBitmapsCircularList(_checkpointsBitmaps[_levelNum], _checkpointsBitmapsData[_levelNum], _checkpointNum, _lastLevelCheckpointNum[_levelNum], false);
	drawSpriteAnim(_iconsSprites, _iconsSpritesData, 5);
	drawSprite(&_iconsSprites[0], _iconsSpritesData, (_checkpointNum + 1) / 10, 119, 108);
	drawSprite(&_iconsSprites[0], _iconsSpritesData, (_checkpointNum + 1) % 10, 127, 108);
	const int num = _loadCheckpointButtonState;
	if (num > 1 && num < 7) {
		drawSprite(&_iconsSprites[9], _iconsSpritesData, num - 2);
	} else {
		drawSpriteAnim(_iconsSprites, _iconsSpritesData, (num != 0) ? 8 : 7);
	}
	drawSpriteAnim(_iconsSprites, _iconsSpritesData, 6);
	refreshScreen(false);
}

void Menu::drawLevelScreen() {
	decodeLZW(_optionsBitmapData[_optionNum], _video->_frontLayer);
	drawSprite(&_iconsSprites[1], _iconsSpritesData, _levelNum);
	DatBitmapsGroup *bitmap = &_levelsBitmaps[_levelNum];
	drawBitmap(bitmap, _levelsBitmapsData + bitmap->offset, 23, 10, bitmap->w, bitmap->h, 192);
	drawSpriteAnim(_iconsSprites, _iconsSpritesData, 4);
	drawSpriteAnim(_iconsSprites, _iconsSpritesData, (_loadLevelButtonState != 0) ? 3 : 2);
	refreshScreen(false);
}

void Menu::drawCutsceneScreen() {
	decodeLZW(_optionsBitmapData[_optionNum], _video->_frontLayer);
	drawBitmapsCircularList(_cutscenesBitmaps, _cutscenesBitmapsData, _cutsceneNum, _cutsceneIndexesCount, false);
	drawSpriteAnim(_iconsSprites, _iconsSpritesData, 10);
	drawSprite(&_iconsSprites[0], _iconsSpritesData, (_cutsceneNum + 1) / 10, 119, 108);
	drawSprite(&_iconsSprites[0], _iconsSpritesData, (_cutsceneNum + 1) % 10, 127, 108);
	const int num = _loadCutsceneButtonState;
	if (num > 1 && num < 7) {
		drawSprite(&_iconsSprites[14], _iconsSpritesData, num - 2);
	} else {
		drawSpriteAnim(_iconsSprites, _iconsSpritesData, (num != 0) ? 13 : 12);
	}
	drawSpriteAnim(_iconsSprites, _iconsSpritesData, 11);
	refreshScreen(false);
}

void Menu::drawSettingsScreen() {
	decodeLZW(_optionsBitmapData[_optionNum], _video->_frontLayer);
	drawSpriteAnim(_iconsSprites, _iconsSpritesData, 0x2A);
	drawSpriteAnim(_iconsSprites, _iconsSpritesData, (_settingNum == kSettingNum_Controls) ? 0x27 : 0x24);
	drawSpriteAnim(_iconsSprites, _iconsSpritesData, (_settingNum == kSettingNum_Difficulty) ? 0x26 : 0x23);
	drawSpriteAnim(_iconsSprites, _iconsSpritesData, (_settingNum == kSettingNum_Sound) ? 0x28 : 0x25);
	drawSprite(&_iconsSprites[0x29], _iconsSpritesData, (_settingNum == kSettingNum_Confirm) ? 1 : 0);
	refreshScreen(true);
}

void Menu::handleSettingsScreen(int num) {
	const uint8_t *data = &_optionData[num * 8];
	num = data[5];
	if (num == kCursor_Select) {
		if (_settingNum == kSettingNum_Controls) {
			playSound(kSound_0x78);
			_condMask = 0x10;
		} else if (_settingNum == kSettingNum_Difficulty) {
			playSound(kSound_0x78);
			_condMask = 0x20;
		} else if (_settingNum == kSettingNum_Sound) {
			playSound(kSound_0x78);
			_condMask = 0x40;
		} else if (_settingNum == kSettingNum_Confirm) {
			playSound(kSound_0x78);
			_condMask = 0x80;
		}
		return;
	} else if (num == kCursor_Left) {
		if (_settingNum != kSettingNum_Confirm && _settingNum > 0 && 0) { // 'controls' not implemented
			playSound(kSound_0x70);
			--_settingNum;
// 427CD4
			_iconsSprites[0x27].num = 0;
			_iconsSprites[0x26].num = 0;
			_iconsSprites[0x28].num = 0;
		}
	} else if (num == kCursor_Right) {
		if (_settingNum != kSettingNum_Confirm && _settingNum < 2 && 0) { // 'volume' not implemented
			playSound(kSound_0x70);
			++_settingNum;
// 427CD4
			_iconsSprites[0x27].num = 0;
			_iconsSprites[0x26].num = 0;
			_iconsSprites[0x28].num = 0;
		}
	} else if (num == kCursor_Up) {
		if (_settingNum == kSettingNum_Confirm) {
			playSound(kSound_0x70);
			_settingNum = kSettingNum_Difficulty;
// 427C46
			_iconsSprites[0x26].num = 0;
		}
	} else if (num == kCursor_Down) {
		if (_settingNum != kSettingNum_Confirm) {
			playSound(kSound_0x70);
		}
		_settingNum = kSettingNum_Confirm;
	}
// 427D10
	drawSettingsScreen();
	_condMask = 8;
	g_system->sleep(kDelayMs);
}

void Menu::drawDifficultyScreen() {
	decodeLZW(_optionsBitmapData[_optionNum], _video->_frontLayer);
	for (int i = 0; i < 3; ++i) {
		if (i != _difficultyNum) {
			drawSprite(&_iconsSprites[0xF], _iconsSpritesData, i * 2);
		}
	}
	drawSprite(&_iconsSprites[0xF], _iconsSpritesData, _difficultyNum * 2 + 1);
	refreshScreen(true);
}

void Menu::handleDifficultyScreen(int num) {
	const uint8_t *data = &_optionData[num * 8];
	num = data[5];
	if (num == kCursor_Select) {
		playSound(kSound_0x78);
		_config->players[_config->currentPlayer].difficulty = _g->_difficulty = _difficultyNum;
		_condMask = 0x80;
	} else if (num == kCursor_Left) {
		if (_difficultyNum > 0) {
			playSound(kSound_0x70);
			--_difficultyNum;
		}
	} else if (num == kCursor_Right) {
		if (_difficultyNum < 2) {
			playSound(kSound_0x70);
			++_difficultyNum;
		}
	}
	drawDifficultyScreen();
	g_system->sleep(kDelayMs);
}

void Menu::drawSoundScreen() {
	decodeLZW(_optionsBitmapData[_optionNum], _video->_frontLayer);
	drawSprite(&_iconsSprites[0x12], _iconsSpritesData, (_soundNum == kSoundNum_Stereo) ? 1 : 0);
	drawSprite(&_iconsSprites[0x12], _iconsSpritesData, (_soundNum == kSoundNum_Volume) ? 3 : 2);
	drawSprite(&_iconsSprites[0x12], _iconsSpritesData, (_soundNum == kSoundNum_Confirm) ? 5 : 4);
	drawSprite(&_iconsSprites[0x12], _iconsSpritesData, (_soundNum == kSoundNum_Test) ? 7 : 6);
	drawSprite(&_iconsSprites[0x12], _iconsSpritesData, (_soundNum == kSoundNum_Cancel) ? 9 : 8);
	drawSprite(&_iconsSprites[0x12], _iconsSpritesData, (_soundNum == kSoundNum_Reset) ? 11 : 10);
	// volume bar
	const int w = (_g->_snd_masterVolume * 96) / 128;
	for (int y = 0; y < 15; ++y) {
		memset(_video->_frontLayer + 18807 + 256 * y, 0xE0, w);
	}
	drawSprite(&_iconsSprites[0x12], _iconsSpritesData, 21);
	if (_soundNum == kSoundNum_Test) {
//		drawSprite(&_iconsSprites[0x12], _iconsSpritesData, _soundTestNum);
	}
	if (_g->_snd_masterVolume != 0) {
		if (_config->players[_config->currentPlayer].stereo) {
			drawSprite(&_iconsSprites[0x12], _iconsSpritesData, 13);
			drawSprite(&_iconsSprites[0x12], _iconsSpritesData, 16);
		} else {
			drawSprite(&_iconsSprites[0x12], _iconsSpritesData, 14);
			drawSprite(&_iconsSprites[0x12], _iconsSpritesData, 15);
		}
	}
// 4255C9
	if (0) { // (_volumeState != 0) && (_soundCounter & 1)
		if (0) { // _volumeState == 1
			drawSprite(&_iconsSprites[0x12], _iconsSpritesData, 18);
			drawSprite(&_iconsSprites[0x12], _iconsSpritesData, 19);
		} else {
			drawSprite(&_iconsSprites[0x12], _iconsSpritesData, 17);
			drawSprite(&_iconsSprites[0x12], _iconsSpritesData, 20);
		}
	} else {
		drawSprite(&_iconsSprites[0x12], _iconsSpritesData, 17);
		drawSprite(&_iconsSprites[0x12], _iconsSpritesData, 19);
	}
// 42563E
	refreshScreen(true);
}

void Menu::handleSoundScreen(int num) {
	// _volumeState = 0;
	// ++_soundCounter;
	const uint8_t *data = &_optionData[num * 8];
	num = data[5];
	if (num == kCursor_Select) {
		if (_soundNum == kSoundNum_Confirm) {
			playSound(kSound_0x78);
			_config->players[_config->currentPlayer].volume = _g->_snd_masterVolume;
			_condMask = 0x80;
		} else if (_soundNum == kSoundNum_Test) {
			playSound(kSound_0x60);
// 425B7D
			// ...
		} else if (_soundNum == kSoundNum_Cancel) {
			playSound(kSound_0x80);
// 425ACB
			_config->players[_config->currentPlayer].volume = _g->_snd_masterVolume = _soundVolume;
			_condMask = 0x80;
		} else if (_soundNum == kSoundNum_Reset) {
			playSound(kSound_0x88);
			_config->players[_config->currentPlayer].volume = _g->_snd_masterVolume = Game::kDefaultSoundVolume;
		}
	} else if (num == kCursor_Left) {
		if (_soundNum == kSoundNum_Volume) {
// 42590D
			if (_g->_snd_masterVolume > 0) {
				playSound(kSound_0x90);
				--_g->_snd_masterVolume;
				_config->players[_config->currentPlayer].volume = _g->_snd_masterVolume;
				// _volumeState = 1;
				_condMask = 8;
			}
		} else if (_soundNum == kSoundNum_Test) {
			playSound(kSound_0x70);
			_soundNum = kSoundNum_Cancel;
		} else if (_soundNum == kSoundNum_Cancel) {
			playSound(kSound_0x70);
			_soundNum = kSoundNum_Confirm;
		}
	} else if (num == kCursor_Right) {
		if (_soundNum == kSoundNum_Volume) {
			if (_g->_snd_masterVolume < 128) {
				playSound(kSound_0x90);
				++_g->_snd_masterVolume;
				_config->players[_config->currentPlayer].volume = _g->_snd_masterVolume;
				// _volumeState = 2;
				_condMask = 8;
			}
		} else if (_soundNum == 2) {
			playSound(kSound_0x70);
			_soundNum = kSoundNum_Cancel;
		} else if (_soundNum == 4) {
			if (_g->_snd_masterVolume != 0) {
				playSound(kSound_0x70);
			}
			_soundNum = kSoundNum_Test;
		}
	} else if (num == kCursor_Up) {
		if (_soundNum != kSoundNum_Volume) {
			playSound(kSound_0x70);
		}
		if (_soundNum >= 2 && _soundNum <= 4) {
			_soundNum = kSoundNum_Volume;
		} else if (_soundNum == kSoundNum_Reset) {
			_soundNum = kSoundNum_Cancel;
		}
	} else if (num == kCursor_Down) {
		if (_soundNum != kSoundNum_Reset) {
			playSound(kSound_0x70);
		}
		if (_soundNum == kSoundNum_Stereo) {
			_soundNum = kSoundNum_Volume;
		} else if (_soundNum == kSoundNum_Volume) {
			_soundNum = kSoundNum_Cancel;
		} else if (_soundNum >= 2 && _soundNum <= 4) {
			_soundNum = 5;
		}
	} else {
		// _soundCounter = 0;
	}
// 425D13
	drawSoundScreen();
	g_system->sleep(kDelayMs);
}

void Menu::changeToOption(int num) {
	const uint8_t *data = &_optionData[num * 8];
	const int button = data[6];
	if (button != 0xFF) {
		assert(button < _optionsButtonSpritesCount);
		_currentOptionButtonSound = READ_LE_UINT32(_optionsButtonSpritesData + button * 20);
		_currentOptionButtonSprite = (DatSpritesGroup *)(_optionsButtonSpritesData + button * 20 + 4);
		_currentOptionButtonSprite->num = 0; // start from frame #0
	} else {
		_currentOptionButtonSound = 0;
		_currentOptionButtonSprite = 0;
	}
// 428053
	if (!_paf->_skipCutscenes) {
		_paf->play(data[5]);
	}
	if (_optionNum == kMenu_NewGame + 1) {
		_config->players[_config->currentPlayer].levelNum = 0;
		_config->players[_config->currentPlayer].checkpointNum = 0;
	} else if (_optionNum == kMenu_CurrentGame + 1) {
		// _config->players[_config->currentPlayer].levelNum = _g->_currentLevel;
		// _config->players[_config->currentPlayer].checkpointNum = _g->_currentLevelCheckpoint;
	} else if (_optionNum == kMenu_Load + 1) {
// 4281C3
		_loadLevelButtonState = 0;
		memcpy(_paletteBuffer, _optionsBitmapData[5] + _optionsBitmapSize[5], 768);
		memcpy(_paletteBuffer + 192 * 3, _levelsBitmapsData + _levelsBitmaps[_levelNum].palette, 64 * 3);
		g_system->setPalette(_paletteBuffer, 256, 6);
		drawLevelScreen();
	} else if (_optionNum == kMenu_Load + 2) {
// 42816B
		_loadCheckpointButtonState = 0;
		_checkpointNum = 0;
		setLevelCheckpoint(_config->currentPlayer);
		memcpy(_paletteBuffer, _optionsBitmapData[6] + _optionsBitmapSize[6], 768);
		g_system->setPalette(_paletteBuffer, 256, 6);
		drawCheckpointScreen();
	} else if (_optionNum == kMenu_Settings + 1) {
// 428118
		_settingNum = kSettingNum_Difficulty;
		memcpy(_paletteBuffer, _optionsBitmapData[_optionNum] + _optionsBitmapSize[_optionNum], 768);
		handleSettingsScreen(5);
	} else if (_optionNum == kMenu_Cutscenes + 1) {
// 4280EA
		_loadCutsceneButtonState = 0;
		_cutsceneNum = 0;
		drawCutsceneScreen();
	} else if (_optionsBitmapSize[_optionNum] != 0) {
		decodeLZW(_optionsBitmapData[_optionNum], _video->_frontLayer);
		memcpy(_paletteBuffer, _optionsBitmapData[_optionNum] + _optionsBitmapSize[_optionNum], 256 * 3);
		refreshScreen(true);
	}
}

void Menu::handleLoadLevel(int num) {
	const uint8_t *data = &_optionData[num * 8];
	num = data[5];
	if (num == 16) {
		if (_loadLevelButtonState != 0) {
			playSound(kSound_0x70);
		}
		_loadLevelButtonState = 0;
	} else if (num == 17) {
		if (_loadLevelButtonState == 0) {
			playSound(kSound_0x70);
		}
		_loadLevelButtonState = 1;
	} else if (num == 18) {
		if (_loadLevelButtonState != 0) {
			playSound(kSound_0x80);
			_condMask = 0x80;
		} else {
			playSound(kSound_0x78);
			_condMask = 0x10;
		}
	} else if (num == 19) {
		if (_lastLevelNum > 1) {
			playSound(kSound_0x70);
			++_levelNum;
			_checkpointNum = 0;
			if (_levelNum >= _lastLevelNum) {
				_levelNum = 0;
			}
			memcpy(_paletteBuffer + 192 * 3, _levelsBitmapsData + _levelsBitmaps[_levelNum].palette, 64 * 3);
			g_system->setPalette(_paletteBuffer, 256, 6);
			_condMask = 0x20;
		}
	} else if (num == 20) {
		if (_lastLevelNum > 1) {
			playSound(kSound_0x70);
			--_levelNum;
			_checkpointNum = 0;
			if (_levelNum < 0) {
				_levelNum = _lastLevelNum - 1;
			}
			memcpy(_paletteBuffer + 192 * 3, _levelsBitmapsData + _levelsBitmaps[_levelNum].palette, 64 * 3);
			g_system->setPalette(_paletteBuffer, 256, 6);
			_condMask = 0x40;
		}
	}
	drawLevelScreen();
	g_system->sleep(kDelayMs);
}

void Menu::handleLoadCheckpoint(int num) {
	const uint8_t *data = &_optionData[num * 8];
	num = data[5];
	if (num == 11) {
		if (_loadCheckpointButtonState != 0) {
			playSound(kSound_0x70);
			_loadCheckpointButtonState = 0;
		}
	} else if (num == 12) {
		if (_loadCheckpointButtonState == 0) {
			playSound(kSound_0x70);
			_loadCheckpointButtonState = 1;
		}
	} else if (num == 13) {
		if (_loadCheckpointButtonState != 0) {
			playSound(kSound_0x80);
			_condMask = 0x80;
		} else {
			playSound(kSound_0x78);
			_loadCheckpointButtonState = 2;
			_condMask = 0x08;
			if (_levelNum == 7 && _checkpointNum == 11) {
				_config->players[_config->currentPlayer].levelNum = kLvl_dark;
				_config->players[_config->currentPlayer].checkpointNum = 0;
			} else {
				_config->players[_config->currentPlayer].levelNum = _levelNum;
				_config->players[_config->currentPlayer].checkpointNum = _checkpointNum;
			}
			debug(kDebug_MENU, "Restart game at level %d checkpoint %d", _levelNum, _checkpointNum);
			return;
		}
	} else if (num == 14) {
		if (_lastLevelCheckpointNum[_levelNum] > 2 || _loadCheckpointButtonState == 0) {
			playSound(kSound_0x70);
			++_checkpointNum;
			if (_checkpointNum >= _lastLevelCheckpointNum[_levelNum]) {
				_checkpointNum = 0;
			}
		}
	} else if (num == 15) {
		if (_lastLevelCheckpointNum[_levelNum] > 2 || _loadCheckpointButtonState == 1) {
			playSound(kSound_0x70);
			--_checkpointNum;
			if (_checkpointNum < 0) {
				_checkpointNum = _lastLevelCheckpointNum[_levelNum] - 1;
			}
		}
	}
	drawCheckpointScreen();
	g_system->sleep(kDelayMs);
}

void Menu::handleLoadCutscene(int num) {
	const uint8_t *data = &_optionData[num * 8];
	num = data[5];
	if (num == 6) {
		if (_loadCutsceneButtonState != 0) {
			playSound(kSound_0x70);
			_loadCutsceneButtonState = 0;
		}
	} else if (num == 7) {
		if (_loadCutsceneButtonState == 0) {
			playSound(kSound_0x70);
			_loadCutsceneButtonState = 1;
		}
	} else if (num == 8) {
		if (_loadCutsceneButtonState != 0) {
			playSound(kSound_0x80);
			_condMask = 0x80;
		} else {
			playSound(kSound_0x78);
			_loadCutsceneButtonState = 2;
			if (!_paf->_skipCutscenes) {
				const int num = _cutscenesBitmaps[_cutsceneIndexes[_cutsceneNum]].data;
				_paf->play(num);
				if (num == kPafAnimation_end) {
					_paf->play(kPafAnimation_cinema);
				}
			}
			playSound(kSound_0x98);
			playSound(kSound_0xA0);
			_loadCutsceneButtonState = 0;
		}
	} else if (num == 9) {
		if (_cutsceneIndexesCount > 2 || _loadCutsceneButtonState == 0) {
			playSound(kSound_0x70);
			++_cutsceneNum;
			if (_cutsceneNum >= _cutsceneIndexesCount) {
				_cutsceneNum = 0;
			}
		}
	} else if (num == 10) {
		if (_cutsceneIndexesCount > 2 || _loadCutsceneButtonState == 1) {
			playSound(kSound_0x70);
			--_cutsceneNum;
			if (_cutsceneNum < 0) {
				_cutsceneNum = _cutsceneIndexesCount - 1;
			}
		}
	}
	drawCutsceneScreen();
	g_system->sleep(kDelayMs);
}

static bool matchInput(uint8_t type, uint8_t mask, const PlayerInput &inp, uint8_t optionMask) {
	if (type != 0) {
		if ((mask & 1) != 0 && inp.keyReleased(SYS_INP_RUN)) {
			return true;
		}
		if ((mask & 2) != 0 && inp.keyReleased(SYS_INP_JUMP)) {
			return true;
		}
		if ((mask & 4) != 0 && inp.keyReleased(SYS_INP_SHOOT)) {
			return true;
		}
		if ((mask & optionMask) != 0) {
			return true;
		}
	} else {
		if ((mask & 1) != 0 && inp.keyReleased(SYS_INP_UP)) {
			return true;
		}
		if ((mask & 2) != 0 && inp.keyReleased(SYS_INP_RIGHT)) {
			return true;
		}
		if ((mask & 4) != 0 && inp.keyReleased(SYS_INP_DOWN)) {
			return true;
		}
		if ((mask & 8) != 0 && inp.keyReleased(SYS_INP_LEFT)) {
			return true;
		}
	}
	return false;
}

void Menu::handleOptions() {
	_lastLevelNum = _config->players[_config->currentPlayer].lastLevelNum + 1;
	if (_lastLevelNum > _res->_datHdr.levelsCount) {
		_lastLevelNum = _res->_datHdr.levelsCount;
	}
	_levelNum = _config->players[_config->currentPlayer].levelNum;
	if (_levelNum > kLvl_dark) {
		_levelNum = kLvl_dark;
	}
	if (_levelNum == kLvl_dark) {
		_levelNum = 7;
		_checkpointNum = 11;
	}
	_cutsceneIndexesCount = 0;
	const uint32_t playedCutscenesMask = _config->players[_config->currentPlayer].cutscenesMask;
	for (int i = 0; i < _res->_datHdr.cutscenesCount; ++i) {
		if ((playedCutscenesMask & (1 << i)) != 0) {
			_cutsceneIndexes[_cutsceneIndexesCount] = i;
			++_cutsceneIndexesCount;
		}
	}
	if (_cutsceneIndexesCount > _res->_datHdr.cutscenesCount) {
		_cutsceneIndexesCount = _res->_datHdr.cutscenesCount;
	}
// 428529
	_optionNum = kMenu_Settings;
	changeToOption(0);
	_condMask = 0;
	while (!g_system->inp.quit) {
// 428752
		g_system->processEvents();
		if (g_system->inp.keyPressed(SYS_INP_ESC)) {
			_optionNum = -1;
			break;
		}
		// get transition from inputs and menu return code (_condMask)
		int num = -1;
		for (int i = 0; i < _res->_datHdr.menusCount; ++i) {
			const uint8_t *data = &_optionData[i * 8];
			if (data[0] == _optionNum && matchInput(data[1] & 1, data[2], g_system->inp, _condMask)) {
				num = i;
				break;
			}
		}
		_condMask = 0;
		if (num == -1) {
			g_system->sleep(kDelayMs);
			continue;
		}
// 4287AD
		const uint8_t *data = &_optionData[num * 8];
		const int prevOptionNum = _optionNum;
		_optionNum = data[3];
		debug(kDebug_MENU, "handleOptions option %d code %d", _optionNum, data[4]);
		switch (data[4]) {
		case 0:
			if (prevOptionNum != _optionNum) {
				_iconsSprites[0x2A].num = 0;
				_iconsSprites[0x27].num = 0;
				_iconsSprites[0x26].num = 0;
				_iconsSprites[0x28].num = 0;
				memcpy(_paletteBuffer, _optionsBitmapData[_optionNum] + _optionsBitmapSize[_optionNum], 768);
			}
			handleSettingsScreen(num);
			break;
		// case 1: // controls
		case 4:
			if (prevOptionNum != _optionNum) {
				memcpy(_paletteBuffer, _optionsBitmapData[_optionNum] + _optionsBitmapSize[_optionNum], 768);
				_difficultyNum = _config->players[_config->currentPlayer].difficulty;
			}
			handleDifficultyScreen(num);
			break;
		case 5:
			if (prevOptionNum != _optionNum) {
				memcpy(_paletteBuffer, _optionsBitmapData[_optionNum] + _optionsBitmapSize[_optionNum], 768);
				_soundVolume = _g->_snd_masterVolume;
				_soundNum = kSoundNum_Confirm;
				// _soundCounter = 0;
			}
			handleSoundScreen(num);
			break;
		case 6:
			playSound(kSound_0x70);
			changeToOption(num);
			break;
		case 7:
			playSound(kSound_0x78);
			changeToOption(num);
			break;
		case 8:
			changeToOption(num);
			break;
		case 9:
			handleLoadCutscene(num);
			break;
		case 10:
			handleLoadLevel(num);
			break;
		case 11:
			break;
		case 12:
			setLevelCheckpoint(_config->currentPlayer);
			handleLoadCheckpoint(num);
			break;
		default:
			warning("Unhandled option %d %d", _optionNum, data[4]);
			break;
		}
// 428D41
		if (_optionNum == kMenu_Quit + 1 || _optionNum == kMenu_NewGame + 1 || _optionNum == kMenu_CurrentGame + 1 || _optionNum == kMenu_ResumeGame) {
// 428E74
			// 'setup.cfg' is saved when exiting the main loop
			break;
		}
	}
}
