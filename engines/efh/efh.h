/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef EFH_H
#define EFH_H

#include "audio/softsynth/pcspk.h"
#include "audio/mixer.h"
#include "common/file.h"
#include "common/rect.h"
#include "common/events.h"
#include "common/serializer.h"

#include "engines/advancedDetector.h"
#include "engines/engine.h"
#include "graphics/surface.h"

#include "efh/constants.h"

namespace Common {
class RandomSource;
}

/**
 * This is the namespace of the Efh engine.
 *
 * Status of this engine:
 * - No music in intro
 * - No random PC speaker farts (aka sounds)
 * - The rest is more or less working :)
 *
 * Games using this engine:
 * - Escape From Hell
 *
 * Note: Wasteland and Fountain of dreams *seem* to use the same engine, but it's not the case.
 * Escape From Hell was written from scratch based on the visual look of the other
 */
namespace Efh {

static const uint8 kSavegameVersion = 1;
#define EFH_SAVE_HEADER MKTAG('E', 'F', 'H', 'S')

enum EfhDebugChannels {
	kDebugEngine = 1,
	kDebugUtils,
	kDebugGraphics,
	kDebugScript,
	kDebugFight,
};

class EfhGraphicsStruct {
public:
	EfhGraphicsStruct();
	EfhGraphicsStruct(int8 **lineBuf, int16 x, int16 y, int16 width, int16 height);

	int8 **_vgaLineBuffer;
	uint16 _shiftValue;
	uint16 _width;
	uint16 _height;
	Common::Rect _area;

	void copy(EfhGraphicsStruct *src);
};

struct InvObject {
	int16 _ref;
	uint8 _stat1; // abbb bbbb - a: equipped b: uses left
	uint8 _curHitPoints;

	void init();
	bool isEquipped();
	int8 getUsesLeft();
};

struct MapSpecialTileStruct {
	uint8 _placeId;
	uint8 _posX;
	uint8 _posY;
	uint8 _triggerType; // 0xFD = Check inventory 0xFE = Check Character in team 0xFF Display description <= 0x77 = check score (all values in this case in data are <= 0xF)
	uint8 _triggerValue;
	uint16 _field5_textId;
	uint16 _field7_textId;

	void init();
};

struct FrameList {
	int8 _subFileId[4];

	void init();
};
struct AnimInfo {
	uint16 _posX[10];
	uint8 _posY[10];
	FrameList _frameList[15];

	void init();
};

struct ItemStruct {
	char _name[15];
	uint8 _damage;
	uint8 _defense;
	uint8 _attacks;
	uint8 _uses;
	int8 _agilityModifier; // data contains values from -8 to +8
	uint8 _range;
	uint8 _attackType;
	uint8 _specialEffect;
	uint8 _defenseType;
	uint8 _exclusiveType;
	uint8 _field19_mapPosX_or_maxDeltaPoints;
	uint8 _mapPosY;

	void init();
};

struct NPCStruct {
	char _name[11];
	uint8 fieldB_textId;
	uint8 field_C;
	uint8 field_D;
	uint8 fieldE_textId;
	uint8 field_F;
	uint8 field_10;
	uint8 field11_NpcId;
	uint16 field12_textId;
	uint16 field14_textId;
	uint32 _xp;
	uint8 _activeScore[15];
	uint8 _passiveScore[11];
	uint8 _infoScore[11];
	uint8 field_3F;
	uint8 field_40;
	InvObject _inventory[10];
	uint8 _possessivePronounSHL6;
	uint8 _speed;
	uint8 field_6B;
	uint8 field_6C;
	uint8 field_6D;
	uint8 _defaultDefenseItemId;
	uint8 field_6F;
	uint8 field_70;
	uint8 field_71;
	uint8 field_72;
	uint8 field_73;
	int16 _hitPoints;
	int16 _maxHP;
	uint8 field_78;
	uint16 field_79;
	uint16 field_7B;
	uint8 field_7D;
	uint8 field_7E;
	uint8 field_7F;
	uint8 field_80;
	uint8 field_81;
	uint8 field_82;
	uint8 field_83;
	uint8 field_84;
	uint8 field_85;

