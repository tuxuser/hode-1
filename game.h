/*
 * Heart Of Darkness engine rewrite
 * Copyright (C) 2009-2011 Gregory Montoir
 */

#ifndef GAME_H__
#define GAME_H__

#include "intern.h"
#include "defs.h"
#include "fileio.h"
#include "mixer.h"
#include "random.h"
#include "resource.h"

struct Game;
struct PafPlayer;
struct Video;
struct SystemStub;

struct CheckpointData {
	int16_t xPos;
	int16_t yPos;
	uint16_t flags2;
	uint16_t anim;
	uint8_t screenNum;
	uint8_t spriteNum;
};

struct Game {
	enum {
		kObjectDataTypeAndy,
		// LvlObject.type == 0
		kObjectDataTypeAnimBackgroundData,
		// LvlObject.type == 8
		kObjectDataTypeShoot,
		// LvlObject.type == 1
		kObjectDataTypeLvlBackgroundSound,
		kObjectDataTypeMonster1,
		kObjectDataTypeMonster2
	};
	enum {
		kMaxScreens = 40,
		kMaxTasks = 128,
		kMaxVars = 40,
		kMaxLocals = 8,
		kMaxMovingStates = 8,
		kMaxShootLvlObjectData = 32,
		kMaxSssObjects = 32,
		kMaxMonsterObjects1 = 32,
		kMaxMonsterObjects2 = 64,
		kMaxBackgroundAnims = 64,
		kMaxSprites = 128,
		kMaxSpriteTypes = 32,
		kFrameTimeStamp = 50 // 80
	};

	static const uint8_t _pointDstIndexTable[];
	static const uint8_t _pointRandomizeShiftTable[];
	static const uint8_t _pointSrcIndex2Table[];
	static const uint8_t _pointSrcIndex1Table[];
	static const uint8_t _actionDirectionKeyMaskTable[];
	static const CheckpointData *_levelCheckpointData[];
	static const uint8_t *_levelScreenStartData[];
	static const uint8_t _pwr1_screenTransformData[];
	static const uint8_t _pwr2_screenTransformData[];
	static const uint8_t _pwr1_screenTransformLut[];
	static const uint8_t _lava_screenTransformLut[];
	static const uint8_t _pwr2_screenTransformLut[];

	Mixer _mix;
	PafPlayer *_paf;
	Random _rnd;
	Resource *_res;
	Video *_video;
	SystemStub *_system;

	int _difficulty;
	LvlObject *_screenLvlObjectsList[kMaxScreens]; // LvlObject linked list for each screen
	LvlObject *_andyObject;
	LvlObject *_plasmaExplosionObject;
	LvlObject *_plasmaCannonObject;
	LvlObject *_specialAnimLvlObject;
	LvlObject *_currentSoundLvlObject;
	int _currentLevel;
	int _levelCheckpoint;
	int _currentLevelCheckpoint;
	bool _quit;
	Sprite _spritesTable[kMaxSprites];
	Sprite *_spritesListNextPtr; // pointer to the first 'free' element
	Sprite *_spriteListPtrTable[kMaxSpriteTypes];
	uint16_t _fadePaletteBuffer[256 * 3];
	uint8_t *_shadowScreenMaskBuffer;
	uint8_t *_transformShadowBuffer;
	uint8_t _transformShadowLayerDelta;
	uint8_t _directionKeyMask;
	uint8_t _actionKeyMask;
	uint8_t _currentRightScreen;
	uint8_t _currentLeftScreen;
	uint8_t _currentScreen;
	int8_t _levelRestartCounter;
	bool _fadePalette;
	bool _hideAndyObjectSprite;
	ShootLvlObjectData _shootLvlObjectDataTable[kMaxShootLvlObjectData];
	ShootLvlObjectData *_shootLvlObjectDataList; // pointer to the first 'free' element
	LvlObject *_lvlObjectsList0;
	LvlObject *_lvlObjectsList1;
	LvlObject *_lvlObjectsList2;
	LvlObject *_lvlObjectsList3;
	uint8_t _screenCounterTable[kMaxScreens];
	uint8_t _screenPosTable[5][24 * 32];
	uint8_t _screenTempMaskBuffer[24 * 32];
	uint8_t _screenMaskBuffer[96 * 24 * 32];
	int _mstAndyCurrentScreenNum;
	uint8_t _shakeScreenDuration;
	const uint8_t *_shakeScreenTable;
	uint8_t _plasmaCannonDirection;
	uint8_t _plasmaCannonPrevDirection;
	uint8_t _plasmaCannonPointsSetupCounter;
	uint8_t _plasmaCannonLastIndex1;
	bool _plasmaCannonExplodeFlag;
	uint8_t _plasmaCannonPointsMask;
	uint8_t _plasmaCannonFirstIndex;
	uint8_t _plasmaCannonLastIndex2;
	uint8_t _plasmaCannonFlags;
	uint8_t _actionDirectionKeyMaskCounter;
	bool _fallingAndyFlag;
	uint8_t _fallingAndyCounter;
	uint8_t _actionDirectionKeyMaskIndex;
	uint8_t _andyActionKeyMaskAnd, _andyActionKeyMaskOr;
	uint8_t _andyDirectionKeyMaskAnd, _andyDirectionKeyMaskOr;
	int32_t _plasmaCannonPosX[129];
	int32_t _plasmaCannonPosY[129];
	int32_t _plasmaCannonXPointsTable1[129];
	int32_t _plasmaCannonYPointsTable1[129];
	int32_t _plasmaCannonXPointsTable2[127];
	int32_t _plasmaCannonYPointsTable2[127];
	ScreenMask _shadowScreenMasksTable[8];

