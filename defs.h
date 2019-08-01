/*
 * Heart Of Darkness engine rewrite
 * Copyright (C) 2009-2011 Gregory Montoir
 */

#ifndef DEFS_H__
#define DEFS_H__

enum {
	kPosTopScreen    = 0,
	kPosRightScreen  = 1,
	kPosBottomScreen = 2,
	kPosLeftScreen   = 3
};

enum {
	kNone = 0xFFFFFFFF, // (uint32_t)-1
	kLvlAnimHdrOffset = 0x2C,
	kMstHeightMapDataSize = 948
};

enum {
	kActionKeyMaskRun   = 0x1,
	kActionKeyMaskJump  = 0x2,
	kActionKeyMaskShoot = 0x4
};

enum {
	kDirectionKeyMaskUp    = 0x1, // climb
	kDirectionKeyMaskRight = 0x2,
	kDirectionKeyMaskDown  = 0x4, // crouch
	kDirectionKeyMaskLeft  = 0x8
};

enum {
	kLvl_rock, // 0
	kLvl_fort,
	kLvl_pwr1,
	kLvl_isld,
	kLvl_lava, // 4
	kLvl_pwr2,
	kLvl_lar1,
	kLvl_lar2,
	kLvl_dark, // 8
	kLvl_test
};

struct Point8_t {
	int8_t x;
	int8_t y;
} PACKED;

struct Point16_t {
	int16_t x;
	int16_t y;
};

struct AnimBackgroundData {
	const uint8_t *currentSpriteData; // 0
	uint8_t *firstSpriteData; // 4
	uint8_t *otherSpriteData; // 8
	uint16_t framesCount; // 12
	uint16_t currentFrame; // 14
};

struct LvlAnimSeqHeader {
	uint16_t firstFrame;
	uint16_t unk2;
	int8_t dx; // 4
	int8_t dy; // 5
	uint8_t count; // 6
	uint8_t unk7; // 7
	uint16_t sound;
	uint16_t flags0;
	uint16_t flags1;
	uint16_t unkE;
	uint32_t offset; // 0x10, LvlAnimSeqFrameHeader
} PACKED; // sizeof == 20

struct LvlAnimSeqFrameHeader {
	uint16_t move; // 0
	uint16_t anim; // 2
	uint8_t frame; // 4
	uint8_t unk5; // 5
	int8_t unk6;
	int8_t unk7;
} PACKED; // sizeof == 8

struct LvlAnimHeader {
	uint16_t unk0;
	uint8_t seqCount;
	uint8_t unk2;
	uint32_t seqOffset;
} PACKED; // sizeof == 8

struct LvlSprMoveData {
	uint8_t op1;
	uint8_t op2;
	uint16_t op3;
	uint16_t op4;
	uint16_t unk0x6;
} PACKED; // sizeof == 8

struct LvlSprHotspotData {
	Point8_t pts[8];
} PACKED; // sizeof == 16

struct LvlObjectData {
	uint8_t unk0;
	uint8_t spriteNum;
	uint16_t framesCount;
	uint16_t hotspotsCount;
	uint16_t movesCount;
	uint16_t animsCount;
	uint8_t refCount; // 0xA
	uint8_t frame; // 0xB
	uint16_t anim; // 0xC
	uint8_t unkE;
	uint8_t unkF;
	uint8_t *animsInfoData; // 0x10, LevelSprAnimInfo
	uint8_t *movesData; // 0x14, LvlSprMoveData
	uint8_t *framesData; // 0x18
	uint32_t framesDataOffset; // 0x1C, not needed
	uint8_t *animsData; // 0x20
	uint8_t *coordsData; // 0x24
	uint8_t *hotspotsData; // 0x28, LvlSprHotspotData
};

struct Game;
struct SssObject;

struct LvlObject {
	int32_t xPos; // 0
	int32_t yPos; // 4
	uint8_t screenNum; // 8
	uint8_t screenState;
	uint8_t flags;
	uint8_t frame;
	uint16_t anim;
	uint8_t type;
	uint8_t spriteNum;
	uint16_t flags0; // hFlags
	uint16_t flags1; // vFlags
	uint16_t flags2; // spriteType
	uint8_t objectUpdateType;
	uint8_t hitCount;
	LvlObject *childPtr; // _andyObject : plasma cannon object or specialAnimation
	uint16_t width;
	uint16_t height;
	uint8_t actionKeyMask;
	uint8_t directionKeyMask;
	uint16_t currentSprite;
	uint16_t currentSound;
	uint8_t unk26;
	uint8_t unk27;
	const uint8_t *bitmapBits;
	int (Game::*callbackFuncPtr)(LvlObject *ptr);
	void *dataPtr;
	SssObject *sssObj; // 0x34
	LvlObjectData *levelData0x2988;
	Point16_t posTable[8];
	LvlObject *nextPtr;
};

struct SssFilter;
struct SssPcm;