	void init();
	uint8 getPronoun();
	void synchronize(Common::Serializer &s);
};

struct FontDescr {
	const uint8 *_widthArray;
	const uint8 *_extraLines;
	const Font  *_fontData;
	uint8 _charHeight;
	uint8 _extraVerticalSpace;
	uint8 _extraHorizontalSpace;
};

struct BufferBM {
	uint8 *_dataPtr;
	uint16 _width;
	uint16 _startX;
	uint16 _startY;
	uint16 _height;
	uint16 _lineDataSize;
	uint8 _paletteTransformation;
	uint16 _fieldD;
};

struct CharStatus {
	int16 _type;
	int16 _duration;
};

struct MapMonster {
	uint8 _possessivePronounSHL6; // aabb bbbb aa:Possessive Pronoun, bb bbbb: unknown
	uint8 _npcId;
	uint8 _fullPlaceId; // unsigned? Magic values are 0xFF and 0xFE
	uint8 _posX;
	uint8 _posY;
	uint8 _weaponItemId;
	uint8 _maxDamageAbsorption;
	uint8 _monsterRef;
	uint8 _additionalInfo; // abbb cddd a: special move flag, bbb: Pct modifier for random move, c aggressiveness, ddd move type
	uint8 _talkTextId;
	uint8 _groupSize;
	int16 _hitPoints[9];

	uint8 getPronoun();
};

struct InitiativeStruct {
	int16 _id;
	int16 _initiative;

	void init();
};

struct TileFactStruct {
	uint8 _status;
	uint8 _tileId;

	void init();
};

struct TeamChar {
	int16 _id;
	CharStatus _status;
	int16 _pctVisible;
	int16 _pctDodgeMiss;
	int16 _nextAttack;
	int16 _lastInventoryUsed;
	int16 _lastAction;

	void init();
};

struct TeamMonster {
	int16 _id;
	CharStatus _mobsterStatus[9];

	void init();
};

enum EFHAction {
	kActionNone,
	kActionExit,
	kActionSave,
	kActionLoad,
	kActionMoveUp,
	kActionMoveDown,
	kActionMoveLeft,
	kActionMoveRight,
	kActionMoveUpLeft,
	kActionMoveUpRight,
	kActionMoveDownLeft,
	kActionMoveDownRight,
	kActionCharacter1Status,
	kActionCharacter2Status,
	kActionCharacter3Status
};

class EfhEngine : public Engine {
public:
	EfhEngine(OSystem *syst, const ADGameDescription *gd);
	~EfhEngine() override;

	Graphics::Surface *_mainSurface;
	Common::RandomSource *_rnd;

	const ADGameDescription *_gameDescription;

	// metaengine.cpp
	void initGame(const ADGameDescription *gd);
	uint32 getFeatures() const;
	const char *getGameId() const;
	Common::Platform getPlatform() const;
	bool hasFeature(EngineFeature f) const override;
	const char *getCopyrightString() const;

	// savegames.cpp
	Common::String getSavegameFilename(int slot);
	bool canLoadGameStateCurrently(Common::U32String *msg = nullptr) override;
	bool canSaveGameStateCurrently(Common::U32String *msg = nullptr) override;
	Common::Error loadGameState(int slot) override;
	Common::Error saveGameState(int slot, const Common::String &desc, bool isAutosave = false) override;

protected:
	int _lastTime;
	// Engine APIs
	Common::Error run() override;

private:
	Common::Platform _platform;
	int _loadSaveSlot;
	bool _saveAuthorized;
	Common::CustomEventType _customAction = kActionNone;