	uint16_t _mstCurrentAnim;
	uint16_t _specialAnimMask;
	int16_t _mstOriginPosX;
	int16_t _mstOriginPosY;
	bool _specialAnimFlag;
	MovingOpcodeState _mstMovingState[kMaxMovingStates];
	int _mstMovingStateCount;
	uint8_t _mstOp68_type, _mstOp68_arg9, _mstOp67_type;
	uint8_t _mstOp67_flags1;
	uint16_t _mstOp67_unk;
	int _mstOp67_x1, _mstOp67_x2, _mstOp67_y1, _mstOp67_y2;
	uint8_t _mstOp67_screenNum;
	uint16_t _mstOp68_flags1;
	int _mstOp68_x1, _mstOp68_x2, _mstOp68_y1, _mstOp68_y2;
	uint8_t _mstOp68_screenNum;
	uint32_t _mstLogicHelper1TestValue;
	uint32_t _mstLogicHelper1TestMask;
	int _runTaskOpcodesCount;
	int32_t _mstVars[kMaxVars];
	uint32_t _mstFlags;
	int _clipBoxOffsetX, _clipBoxOffsetY;
	Task *_currentTask;
	int _mstOp54Counter;
	int _mstOp56Counter;
	uint8_t _mstOp54Table[32];
	bool _mstLogicDisabled;
	LvlObject _declaredLvlObjectsList[160];
	LvlObject *_declaredLvlObjectsListHead; // pointer to the first 'free' element
	int _declaredLvlObjectsListCount;
	AndyLvlObjectData _andyObjectScreenData;
	AnimBackgroundData _animBackgroundDataTable[kMaxBackgroundAnims];
	int _animBackgroundDataCount;
	uint8_t _andyActionKeysFlags;
	int _executeMstLogicCounter;
	int _executeMstLogicPrevCounter;
	Task _tasksTable[kMaxTasks];
	Task *_tasksList;
	Task *_monsterObjects1TasksList;
	Task *_monsterObjects2TasksList;
	int _mstPrevPosX; // _mstAndyLevelPrevPosX
	int _mstPrevPosY; // _mstAndyLevelPrevPosY
	int _mstPosX; // _mstAndyLevelPosX
	int _mstPosY; // _mstAndyLevelPosY
	int _mstAndyScreenPosX;
	int _mstAndyScreenPosY;
	int _mstAndyRectNum;
	int _m43Num1;
	int _m43Num2;
	int _m43Num3;
	int _yMstPos;
	int _mstUnk12;
	int _m48Num;
	uint8_t _mstUnk8;
	uint32_t _mstAndyVarMask;
	int _mstChasingMonstersCount;
	int _mstPosXmin, _mstPosXmax;
	int _mstPosYmin, _mstPosYmax;
	int _mstTemp_x1, _mstTemp_x2, _mstTemp_y1, _mstTemp_y2;
	MonsterObject1 _monsterObjects1Table[kMaxMonsterObjects1];
	MonsterObject2 _monsterObjects2Table[kMaxMonsterObjects2];
	int _mstTickDelay;
	uint8_t _mstCurrentActionKeyMask;
	int _xMstPos1, _xMstPos2;
	int _mstCurrentPosX, _mstCurrentPosY;
	int _mstBoundingBoxesCount;
	MstBoundingBox _mstBoundingBoxesTable[64];
	Task *_mstCurrentTask;
	MstCollision _mstCollisionTable[2][32]; // 0:facingRight, 1:facingLeft