struct SssObject {
	SssPcm *pcm; // 0x0
	uint16_t num; // 0x4
	uint16_t unk6; // 0x6
	int8_t unk8; // 0x8
	int8_t priority; // 0x9
	uint8_t flags; // 0xA
	bool stereo; // 0xB
	uint32_t flags0; // 0xC
	uint32_t flags1; // 0x10
	int32_t panning; // 0x14 panning default:64
	int32_t volume; // 0x18 volume (db) default:128
	int panL; // 0x1C leftGain
	int panR; // 0x20 rightGain
	int panType; // 0x24 : 0: silent, 1:fullRight 2:fullLeft 3:both
	const int16_t *currentPcmPtr; // 0x28
	int32_t pcmFramesCount; // 0x2C
	SssObject *prevPtr; // 0x30
	SssObject *nextPtr; // 0x34
	const uint8_t *codeDataStage1; // 0x38
	const uint8_t *codeDataStage2; // 0x3C
	const uint8_t *codeDataStage3; // 0x40
	const uint8_t *codeDataStage4; // 0x44
	int32_t repeatCounter; // 0x48
	int32_t pauseCounter; // 0x4C
	int32_t delayCounter; // 0x50
	int32_t volumeModulateSteps; // 0x54
	int32_t panningModulateSteps; // 0x58
	int32_t volumeModulateCurrent; // 0x5C
	int32_t volumeModulateDelta; // 0x60
	int32_t panningModulateCurrent; // 0x64
	int32_t panningModulateDelta; // 0x68
	int32_t currentPcmFrame; // 0x6C
	int *panningPtr; // 0x70
	LvlObject *lvlObject; // 0x74
	int32_t nextSoundNum; // 0x78 indexes _sssUnk3
	int32_t nextSoundCounter; // 0x7C
	SssFilter *filter;
};

struct Sprite {
	int16_t xPos;
	int16_t yPos;
	const uint8_t *bitmapBits;
	Sprite *nextPtr;
	uint16_t num;
	uint16_t flags;
};

struct BoundingBox {
	int32_t x1; // 0
	int32_t y1; // 4
	int32_t x2; // 8
	int32_t y2; // C
};

struct AndyLvlObjectData {
	uint8_t unk0;
	uint8_t unk1;
	uint8_t unk2;
	uint8_t unk3;
	uint8_t unk4;
	uint8_t unk5;
	uint16_t unk6;
	BoundingBox boundingBox; // 0x8
	int32_t dxPos; // 0x18
	int32_t dyPos; // 0x1C
	LvlObject *shootLvlObject; // 0x20 'cannon plasma' or 'special powers'
};

struct ShootLvlObjectData {
	uint8_t unk0;
	uint8_t unk1;
	uint8_t counter; // 0x2
	uint8_t unk3;
	int32_t dxPos; // 0x4
	int32_t dyPos; // 0x8
	int32_t x2; // 0xC
	int32_t y2; // 0x10
	LvlObject *o; // 0x1C
	ShootLvlObjectData *nextPtr; // 0x20 next pointer to 'free' element
};

struct ScreenMask { // ShadowScreenMask
	uint32_t dataSize;
	uint8_t *projectionDataPtr;
	uint8_t *shadowPalettePtr;
	int16_t x;
	int16_t y;
	uint16_t w;
	uint16_t h;
}; // sizeof == 0x14

struct CrackSprite { // mstOp57
	uint8_t screenNum;
	uint8_t initData1;
	int8_t initData2; // xPos
	int8_t initData3; // yPos
	uint32_t initData4;
	uint8_t initData8; // rect1_x1
	uint8_t initData9; // rect1_y1
	uint8_t initDataA; // rect1_x2
	uint8_t initDataB; // rect1_y2
	uint8_t initDataC; // rect2_x1
	uint8_t initDataD; // rect2_y1
	uint8_t initDataE; // rect2_x2
	uint8_t initDataF; // rect2_y2
	uint32_t flags[7]; // 0x10
}; // sizeof == 0x2C

struct MonsterObject1;

struct MovingOpcodeState {
	int32_t xPos;
	int32_t yPos; // 4
	BoundingBox boundingBox; // 8
	int32_t unk18;
	int32_t width;
	int32_t height;
	int32_t unk24;
	ShootLvlObjectData *unk28;
	LvlObject *o; // 0x2C
	MonsterObject1 *m; // 0x30
	int32_t unk34;
	int32_t unk38;
	int32_t unk3C;
	uint8_t unk40;
	uint8_t unk41;
}; // sizeof == 0x44

struct AndyMoveData {
	int32_t xPos;
	int32_t yPos;
	uint16_t anim; // 8
	uint16_t unkA;
	uint16_t unkC;
	uint16_t unkE;
	uint8_t frame; // 0x10
	uint8_t unk11;
	uint16_t flags0;
	uint16_t flags1;
	uint16_t unk16;
	uint16_t unk18;
	uint16_t unk1A;
	const uint8_t *unk1C;
	const uint8_t *framesData;
	const uint8_t *unk24;
	const uint8_t *unk28;
}; // sizeof == 0x2C