	void initialize();
	void playIntro();
	void initEngine();
	void initMapMonsters();
	void saveAnimImageSetId();
	int16 getEquipmentDefense(int16 charId);
	uint16 getEquippedExclusiveType(int16 charId, int16 exclusiveType, bool flag);
	void displayLowStatusScreen(bool flag);
	void loadImageSetToTileBank(int16 bankId, int16 setId);
	void restoreAnimImageSetId();
	void checkProtection();
	void loadEfhGame();
	void copyCurrentPlaceToBuffer(int16 id);
	uint8 getMapTileInfo(int16 mapPosX, int16 mapPosY);
	uint16 getStringWidth(const Common::String &str) const;
	void setTextPos(int16 textPosX, int16 textPosY);
	void drawGameScreenAndTempText(bool flag);
	void drawMap(bool largeMapFl, int16 mapPosX, int16 mapPosY, int16 mapSize, bool drawHeroFl, bool drawMonstersFl);
	void displaySmallMap(int16 posX, int16 posY);
	void displayLargeMap(int16 posX, int16 posY);
	void drawScreen();
	void removeObject(int16 charId, int16 objectId);
	void totalPartyKill();
	void removeCharacterFromTeam(int16 teamMemberId);
	void refreshTeamSize();
	bool isNpcATeamMember(int16 id);
	void handleWinSequence();
	bool giveItemTo(int16 charId, int16 objectId, int16 fromCharId);
	int16 chooseCharacterToReplace();
	int16 handleCharacterJoining();
	void drawText(uint8 *srcPtr, int16 posX, int16 posY, int16 maxX, int16 maxY, bool flag);
	void displayMiddleLeftTempText(uint8 *impArray, bool flag);
	void transitionMap(int16 centerX, int16 centerY);
	void setSpecialTechZone(int16 unkId, int16 centerX, int16 centerY);
	int16 findMapSpecialTileIndex(int16 posX, int16 posY);
	bool isPosOutOfMap(int16 mapPosX, int16 mapPosY);
	void goSouth();
	void goNorth();
	void goEast();
	void goWest();
	void goNorthEast();
	void goSouthEast();
	void goNorthWest();
	void goSouthWest();
	void showCharacterStatus(uint8 character);
	void handleNewRoundEffects();
	void resetGame();
	void computeMapAnimation();
	void handleAnimations();
	void handleEvents();
	int8 checkMonsterMoveCollisionAndTileTexture(int16 monsterId);
	bool moveMonsterAwayFromTeam(int16 monsterId);
	bool moveMonsterTowardsTeam(int16 monsterId);
	bool moveMonsterGroupOther(int16 monsterId, int16 direction);
	bool moveMonsterGroupRandom(int16 monsterId);
	int16 computeMonsterGroupDistance(int16 monsterId);
	bool checkWeaponRange(int16 monsterId, int16 weaponId);
	bool checkMonsterMovementType(int16 id, bool teamFlag);
	bool checkTeamWeaponRange(int16 monsterId);
	bool checkIfMonsterOnSameLargeMapPlace(int16 monsterId);
	bool checkMonsterWeaponRange(int16 monsterId);
	void handleMapMonsterMoves();
	bool checkMapMonsterAvailability(int16 monsterId);
	void displayMonsterAnim(int16 monsterId);
	int16 countAliveMonsters(int16 id);
	bool checkMonsterGroupDistance1OrLess(int16 monsterId);
	bool handleTalk(int16 monsterId, int16 arg2, int16 itemId);
	void startTalkMenu(int16 monsterId);
	void displayImp1Text(int16 textId);
	bool handleInteractionText(int16 mapPosX, int16 mapPosY, int16 charId, int16 itemId, int16 arg8, int16 imageSetId);
	int8 checkTileStatus(int16 mapPosX, int16 mapPosY, bool teamFl);
	void computeInitiatives();
	void redrawScreenForced();
	int16 countMonsterGroupMembers(int16 monsterGroup);
	uint16 getXPLevel(uint32 xp);
	bool isItemCursed(int16 itemId);
	bool hasObjectEquipped(int16 charId, int16 objectId);
	void setMapMonsterAggressivenessAndMovementType(int16 id, uint8 mask);
	bool isMonsterActive(int16 groupId, int16 id);
	int16 getTileFactId(int16 mapPosX, int16 mapPosY);
	void setCharacterObjectToBroken(int16 charId, int16 objectId);
	int16 selectOtherCharFromTeam();
	bool checkMonsterCollision();