	Game(SystemStub *system, const char *dataPath);
	~Game();

	// benchmark.cpp
	uint32_t benchmarkLoop(const uint8_t *p, int count);
	uint32_t benchmarkCpu();

	// game.cpp

	void mainLoop(int level, int checkpoint, bool levelChanged);
	void mixAudio(int16_t *buf, int len);
	void resetShootLvlObjectDataTable();
	void clearShootLvlObjectData(LvlObject *ptr);
	void setShakeScreen(int type, int counter);
	void fadeScreenPalette();
	void shakeScreen();
	void transformShadowLayer(int delta);
	void decodeShadowScreenMask(LvlBackgroundData *lvl);
	void playSound(int num, LvlObject *ptr, int a, int b);
	void removeSound(LvlObject *ptr);
	void setupBackgroundBitmap();
	void addToSpriteList(LvlObject *ptr);
	int16_t calcScreenMaskDy(int16_t xPos, int16_t yPos, int num);
	void setupScreenPosTable(uint8_t num);
	void setupScreenMask(uint8_t num);
	void resetScreenMask();
	void setScreenMaskRectHelper(int x1, int y1, int x2, int y2, int screenNum);
	void setScreenMaskRect(int x1, int y1, int x2, int y2, int pos);
	void updateScreenMaskBuffer(int x, int y, int type);
	void setupLvlObjectBitmap(LvlObject *ptr);
	void randomizeInterpolatePoints(int32_t *pts, int count);
	int fixPlasmaCannonPointsScreenMask(int num);
	void setupPlasmaCannonPointsHelper();
	void destroyLvlObjectPlasmaExplosion(LvlObject *o);
	void shuffleArray(uint8_t *p, int count);
	void destroyLvlObject(LvlObject *o);
	void setupPlasmaCannonPoints(LvlObject *ptr);
	int testPlasmaCannonPointsDirection(int x1, int y1, int x2, int y2);
	void preloadLevelScreenData(int num, int prev);
	void loadLevelScreenSounds(int num);
	void setLvlObjectPosRelativeToObject(LvlObject *ptr1, int num1, LvlObject *ptr2, int num2);
	void setLvlObjectPosRelativeToPoint(LvlObject *ptr, int num, int x, int y);
	void clearLvlObjectsList1();
	void clearLvlObjectsList2();
	void clearLvlObjectsList3();
	LvlObject *addLvlObjectToList1(int type, int num);
	LvlObject *addLvlObjectToList3(int num);
	void removeLvlObject(LvlObject *ptr);
	void removeLvlObject2(LvlObject *o);
	void setAndySprite(int num);
	void setupAndyLvlObject();
	void updateScreenHelper(int num);
	void resetDisplay();
	void updateScreen(uint8_t num);
	void resetScreen();
	void restartLevel();
	void playAndyFallingCutscene(int type);
	int8_t updateLvlObjectScreen(LvlObject *ptr);
	void setAndyAnimationForArea(BoundingBox *box, int dx);
	void setAndyLvlObjectPlasmaCannonKeyMask();
	int setAndySpecialAnimation(uint8_t mask);
	int clipBoundingBox(BoundingBox *coords, BoundingBox *box);
	int updateBoundingBoxClippingOffset(BoundingBox *_ecx, BoundingBox *_ebp, const uint8_t *coords, int direction);
	int clipLvlObjectsBoundingBoxHelper(LvlObject *o1, BoundingBox *box1, LvlObject *o2, BoundingBox *box2);
	int clipLvlObjectsBoundingBox(LvlObject *o, LvlObject *ptr, int count);
	int clipLvlObjectsSmall(LvlObject *o, LvlObject *ptr, int count);
	int restoreAndyCollidesLava();
	int updateAndyLvlObject();
	void drawPlasmaCannon();
	void drawScreen();
	void updateLvlObjectList(LvlObject **list);
	void updateLvlObjectLists();
	LvlObject *updateAnimatedLvlObjectType0(LvlObject *ptr);
	LvlObject *updateAnimatedLvlObjectType1(LvlObject *ptr);
	LvlObject *updateAnimatedLvlObjectType2(LvlObject *ptr);
	LvlObject *updateAnimatedLvlObjectTypeDefault(LvlObject *ptr);
	LvlObject *updateAnimatedLvlObject(LvlObject *o);
	void updateAnimatedLvlObjectsLeftRightCurrentScreens();
	void updatePlasmaCannonExplosionLvlObject(LvlObject *ptr);
	void resetPlasmaCannonState();
	void updateAndyMonsterObjects();
	void updateInput();
	void levelMainLoop();
	void callLevel_postScreenUpdate(int num);
	void callLevel_preScreenUpdate(int num);
	void callLevel_initialize();
	void callLevel_tick();
	void callLevel_terminate();
	int displayHintScreen(int num, int pause);
	void prependLvlObjectToList(LvlObject **list, LvlObject *ptr);
	void removeLvlObjectFromList(LvlObject **list, LvlObject *ptr);
	void *getLvlObjectDataPtr(LvlObject *o, int type) const;
	void lvlObjectType0Init(LvlObject *ptr);
	void lvlObjectType1Init(LvlObject *ptr);
	void lvlObjectTypeInit(LvlObject *ptr);
	void lvlObjectType0CallbackHelper1();
	int calcScreenMaskDx(int x, int y, int num);
	void lvlObjectType0CallbackBreathBubbles(LvlObject *ptr);
	void setupSpecialPowers(LvlObject *ptr);
	int lvlObjectType0Callback(LvlObject *ptr);
	int lvlObjectType1Callback(LvlObject *ptr);
	int lvlObjectType8Callback(LvlObject *o);
	int lvlObjectList3Callback(LvlObject *o);
	void lvlObjectSpecialPowersCallbackHelper1(LvlObject *o);
	uint8_t lvlObjectSpecialPowersCallbackScreen(LvlObject *o);
	int lvlObjectSpecialPowersCallback(LvlObject *o);
	void lvlObjectTypeCallback(LvlObject *o);
	LvlObject *addLvlObject(int type, int x, int y, int screen, int num, int o_anim, int o_flags1, int o_flags2, int actionKeyMask, int directionKeyMask);
	int setLvlObjectPosInScreenGrid(LvlObject *o, int pos);
	LvlObject *declareLvlObject(uint8_t type, uint8_t num);
	void clearDeclaredLvlObjectsList();
	void initLvlObjects();
	void setLvlObjectType8Resource(LvlObject *ptr, uint8_t _dl, uint8_t num);
	LvlObject *findLvlObject(uint8_t type, uint8_t spriteNum, int screenNum);
	LvlObject *findLvlObject2(uint8_t type, uint8_t flags, int screenNum);
	LvlObject *findLvlObjectType2(int spriteNum, int screenNum);
	LvlObject *findLvlObjectBoundingBox(BoundingBox *box);
	void resetMstOp57Sprites();
	void updateMstOp57Sprites();
	void captureScreenshot();