struct MstBoundingBox {
	int x1; // 0
	int y1; // 4
	int x2; // 8
	int y2; // 0xC
	int monster1Index; // 0x10
}; // sizeof == 0x20

struct MstCollision {
	MonsterObject1 *monster1[32]; // 0x00
	uint32_t count; // 0x80
}; // sizeof == 132

struct Task;

struct MstUnk35;
struct MstUnk44;
struct MstUnk45;
struct MstUnk46;
struct MstUnk46Unk1;
struct MstUnk44Unk1;
struct MstUnk48Unk12Unk4;
struct MstUnk49;
struct MstUnk49Unk1;

struct MonsterObject1 {
	MstUnk46 *m46;
	MstUnk46Unk1 *m46Unk1;
	const uint8_t *unk8; // infos
	MstUnk44Unk1 *unkC; // m44Unk1
	LvlObject *o16; // 0x10
	LvlObject *o20; // 0x14
	MstUnk48Unk12Unk4 *unk18; // m48Unk4
	MovingOpcodeState *collidePtr; // 0x1C
	int monster1Index; // 0x20
	int executeCounter; // 0x24
	int32_t localVars[8]; // 0x28
	uint8_t flags48; // 0x48 0x4:indexUnk51!=kNone, 0x10:indexUnk47!=kNone
	uint8_t flags49; // 0x49 facingDirectionMask
	uint8_t flags4A; // 0x4A
	uint8_t flags4B; // 0x4B
	int xPos; // 0x4C
	int yPos; // 0x50
	int xMstPos; // 0x54 xLevelPos
	int yMstPos; // 0x58 yLevelPos
	int xDelta; // 0x5C
	int yDelta; // 0x60
	int unk64; // 0x64
	int unk68; // 0x68
	int unk6C; // 0x6C
	int unk70; // 0x70
	int unk74; // 0x74
	int unk78; // 0x78
	int unk7C; // 0x7C
	int unk80; // 0x80
	int unk84; // 0x84
	int unk88; // 0x88
	int unk8C; // 0x8C
	int unk90; // 0x90
	int x1; // 0x94
	int x2; // 0x98
	int y1; // 0x9C
	int y2; // 0xA0
	uint8_t flagsA4; // 0xA4
	uint8_t flagsA5; // 0xA5
	uint8_t flagsA6; // 0xA6
	uint8_t flagsA7; // 0xA7
	uint8_t flagsA8[4]; // 0xA8, 0xA9, 0xAA, 0xAB collideRectNum
	int32_t unkAC;
	int32_t unkB0;
	int32_t unkB4; // _xMstPos2
	int32_t unkB8; // _yMstPos2
	int32_t unkBC;
	int32_t unkC0;
	Task *task; // 0xC4
	uint8_t rnd_m49[4]; // 0xC8
	uint8_t rnd_m35[4]; // 0xCC
	MstUnk35 *m35; // 0xD0
	MstUnk49Unk1 *m49Unk1; // 0xD4, sizeof=16
	MstUnk49 *m49; // 0xD8
	int indexUnk49Unk1; // 0xDC // indexes _mstUnk49.data1
	uint8_t unkE4;
	uint8_t unkE6; // 0xE6 indexes _mstLut4
	uint8_t directionKeyMask;
	uint16_t unkE8; // 0xE8
	int collideDistance; // 0xEC
	int unkF0; // 0xF0
	int unkF4; // 0xF4
	uint8_t unkF8; // 0xF8
	int unkFC; // 0xFC
}; // sizeof == 256

struct MonsterObject2 {
	MstUnk45 *m45;
	LvlObject *o; // 4
	MonsterObject1 *monster1; // 8
	int monster2Index; // 10
	int xPos; // 14
	int yPos; // 18
	int xMstPos; // 1C
	int yMstPos; // 20
	uint8_t flags24; // 24
	int x1; // 28
	int x2; // 2C
	int y1; // 30
	int y2; // 34
	uint8_t hPosIndex; // 38
	uint8_t vPosIndex; // 39
	uint8_t hDir; // 3A
	uint8_t vDir; // 3B
	Task *task; // 0x3C
}; // sizeof == 64

struct Task {
	const uint8_t *codeData;
	Task *prevPtr, *nextPtr; // 4,8
	MonsterObject1 *monster1; // 0xC
	MonsterObject2 *monster2; // 0x10
	int32_t localVars[8]; // 0x14
	uint8_t flags; // 0x34
	uint8_t state; // 0x35
	int16_t arg1; // 0x36 delay/counter/type
	uint32_t arg2; // 0x38  num/index
	int (Game::*run)(Task *t); // 0x3C
	Task *child; // 0x40
}; // sizeof == 0x44

#endif // DEFS_H__