	// Fight
	bool handleFight(int16 monsterId);
	void handleFight_checkEndEffect(int16 charId);
	void handleFight_lastAction_A(int16 teamCharId);
	void handleFight_lastAction_D(int16 teamCharId);
	void handleFight_lastAction_H(int16 teamCharId);
	bool handleFight_lastAction_U(int16 teamCharId);
	void handleFight_MobstersAttack(int groupId);
	bool isTPK();
	bool isMonsterAlreadyFighting(int16 monsterId, int16 teamMonsterId);
	void createOpponentList(int16 monsterTeamId);
	void resetTeamMonsterEffects();
	void initFight(int16 monsterId);
	void resetTeamMonsterIdArray();
	bool isTeamMemberStatusNormal(int16 id);
	void getDeathTypeDescription(int16 victimId, int16 attackerId);
	int16 determineTeamTarget(int16 charId, int16 unkFied18Val, bool checkDistanceFl);
	bool getTeamAttackRoundPlans();
	void drawCombatScreen(int16 charId, bool whiteFl, bool drawFl);
	void getXPAndSearchCorpse(int16 charId, Common::String namePt1, Common::String namePt2, int16 monsterId);
	bool characterSearchesMonsterCorpse(int16 charId, int16 monsterId);
	void addReactionText(int16 id);
	void displayEncounterInfo(bool WhiteFl);
	int16 getWeakestMobster(int16 groupNumber);
	int16 getCharacterScore(int16 charId, int16 itemId);
	bool checkSpecialItemsOnCurrentPlace(int16 itemId);
	bool hasAdequateDefense(int16 monsterId, uint8 attackType);
	bool hasAdequateDefenseNPC(int16 charId, uint8 attackType);
	void addNewOpponents(int16 monsterId);
	int16 getTeamMonsterAnimId();
	int16 selectMonsterGroup();
	void redrawCombatScreenWithTempText(int16 charId);
	void handleDamageOnArmor(int16 charId, int16 damage);

	// Files
	int32 readFileToBuffer(const Common::Path &filename, uint8 *destBuffer);
	void readAnimInfo();
	void findMapFile(int16 mapId);
	void rImageFile(const Common::Path &filename, uint8 *targetBuffer, uint8 **subFilesArray, uint8 *packedBuffer);
	void readItems();
	void readImpFile(int16 id, bool techMapFl);
	void loadNewPortrait();
	void loadAnimImageSet();
	void loadHistory();
	void loadTechMapImp(int16 fileId);
	void loadPlacesFile(uint16 fullPlaceId, bool forceReloadFl);
	void readTileFact();
	void loadNPCS();
	void preLoadMaps();

	// Graphics
	void initPalette();
	void drawLeftCenterBox();
	void displayNextAnimFrame();
	void displayAnimFrame();
	void displayAnimFrames(int16 animId, bool displayMenuBoxFl);
	void displayFctFullScreen();
	void copyDirtyRect(int16 minX, int16 minY, int16 maxX, int16 maxY);
	void copyGraphicBufferFromTo(EfhGraphicsStruct *efh_graphics_struct, EfhGraphicsStruct *efh_graphics_struct1, const Common::Rect &rect, int16 min_x, int16 min_y);
	void displayBufferBmAtPos(BufferBM *bufferBM, int16 posX, int16 posY);
	void drawRect(int16 minX, int16 minY, int16 maxX, int16 maxY);
	void drawColoredRect(int16 minX, int16 minY, int16 maxX, int16 maxY, int16 color);
	void clearScreen(int16 color);
	void displayRawDataAtPos(uint8 *imagePtr, int16 posX, int16 posY);
	void drawString(const Common::String &str, int16 startX, int16 startY, uint16 textColor);
	void displayCenteredString(const Common::String &str, int16 minX, int16 maxX, int16 posY);
	void displayMenuAnswerString(const Common::String &str, int16 minX, int16 maxX, int16 posY);
	void drawMapWindow();
	void displayGameScreen();
	void drawUpperLeftBorders();
	void drawUpperRightBorders();
	void drawBottomBorders();
	void drawChar(uint8 curChar, int16 posX, int16 posY);
	void setTextColorWhite();
	void setTextColorRed();
	void setTextColorGrey();
	void displayStringAtTextPos(const Common::String &message);
	void clearBottomTextZone(int16 color);
	void clearBottomTextZone_2(int16 color);
	void setNextCharacterPos();
	void displayCharAtTextPos(char character);
	void displayWindow(uint8 *buffer, int16 posX, int16 posY, uint8 *dest);
	void displayColoredMenuBox(int16 minX, int16 minY, int16 maxX, int16 maxY, int16 color);