	// level1_rock.cpp
	void postScreenUpdate_rock_screen0();
	void postScreenUpdate_rock_screen4();
	void postScreenUpdate_rock_screen8();
	void postScreenUpdate_rock_screen9();
	void postScreenUpdate_rock_screen10();
	void postScreenUpdate_rock_screen11();
	void postScreenUpdate_rock_screen13();
	void postScreenUpdate_rock_screen15();
	void postScreenUpdate_rock_screen16();
	void postScreenUpdate_rock_screen18();
	void postScreenUpdate_rock_screen19();
	void callLevel_postScreenUpdate_rock(int num);

	int objectUpdate_rock_case0(LvlObject *o);
	void level1OpHelper1(LvlObject *ptr, uint8_t *p);
	int level1OpHelper2(LvlObject *ptr);
	int objectUpdate_rock_case1(LvlObject *o);
	int objectUpdate_rock_case2(LvlObject *o);
	int objectUpdate_rock_case3(LvlObject *o);
	int objectUpdate_rock_case4(LvlObject *o);

	void preScreenUpdate_rock_screen0();
	void preScreenUpdate_rock_screen1();
	void preScreenUpdate_rock_screen2();
	void preScreenUpdate_rock_screen3();
	void preScreenUpdate_rock_screen4();
	void preScreenUpdate_rock_screen5();
	void preScreenUpdate_rock_screen7();
	void preScreenUpdate_rock_screen9();
	void preScreenUpdate_rock_screen10();
	void preScreenUpdate_rock_screen13();
	void preScreenUpdate_rock_screen14();
	void preScreenUpdate_rock_screen15();
	void preScreenUpdate_rock_screen16();
	void preScreenUpdate_rock_screen17();
	void preScreenUpdate_rock_screen18();
	void preScreenUpdate_rock_screen19();
	void callLevel_preScreenUpdate_rock(int num);

	void callLevel_initialize_rock();
	void callLevel_tick_rock();
	void callLevel_terminate_rock();

	void setupLvlObjects_rock_screen2();
	void setupLvlObjects_rock_screen3();
	void setupLvlObjects_rock_screen18();
	void callLevel_setupLvlObjects_rock(int hum);

	// level2_fort.cpp
	void postScreenUpdate_fort_screen1();
	void postScreenUpdate_fort_screen6();
	void postScreenUpdate_fort_screen7();
	void postScreenUpdate_fort_screen8();
	void postScreenUpdate_fort_screen16();
	void postScreenUpdate_fort_screen17();
	void postScreenUpdate_fort_screen21();
	void callLevel_postScreenUpdate_fort(int num);

	void preScreenUpdate_fort_screen1();
	void preScreenUpdate_fort_screen2();
	void preScreenUpdate_fort_screen6();
	void preScreenUpdate_fort_screen9();
	void preScreenUpdate_fort_screen14();
	void preScreenUpdate_fort_screen16();
	void preScreenUpdate_fort_screen17();
	void preScreenUpdate_fort_screen21();
	void callLevel_preScreenUpdate_fort(int num);

	void callLevel_tick_fort();

	void setupLvlObjects_fort_screen1();
	void callLevel_setupLvlObjects_fort(int num);

	// level3_pwr1.cpp
	void postScreenUpdate_pwr1_helper(BoundingBox *b, int dx, int dy);
	void postScreenUpdate_pwr1_screen6();
	void postScreenUpdate_pwr1_screen10();
	void postScreenUpdate_pwr1_screen12();
	void postScreenUpdate_pwr1_screen14();
	void postScreenUpdate_pwr1_screen16();
	void postScreenUpdate_pwr1_screen18();
	void postScreenUpdate_pwr1_screen23();
	void postScreenUpdate_pwr1_screen27();
	void postScreenUpdate_pwr1_screen35();
	void callLevel_postScreenUpdate_pwr1(int num);

	void preScreenUpdate_pwr1_screen4();
	void preScreenUpdate_pwr1_screen6();
	void preScreenUpdate_pwr1_screen9();
	void preScreenUpdate_pwr1_screen15();
	void preScreenUpdate_pwr1_screen21();
	void preScreenUpdate_pwr1_screen23();
	void preScreenUpdate_pwr1_screen24();
	void preScreenUpdate_pwr1_screen26();
	void preScreenUpdate_pwr1_screen27();
	void preScreenUpdate_pwr1_screen29();
	void preScreenUpdate_pwr1_screen31();
	void preScreenUpdate_pwr1_screen35();
	void callLevel_preScreenUpdate_pwr1(int num);

	void callLevel_initialize_pwr1();
	void callLevel_tick_pwr1();

	// level4_isld.cpp
	void postScreenUpdate_isld_screen0();
	void postScreenUpdate_isld_screen1();
	void postScreenUpdate_isld_screen2();
	void postScreenUpdate_isld_screen3();
	void postScreenUpdate_isld_screen4();
	void postScreenUpdate_isld_screen8();
	void postScreenUpdate_isld_screen9();
	void postScreenUpdate_isld_screen13();
	void postScreenUpdate_isld_screen15();
	void postScreenUpdate_isld_screen19();
	void postScreenUpdate_isld_screen20();
	void postScreenUpdate_isld_screen21();
	void callLevel_postScreenUpdate_isld(int num);

	void preScreenUpdate_isld_screen1();
	void preScreenUpdate_isld_screen2();
	void preScreenUpdate_isld_screen3();
	void preScreenUpdate_isld_screen9();
	void preScreenUpdate_isld_screen14();
	void preScreenUpdate_isld_screen15();
	void preScreenUpdate_isld_screen16();
	void preScreenUpdate_isld_screen21();
	void callLevel_preScreenUpdate_isld(int num);

	void callLevel_initialize_isld();
	void callLevel_tick_isld();
	void callLevel_terminate_isld();

	// level5_lava.cpp
	void postScreenUpdate_lava_helper(int yPos);
	void postScreenUpdate_lava_screen0();
	void postScreenUpdate_lava_screen4();
	void postScreenUpdate_lava_screen5();
	void postScreenUpdate_lava_screen6();
	void postScreenUpdate_lava_screen7();
	void postScreenUpdate_lava_screen8();
	void postScreenUpdate_lava_screen10();
	void postScreenUpdate_lava_screen11();
	void postScreenUpdate_lava_screen12();
	void postScreenUpdate_lava_screen13();
	void postScreenUpdate_lava_screen14();
	void postScreenUpdate_lava_screen15();
	void callLevel_postScreenUpdate_lava(int num);