	// Menu
	int16 displayBoxWithText(const Common::String &str, int16 menuType, int16 displayOption, bool displayTeamWindowFl);
	bool handleDeathMenu();
	void displayCombatMenu(int16 charId);
	void displayMenuItemString(int16 menuBoxId, int16 thisBoxId, int16 minX, int16 maxX, int16 minY, const char *str);
	void displayStatusMenu(int16 windowId);
	void prepareStatusRightWindowIndexes(int16 menuId, int16 charId);
	void displayCharacterSummary(int16 curMenuLine, int16 npcId);
	void displayCharacterInformationOrSkills(int16 curMenuLine, int16 npcId);
	void displayStatusMenuActions(int16 menuId, int16 curMenuLine, int16 npcId);
	void prepareStatusMenu(int16 windowId, int16 menuId, int16 curMenuLine, int16 charId, bool refreshFl);
	void displayWindowAndStatusMenu(int16 charId, int16 windowId, int16 menuId, int16 curMenuLine);
	int16 displayStringInSmallWindowWithBorder(const Common::String &str, bool delayFl, int16 charId, int16 windowId, int16 menuId, int16 curMenuLine);
	int16 handleStatusMenu(int16 gameMode, int16 charId);
	void unequipItem(int16 charId, int16 objectId, int16 windowId, int16 menuId, int16 curMenuLine);
	void tryToggleEquipped(int16 charId, int16 objectId, int16 windowId, int16 menuId, int16 curMenuLine);
	int16 useObject(int16 charId, int16 objectId, int16 teamMonsterId, int16 menuId, int16 curMenuLine, int16 gameMode);

	// Savegames
	void synchronize(Common::Serializer &s);

	// Script
	const uint8 *script_readNumberArray(const uint8 *buffer, int16 destArraySize, int16 *destArray);
	const uint8 *script_getNumber(const uint8 *srcBuffer, int16 *retBuf);
	int16 script_parse(Common::String str, int16 posX, int16 posY, int16 maxX, int16 maxY, bool scriptExecuteFlag);

	// Sound
	void songDelay(int delay);
	void playNote(int frequencyIndex, int totalDelay);
	Common::KeyCode playSong(uint8 *buffer);
	void generateSound1(int lowFreq, int highFreq, int duration);
	void generateSound2(int startFreq, int endFreq, int speed);
	void generateSound3();
	void generateSound4(int repeat);
	void generateSound5(int repeat);
	void generateSound(int16 soundType);
	void genericGenerateSound(int16 soundType, int16 repeatCount);

	// Utils
	void decryptImpFile(bool techMapFl);
	void loadImageSet(int16 imageSetId, uint8 *buffer, uint8 **subFilesArray, uint8 *destBuffer);
	uint32 uncompressBuffer(uint8 *compressedBuf, uint8 *destBuf);
	int16 getRandom(int16 maxVal);
	Common::KeyCode getLastCharAfterAnimCount(int16 delay);
	Common::KeyCode getInput(int16 delay);
	Common::KeyCode waitForKey();
	Common::KeyCode handleAndMapInput(bool animFl);
	bool getValidationFromUser();
	uint32 ROR(uint32 val, uint8 shiftVal);
	Common::String getArticle(int pronoun);