	void preScreenUpdate_lava_screen0();
	void preScreenUpdate_lava_screen3();
	void preScreenUpdate_lava_screen6();
	void preScreenUpdate_lava_screen10();
	void preScreenUpdate_lava_screen13();
	void preScreenUpdate_lava_screen15();
	void callLevel_preScreenUpdate_lava(int num);

	void callLevel_initialize_lava();
	void callLevel_tick_lava();

	void setupLvlObjects_lava_screen3();
	void callLevel_setupLvlObjects_lava(int num);

	// level6_pwr2.cpp
	void postScreenUpdate_pwr2_screen2();
	void postScreenUpdate_pwr2_screen3();
	void postScreenUpdate_pwr2_screen5();
	void postScreenUpdate_pwr2_screen8();
	void callLevel_postScreenUpdate_pwr2(int num);

	void preScreenUpdate_pwr2_screen2();
	void preScreenUpdate_pwr2_screen3();
	void preScreenUpdate_pwr2_screen5();
	void preScreenUpdate_pwr2_screen7();
	void callLevel_preScreenUpdate_pwr2(int num);

	void callLevel_initialize_pwr2();
	void callLevel_tick_pwr2();

	// level7_lar1.cpp
	void updateLevelTick_lar_helper1(int num, uint8_t *p1, const BoundingBox *r);
	void updateLevelTick_lar_helper2(int num, uint8_t *p1, const BoundingBox *b, const BoundingBox *r);
	void updateLevelTick_lar(int count, uint8_t *p1, const BoundingBox *r);
	void postScreenUpdate_lar1_helper(LvlObject *o, uint8_t *p, int num);

	void postScreenUpdate_lar1_screen0();
	void postScreenUpdate_lar1_screen3();
	void postScreenUpdate_lar1_screen4();
	void postScreenUpdate_lar1_screen5();
	void postScreenUpdate_lar1_screen8();
	void postScreenUpdate_lar1_screen9();
	void postScreenUpdate_lar1_screen13();
	void postScreenUpdate_lar1_screen15();
	void postScreenUpdate_lar1_screen16();
	void postScreenUpdate_lar1_screen18();
	void postScreenUpdate_lar1_screen19();
	void postScreenUpdate_lar1_screen20();
	void postScreenUpdate_lar1_screen22();
	void postScreenUpdate_lar1_screen24();
	void callLevel_postScreenUpdate_lar1(int num);

	void preScreenUpdate_lar1_screen0();
	void preScreenUpdate_lar1_screen2();
	void preScreenUpdate_lar1_screen6();
	void preScreenUpdate_lar1_screen11();
	void preScreenUpdate_lar1_screen13();
	void preScreenUpdate_lar1_screen14();
	void preScreenUpdate_lar1_screen16();
	void preScreenUpdate_lar1_screen20();
	void preScreenUpdate_lar1_screen24();
	void callLevel_preScreenUpdate_lar1(int num);

	void callLevel_initialize_lar1();
	void callLevel_tick_lar1();

	void callLevel_setupLvlObjects_lar1(int num);

	// level8_lar2.cpp
	bool postScreenUpdate_lar2_screen2_helper(BoundingBox *b);
	void postScreenUpdate_lar2_screen2();
	void postScreenUpdate_lar2_screen3();
	void postScreenUpdate_lar2_screen4();
	void postScreenUpdate_lar2_screen5();
	void postScreenUpdate_lar2_screen6();
	void postScreenUpdate_lar2_screen7();
	void postScreenUpdate_lar2_screen8();
	void postScreenUpdate_lar2_screen10();
	void postScreenUpdate_lar2_screen11();
	void postScreenUpdate_lar2_screen12();
	void postScreenUpdate_lar2_screen13();
	void postScreenUpdate_lar2_screen19();
	void callLevel_postScreenUpdate_lar2(int num);

	void preScreenUpdate_lar2_screen2();
	void preScreenUpdate_lar2_screen4();
	void preScreenUpdate_lar2_screen5();
	void preScreenUpdate_lar2_screen6();
	void preScreenUpdate_lar2_screen7();
	void preScreenUpdate_lar2_screen8();
	void preScreenUpdate_lar2_screen9();
	void preScreenUpdate_lar2_screen15();
	void preScreenUpdate_lar2_screen19();
	void callLevel_preScreenUpdate_lar2(int num);

	void callLevel_tick_lar2();

	void setupLvlObjects_lar2_screen19();
	void callLevel_setupLvlObjects_lar2(int num);

	// level9_dark.cpp
	void postScreenUpdate_dark_screen0();
	void callLevel_postScreenUpdate_dark(int num);

	void preScreenUpdate_dark_screen0();
	void callLevel_preScreenUpdate_dark(int num);

	// monsters.cpp

	void resetMonsterObject1(MonsterObject1 *m);
	void initMonsterObject1(MonsterObject1 *m);
	bool addChasingMonster(MstUnk48 *m48, uint8_t flag);
	void disableMonsterObject1(MonsterObject1 *m);
	void copyMonsterObject1(Task *t, MonsterObject1 *m, int num);
	int mstTaskStopMonsterObject1(Task *t);
	void updateMstLvlObjectPos(MonsterObject1 *m);
	bool updateMonsterObject1PositionHelper(MonsterObject1 *m);
	bool updateMonsterObject1Position(MonsterObject1 *m);
	void initMonsterObject2_firefly(MonsterObject2 *m);
	void resetMonsterObject2(MonsterObject2 *m);
	int prepareMstTask(Task *t);

	void mstBoundingBoxClear(MonsterObject1 *m, int dir);
	int mstBoundingBoxCollides1(int num, int x1, int y1, int x2, int y2);
	int mstBoundingBoxUpdate(int num, int a, int x1, int y1, int x2, int y2);
	int mstBoundingBoxCollides2(int num, int x1, int y1, int x2, int y2);

	void mstTaskSetScreenPosition(Task *t);
	int getMstDistance(int y, const MovingOpcodeState *p) const;
	void mstTaskUpdateScreenPosition(Task *t);
	void shuffleMstUnk43(MstUnk43 *p);

	void initMstCode();
	void resetMstCode();
	void startMstCode();
	void executeMstCode();
	void executeMstCodeHelper1();
	void executeMstCodeHelper2();
	void mstLvlObjectSetActionDirection(LvlObject *o, const uint8_t *ptr, uint8_t mask1, uint8_t mask2);
	void executeMstUnk4(MonsterObject1 *m);
	void mstSetVerticalHorizontalBounds(MonsterObject1 *m);
	bool executeMstUnk6(MonsterObject1 *m);
	void executeMstUnk8(MonsterObject1 *m);
	int executeMstUnk9(Task *t, MonsterObject1 *m);
	int executeMstUnk11(Task *t, MonsterObject1 *m);
	bool mstTestActionDirection(MonsterObject1 *m, int num);
	bool executeMstUnk19(LvlObject *o, int type);
	bool executeMstUnk21(LvlObject *o, int type);
	bool executeMstUnk28(LvlObject *o, int type) const;
	bool executeMstUnk22(LvlObject *o, int type);
	bool mstCollidesByFlags(MonsterObject1 *m, uint32_t flags);
	bool executeMstUnk27(MonsterObject1 *m, const uint8_t *p);
	int mstUpdateTaskMonsterObject1(Task *t);
	int mstUpdateTaskMonsterObject2(Task *t);
	void mstUpdateRefPos();
	void updateMstHeightMapData();

	void mstRemoveMonsterObject2(Task *t, Task **tasksList);
	void mstTaskAttack(Task *t, uint32_t codeData, uint8_t flags);
	void mstRemoveMonsterObject1(Task *t, Task **tasksList);
	Task *findFreeTask();
	Task *createTask(const uint8_t *codeData);
	int mstTaskSetActionDirection(Task *t, int num, int value);
	void updateTask(Task *t, int num, const uint8_t *codeData);
	void resetTask(Task *t, const uint8_t *codeData);
	void removeTask(Task **tasksList, Task *t);
	void appendTask(Task **tasksList, Task *t);
	int getTaskVar(Task *t, int index, int type) const;
	void setTaskVar(Task *t, int index, int type, int value);
	int getTaskAndyVar(int index, Task *t) const;
	int getTaskOtherVar(int index, Task *t) const;
	int getTaskFlag(Task *t, int index, int type) const;
	int runTask_default(Task *t);
	void mstOp26_removeMstTaskScreen(Task **tasksList, int screenNum);
	void mstOp27_removeMstTaskScreenFlags(Task **tasksList, int num, int arg);
	int mstOp49_setMovingBounds(int a, int b, int c, int d, int screen, Task *t, int num);
	void mstOp52();
	bool mstCollidesDirection(MstUnk48 *m, uint8_t flag);
	void mstOp53(MstUnk48 *m);
	void mstOp54();
	int mstOp56_specialAction(Task *t, int code, int num);
	void mstOp57_addSprite(int x, int y, int screenNum);
	void mstOp58_addLvlObject(Task *t, int num);
	void mstOp59_1(int x, int y, int screenNum, int type, uint16_t flags);
	void mstOp59_2(int x, int y, int screenNum, int type, uint16_t flags);
	void executeMstUnk1(Task *t);
	bool mstSetCurrentPos(MonsterObject1 *m, int x, int y);
	void mstSetHorizontalBounds(MonsterObject1 *m);
	void mstResetCollisionTable();
	void executeMstUnk13(Task *t);
	bool executeMstUnk14(MonsterObject1 *m, int x, int y, uint8_t dir);
	int executeMstUnk23(Task *t);
	int executeMstOp67Type1(Task *t);
	int executeMstOp67Type2(Task *t, int flag);
	void mstOp67_addMonster(Task *t, int y1, int y2, int x1, int x2, int screen, int type, int o_flags1, int o_flags2, int arg1C, int arg20, int arg24);
	void mstOp68(Task *t, const uint8_t *p, int a, int b, int c, int d);
	int runTask_waitResetInput(Task *t);
	int runTask_wait(Task *t);
	int runTask_waitFlags(Task *t);
	int runTask_idle(Task *t);
	int runTask_mstOp231(Task *t);
	int runTask_mstOp232(Task *t);
	int runTask_mstOp233(Task *t);
	int runTask_mstOp234(Task *t);
	int runTask_unk1(Task *t);
	int runTask_unk2(Task *t);
	int runTask_unk3(Task *t);
	int runTask_unk4(Task *t);
	int runTask_unk5(Task *t);
	int runTask_unk6(Task *t);
	int runTask_unk7(Task *t);
	int runTask_unk8(Task *t);
	int runTask_unk9(Task *t);
	int runTask_unk10(Task *t);
	int runTask_unk11(Task *t);