	// Actions
	void handleActionSave();
	void handleActionLoad();

	uint8 _videoMode;
	uint8 _bufferCharBM[128];
	int8 *_vgaLineBuffer[200];
	EfhGraphicsStruct *_vgaGraphicsStruct1;
	EfhGraphicsStruct *_vgaGraphicsStruct2;
	EfhGraphicsStruct *_graphicsStruct;
	uint8 _tileBank[3][12000];
	uint8 _circleImageBuf[40100];
	uint8 _portraitBuf[25000];
	uint8 _decompBuf[40100];
	uint8 _menuBuf[12500];
	uint8 _windowWithBorderBuf[1500];
	uint8 _mapArr[19][7000];
	uint8 _places[12000];
	uint8 _curPlace[24][24];
	NPCStruct _npcBuf[100];
	uint8 _imp1[13000];
	uint8 _imp2[10000];
	uint8 _titleSong[1024];
	ItemStruct _items[300];
	TileFactStruct _tileFact[432];
	AnimInfo _animInfo[100];
	uint8 _history[256];
	uint8 _techDataArr[19][4100];
	Common::String _enemyNamePt1;
	Common::String _enemyNamePt2;
	Common::String _characterNamePt1;
	Common::String _characterNamePt2;
	Common::String _nameBuffer;
	Common::String _attackBuffer;
	Common::String _messageToBePrinted;

	struct BitmapRef {
		int8 _setId1;
		int8 _setId2;
	};

	BitmapRef _mapBitmapRefArr[19];

	MapSpecialTileStruct _mapSpecialTiles[19][100];
	MapMonster _mapMonsters[19][64];
	uint8 _mapGameMaps[19][64][64];

	uint8 _defaultBoxColor;
	FontDescr _fontDescr;

	bool _introDoneFl;
	uint16 _textColor;

	int16 _oldAnimImageSetId;
	int16 _animImageSetId;
	uint8 _paletteTransformationConstant;
	uint8 *_circleImageSubFileArray[12];
	uint8 *_tileBankSubFilesArray[216];
	BufferBM _imageDataPtr;
	int16 _currentTileBankImageSetId[3];
	int16 _unkRelatedToAnimImageSetId;
	int16 _techId;
	int16 _currentAnimImageSetId;
	uint8 *_portraitSubFilesArray[20];
	int16 _unkAnimRelatedIndex;
	uint8 *_imp1PtrArray[100];
	uint8 *_imp2PtrArray[432];
	uint16 _fullPlaceId;
	int16 _guessAnimationAmount;
	uint16 _largeMapFlag; // CHECKME: bool?
	int16 _textPosX;
	int16 _textPosY;

	Common::Rect _initRect;
	bool _engineInitPending;
	bool _protectionPassed;

	int16 _alertDelay;
	int16 _teamSize;
	int16 _word2C872;
	bool _checkTileDisabledByScriptFl;
	bool _redrawNeededFl;
	bool _drawHeroOnMapFl;
	bool _drawMonstersOnMapFl;
	bool _textBoxDisabledByScriptFl;

	int16 _imageSetSubFilesIdx;
	int16 _oldImageSetSubFilesIdx;

	int16 _mapPosX, _mapPosY;
	int16 _oldMapPosX, _oldMapPosY;
	int16 _techDataId_MapPosX, _techDataId_MapPosY;
	uint16 _lastMainPlaceId;

	uint16 _tempTextDelay;
	uint8 *_tempTextPtr;

	bool _ongoingFightFl;
	bool _statusMenuActive;
	int16 _menuStatItemArr[15];
	int16 _menuDepth;
	int16 _menuItemCounter;

	TeamChar _teamChar[3];
	TeamMonster _teamMonster[5];

	InitiativeStruct _initiatives[8];

	int16 _regenCounter;

	Audio::PCSpeaker *_speaker;
};

} // End of namespace Efh

#endif