	// sound.cpp
	bool _sssDisabled;
	SssObject _sssObjectsTable[kMaxSssObjects];
	bool _sssObjectsChanged;
	int _sssObjectsCount;
	SssObject *_sssObjectsList1; // playing
	SssObject *_sssObjectsList2; // paused/idle
	SssObject *_lowPrioritySssObject; // point to the object in _sssObjectsList1 with the highest 'priority'
	bool _sssUpdatedObjectsTable[kMaxSssObjects];
	int _playingSssObjectsMax;
	int _snd_masterPanning;
	int _playingSssObjectsCount;
	int _snd_masterVolume;

	void resetSound();
	void removeSoundObjectFromList(SssObject *so);
	void updateSoundObject(SssObject *so);
	void sssOp4_removeSounds(uint32_t flags);
	void sssOp12_removeSounds2(int num, uint8_t lut, uint8_t c);
	void sssOp16_resumeSound(SssObject *so);
	void sssOp17_pauseSound(SssObject *so);
	const uint8_t *executeSssCode(SssObject *so, const uint8_t *code, bool tempSssObject = false);
	SssObject *addSoundObject(SssPcm *pcm, int priority, uint32_t flags_a, uint32_t flags_b);
	void prependSoundObjectToList(SssObject *so);
	void updateSoundObjectLut2(uint32_t flags);
	SssObject *createSoundObject(int num, int b, int c);
	SssObject *startSoundObject(int num, int b, uint32_t flags);
	void playSoundObject(SssUnk1 *s, int a, int b);
	void clearSoundObjects();
	void setLowPrioritySoundObject(SssObject *so);
	int getSoundObjectPanning(SssObject *so) const;
	void setSoundObjectPanning(SssObject *so);
	void expireSoundObjects(uint32_t flags);
	void mixSoundObjects17640(bool flag);
	void mixSoundObjects();
	void stopSoundObject(SssObject **sssObjectsList, int num);

	// andy.cpp

	AndyMoveData _andyMoveData;
	int32_t _andyPosX;
	int32_t _andyPosY;
	uint8_t _andyMoveMask;
	bool _andyUpdatePositionFlag;
	const Point16_t *_andyLevelData0x288PosTablePtr;
	uint8_t _andyMoveState[32];
	int _andyMaskBufferPos1;
	int _andyMaskBufferPos2;
	int _andyMaskBufferPos4;
	int _andyMaskBufferPos5;
	int _andyMaskBufferPos3;
	int _andyMaskBufferPos0;
	int _andyMaskBufferPos7;
	int _andyMaskBufferPos6;

	int moveAndyObjectOp1(int op);
	int moveAndyObjectOp2(int op);
	int moveAndyObjectOp3(int op);
	int moveAndyObjectOp4(int op);
	void setupAndyObjectMoveData(LvlObject *ptr);
	void setupAndyObjectMoveState();
	void updateAndyObject(LvlObject *ptr);
};

#endif // GAME_H__
