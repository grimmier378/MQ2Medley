// MQ2Medley.cpp - Bard song scheduling plugin for MacroQuest2
//
// Changelog
//   2015-07-08 Winnower	 - Initial release
//   2016-06-22 Dewey		 - Updated for current core MQ source: bump to v 1.01
//							   - fixed using instant cast AA's
//							   - fixed TLO ${Medley.Tune} so it does not CTD
//							   - fixed TLO ${Medley.Medley} so it returns correct set.
//							   - /medley quiet is now a lot more quiet.
//   2016-06-22 Dewey		 - Updated for current core MQ source: bump to v 1.02
//							 - fixed recast calculations due to core changes to TLO ${Spell[].CastTime}
//							 - /medley debug now toggles some extra debug spam.
//   2016-07-10 Dewey		 - Added SongIF which works like MQ2Melee SkillIF[] : bump to v 1.03
//							   - /medley debug now saves its state and shows which song it is evaluating.
//   2016-07-11 Dewey		 - Medley is now much more persistent, like priority version of MQ2Melee. Bump to v1.04
//							   - Any time  Medley set or Twist changes state it should write Medley=[SET] and Playing=[0|1] to the ini.
//							   - Sitting, Hover, Death and FD no longer turn off twist, instead they pause playback.
//   2016-07-17 Dewey		 - Modified Once/Queue'd spells now check to see if they are ready to cast. Bump to v1.05
//							   - Modified profile loading.Should no longer spam NULL songs when loading empty profiles.
//   2016-07-18 Dewey		 - Modified once so it shares the same spell list.  v1.06
//							   - ${Medley.getTimeTillQueueEmpty} broken.
//							   - Experimental support for dynamic target from profile supporting mezing XTarget[n]
//   2016-07-26 Dewey		 - Updated for the eqMules MANDITORY STRING SAFETY DANCE.  v1.07
//
//	 2023-12-31 Grimmier	 - Added in bandolier swapping, 
//								-song1=song\aa\itename^duration^condition^bandoliername
//								-defaults to 1 for no swap. 
//								-otherwise plug in the name from bandolier hotkey. ie drum, lute, etc however you named them in game.
//	 2024-01-01 Grimmier	 - Moved the Bandolier setting to the end of the arguments
// 
//	 2024-01-15 Grimmier	 - Moved the Bandolier settings out of the song string in the INI. 
//								-Now in the ini for each son is a matching bandolier#= setting 
//								-so song1 matches bandolier1 this way we don't break anyones ini's 
//								-Defaults to a value of 1 which is no swapping.
//	 2024-01-16 Grimmier	 - Fixed the seperator to be a | instead of the ^
//								- Moved Conditions to their on settings, similar to other ini files CondSize= max number of conditions
//								- cond1= condition 1 etc. 
//								- Song1=SongName|Duration|SongType|Condition
//								- example: song1=Selo's Accelerating Chorus|30|percussion|cond1
//								- Added in the choice of either bandolier support or MQ2Exchange support for weapon swapping
//								- To use Bandolier instead of MQ2Exchange set Bandolier=1 in the settings 
//								- Added MainHand and OffHand settings to config to set your default weapon set
//								- Added SongType to the data string valid entries are {sing, percussion, wind, brass, string}
//								- added settings under the Medley for each SongType formated type=itemname|itemslot 
//								- Ex: sing=Singing Short Sword|mainhand
//	2024-01-18 Grimmier 	- Added some checks for parsing the ini. Now only the Name and Duration are required. 
//							-you can use the following formats. 
//								** All Conditions present **
//							-song1=Some Run Song|10|percussion|cond3 
//								** Name Duration and Condition present type fills in as noswap **
//							-song1=Some Run Song|10|cond3
//								** Name Duration Type condition Condition defaults to 1 **
//							-song1=Some Run Song|10|percussion
//								** Only Song Name and Duration. no condition and no songtype they default to 1 and noswap.**
//							-song1=Some Run Song|10
// 2024-01-18 Grimmier			- ADDED Backward Compatability, If you use old ini files with ^ seperators everything parses the old way
//								- if you use a | seperator you parse the new way. This should not break any old config files. 
//								- Ditched the idea of songType and instead went with swapSets 
//								- You define sawp Sets like conditions swap1=
//								- swapSet vaules are itemname|slot pairs and you can combind multiple
//								- When swapping it checks against your last swap and will skip any items you already swapped.
//									- if you do not specify a swapSet in your song string we default to noswap 
//									- the noswap swapSet is defaulted to values for MainHand and OffHand if you set them in the INI.
//									- if MainHand and OffHand are not set we will just cast the song with whatever is in our hands at the time.
//									- The logic for this approach is song orders can change based on timers. 
//-----------------
// MQ2Twist contributers:
//    koad 03-24-04 Original plugin (http://macroquest.sourceforge.net/phpBB2/viewtopic.php?t=5962&start=2)
//    CyberTech 03-31-04 w/ code/ideas from Falco72 & Space-boy
//    Cr4zyb4rd 08-19-04 taking over janitorial duties
//    Pheph 08-24-04 cleaning up use of MQ2Data
//    Simkin 12-17-07 Updated for Secrets of Faydwer 10 Songs
//    MinosDis 05-01-08 Updated to fix /twist once
//    dewey2461 09-19-09 Updated to let you twist AA abilities and define clicky/AA songs from inside the game
//    htw 09-22-09+ See changes below
//    gSe7eN 04-03-12 Fixed Show to dShow for March 31st build

/*
MQ2Medley

Usage:
/medley name - Sing the given medley
/medley queue "song/item/aa name" [-targetid|spawnid] [-interrupt] - add songs to queue to cast once
/medley stop/end/off - stop singing
/medley - Resume the medley after using /medley stop
/medley delay # - 10ths of a second, minimum of 0, default 3, how long after casting a spell to wait to cast next spell
/medley reload - reload the INI file
/medley quiet - Toggles songs listing for medley and queued songs
/medley bandolier - Toggles between using MQ2Exchange or In-Game Bandolier

----------------------------
Item Click Method:
MQ2Medley uses /cast item "ItemName"

----------------------------
Examples:
/medley melee
play medley defined in [MQ2Medley-melee] ini setion
/medley queue "Dirge of the Sleepwalker" -interrupt
Interrupt current song and cast AA "Dirge of the Sleepwaler"
/medley queue "Slumber of Silisia" -targetid|${Me.XTarget[2].ID}
When current song ends, will mez XTarget[2] and switch back current target.
Target will be switched for one pulse, which is typicaly less than 20ms
/medley queue "Blade of Vesagran"
Add epic click to queue
/medley queue "Lesson of the Devoted"
Lesson of the Devoted AA will be added to the twist queue and sung when current song finished
/medley bandolier
Toggles between Bandolier swapping and MQ2Exchange swapping.

----------------------------
MQ2Data TLO Variables:
Members:
Medley.Medley
- string of current medley
- false (boolean) if no current medley
Medley.TTQE (time to queue empty)
- double time in seconds until queue is empty, this is estimate only.  If performating normal medley, this will be 0.0
Medley.Tune
- int 0 Always 0 since changed to "A Tune Stuck in My Head" AA
Medley.Active
- boolean true if MQ2Medley is currently trying to cast spells
----------------------------

The ini file has the format:
[MQ2Medley]
Delay=3       Delay between twists in 1/10th of second. Lag & System dependant.
[MQ2Medley-medleyname]   can multiple one of these sections, for each medley you define
songIF=Condition to turn entire block on/off
song1=Name of Song/Item/AA^expression representing duration of song^condition expression for this song to be song
bandolier1=bandolier name from in game
...
song20=

*/

#include <mq/Plugin.h>

PreSetup("MQ2Medley");
PLUGIN_VERSION(1.08);

#define PLUGIN_MSG "\arMQMedley\au:: "

constexpr int MAX_MEDLEY_SIZE = 30;

class SongData
{
private:
	uint32_t castTimeMs;        // ms
public:
	enum SpellType {
		SONG = 1,
		ITEM = 2,
		AA = 3,
		NOT_FOUND = 4
	};

	std::string name;
	SpellType type;
	bool once;                  // is this a cast once spell?
	bool isDot;                 // is dot, if so track time by spawn ID

	unsigned int targetID;      // SpawnID
	std::string durationExp;    // duration in seconds, how long the spell lasts, evaluated with Math.Calc
	std::string conditionalExp; // condition to cast this song under, evaluated with Math.Calc
	std::string bandolier; 		// bandloier name for weapon swap.
	std::string targetExp;      // expression for targetID
	std::string swapSet;		// Song Type ie. sing,precussion,wind,brass,string
public:
	SongData(std::string spellName, SpellType spellType, uint32_t spellCastTimeMs);

	bool isReady();  // true if spell/item/aa is ready to cast (no timer)
	uint32_t getCastTimeMs() const;
	double evalDuration();
	bool evalCondition();
	DWORD evalTarget();
};
// Globals
//weapon swapping 
std::map<std::string, std::vector<std::pair<std::string, std::string>>> swapSettings;
std::string MainHand = ""; // main hand weapon
std::string OffHand = ""; // Offhand Weapon
bool UseBandolier = false;
std::string noswap = "";
std::map<std::string, std::string> conditions; // Conditions map for lookup when building commands.
std::map<std::string, std::string> lastSwapSet;	// SongType of last Song Played used to check for swapping
// Song Data
const SongData nullSong = SongData("", SongData::NOT_FOUND, 0);

bool MQ2MedleyEnabled = false;
uint32_t castPadTimeMs = 300;               // ms to give spell time to finish
std::list<SongData> medley;                // medley[n] = stores medley list
std::string medleyName;
std::map<std::string, uint64_t > songExpires;   // when cast, songExpires["songName"] = epoch(ms) + SongDurationMs
std::map<unsigned int, std::map<std::string, uint64_t >> songExpiresMob; // for per mob tracking

// song to song state variables
SongData currentSong = nullSong;
boolean bWasInterrupted = false;
uint64_t CastDue = 0;
PSPAWNINFO TargetSave = nullptr;

bool bTwist = false;
// Plugin States
bool quiet = false;
bool DebugMode = false;
bool Initialized = false;
char SongIF[MAX_STRING] = "";

void resetTwistData()
{
	medley.clear();
	medleyName = "";

	currentSong = nullSong;
	bWasInterrupted = false;

	bTwist = false;
	SongIF[0] = 0;
	WritePrivateProfileString("MQ2Medley", "Playing", "0", INIFileName);
	WritePrivateProfileString("MQ2Medley", "Medley", "", INIFileName);
}

// returns time in seconds till quest is empty. millisecond precision
double getTimeTillQueueEmpty()
{
	double time = 0.0;
	boolean isOnceQueued = false;

	for (auto song = medley.begin(); song != medley.end(); song++) {
		if (song->once) {
			isOnceQueued = true;
			time += castPadTimeMs;
			time += song->getCastTimeMs();
		}
	}

	if (currentSong.once || isOnceQueued) {
		// FIXME: Narrowing implicit conversion
		time += CastDue - MQGetTickCount64();
	}

	return time;
}

// FIXME:  This is likely not needed
void Evaluate(char* zOutput, char* zFormat, ...) {
	//char zOutput[MAX_STRING]={0};
	va_list vaList;
	va_start(vaList, zFormat);
	vsprintf_s(zOutput, MAX_STRING, zFormat, vaList);
	//DebugSpewAlways("E[%s]",zOutput);
	ParseMacroData(zOutput, MAX_STRING);
	//DebugSpewAlways("R[%s]",zOutput);
	//WriteChatf("::: zOutput = [ %s ]",zOutput);
}


// -1 if not found
// cast time in ms if found
int GetItemCastTime(const std::string& ItemName)
{
	char zOutput[MAX_STRING] = { 0 };
	sprintf_s(zOutput, "${FindItem[=%s].CastTime}", ItemName.c_str());
	ParseMacroData(zOutput, MAX_STRING);
	DebugSpew("MQ2Medley::GetItemCastTime ${FindItem[=%s].CastTime} returned=%s", ItemName.c_str(), zOutput);
	return GetIntFromString(zOutput, -1);
}

// -1 if not found
// cast time in ms if found
int GetAACastTime(const std::string& AAName)
{
	char zOutput[MAX_STRING] = { 0 };
	sprintf_s(zOutput, "${Me.AltAbility[%s].Spell.CastTime}", AAName.c_str());
	ParseMacroData(zOutput, MAX_STRING);
	DebugSpew("MQ2Medley::GetAACastTime ${Me.AltAbility[%s].Spell.CastTime} returned=%s", AAName.c_str(), zOutput);
	return GetIntFromString(zOutput, -1);
}

void MQ2MedleyDoCommand(PSPAWNINFO pChar, PCHAR szLine)
{
	DebugSpew("MQ2Medley::MQ2MedleyDoCommand(pChar, %s)", szLine);
	HideDoCommand(pChar, szLine, FromPlugin);
}

int GemCastTime(const std::string& spellName)
{
	ItemPtr n;
	// Gem 1 to NUM_SPELL_GEMS
	for (int i = 0; i < NUM_SPELL_GEMS; i++)
	{
		// TODO: This logic could be further refined.
		PSPELL pSpell = GetSpellByID(GetPcProfile()->MemorizedSpells[i]);
		if (pSpell && starts_with(pSpell->Name, spellName)) {
			const float mct = static_cast<float>(GetCastingTimeModifier(pSpell) + GetFocusCastingTimeModifier(pSpell, n, false) + pSpell->CastTime);
			if (mct < 0.50f * static_cast<float>(pSpell->CastTime))
				return static_cast<int>(0.50 * (pSpell->CastTime));

			return static_cast<int>(mct);
		}
	}

	return -1;
}

/**
* Get the current casting spell and store in szCurrentCastingSpell, if failed to udpate this will return false
*/
//bool getCurrentCastingSpell()
//{
//	DebugSpew("MQ2Medley::getCurrentCastingSpell -ENTER ");
//	PCSIDLWND pCastingWindow = (PCSIDLWND)pCastingWnd;
//	if (!pCastingWindow->IsVisible())
//	{
//		DebugSpew("MQ2Medley::getCurrentCastingSpell - not active", szCurrentCastingSpell);
//		return false;
//	}
//	if (CXWnd *Child = ((CXWnd*)pCastingWnd)->GetChildItem("Casting_SpellName")) {
//		if (GetCXStr(Child->WindowText, szCurrentCastingSpell, 2047) && szCurrentCastingSpell[0] != '\0') {
//			DebugSpew("MQ2Medley::getCurrentCastingSpell -- current cast is: ", szCurrentCastingSpell);
//			return true;
//		}
//	}
//	return false;
//}

SongData getSongData(const char* name)
{
	std::string spellName = name;  // gem spell, item, or AA

	// if spell name is a # convert to name for that gem
	const int spellNum = GetIntFromString(name, 0);
	if (spellNum > 0 && spellNum <= NUM_SPELL_GEMS) {
		DebugSpew("MQ2Medley::TwistCommand Parsing gem %d", spellNum);
		PSPELL pSpell = GetSpellByID(GetPcProfile()->MemorizedSpells[spellNum - 1]);
		if (pSpell) {
			spellName = pSpell->Name;
		}
		else {
			WriteChatf(PLUGIN_MSG "\arInvalid spell number specified (\ay%s\ar) - ignoring.", name);
			return nullSong;
		}
	}

	int castTime = GemCastTime(spellName);
	if (castTime >= 0)
	{
		if (castTime == 0) {
			// race condition after casting instant spell (Coalition), sometimes causing next song to be skipped
			castTime = 100;
		}
		return SongData(spellName, SongData::SONG, castTime);
	}

	castTime = GetItemCastTime(spellName);
	if (castTime >= 0)
	{
		return SongData(spellName, SongData::ITEM, castTime);
	}

	castTime = GetAACastTime(spellName);
	if (castTime >= 0)
	{
		return SongData(spellName, SongData::AA, castTime);
	}

	return nullSong;
}

const uint64_t getSongExpires(const SongData& song) {
	if (song.isDot && pTarget) {
		if (pTarget->SpawnID) {
			if (songExpiresMob[pTarget->SpawnID].count(song.name)) {
				return songExpiresMob[pTarget->SpawnID][song.name];
			}
			else {
				return MQGetTickCount64();
			}
		}
		else {
			return MQGetTickCount64();
		}
	}
	else {
		if (songExpires.count(song.name)) {
			return songExpires[song.name];
		}
		else {
			return MQGetTickCount64();
		}
	}
}

void setSongExpires(const SongData& song, uint64_t expires) {
	if (song.isDot) {
		if (pTarget && pTarget->SpawnID) {
			songExpiresMob[pTarget->SpawnID][song.name] = expires;
		}
		else {
			// TODO: This shouldn't happen
		}
	}
	else {
		songExpires[song.name] = expires;
	}
}

// returns time it will take to cast (ms)
// preconditions:
//   SongTodo is ready to cast
// -1 - cast failed
int32_t doCast(const SongData& SongTodo)
{
	DebugSpew("MQ2Medley::doCast(%s) ENTER", SongTodo.name.c_str());
	//WriteChatf("MQ2Medley::doCast(%s) ENTER", SongTodo.name.c_str());
	char szTemp[MAX_STRING] = { 0 };
	if (GetCharInfo())
	{
		if (GetCharInfo()->pSpawn)
		{
			switch (SongTodo.type) {
			case SongData::SONG:
				for (int i = 0; i < NUM_SPELL_GEMS; i++)
				{
					PSPELL pSpell = GetSpellByID(GetPcProfile()->MemorizedSpells[i]);
					if (pSpell && (pSpell->Name == SongTodo.name)) {
						int gemNum = i + 1;

						if (!SongTodo.targetID) {
							// do nothing special
						}
						else if (PSPAWNINFO Target = (PSPAWNINFO)GetSpawnByID(SongTodo.targetID)) {
							TargetSave = pTarget;
							pTarget = Target;
							DebugSpew("MQ2Medley::doCast - Set target to %d", Target->SpawnID);
						}
						else {
							WriteChatf("MQ2Medley::doCast - cannot find targetID=%d for to cast \"%s\", SKIPPING", SongTodo.targetID, SongTodo.name.c_str());
							return -1;
						}

						if (UseBandolier == 0) { // Use /exchange instead of bandolier
							std::string command = "/multiline ; /stopsong ; ";

							if (SongTodo.swapSet == "noswap") {
								// Handle noswap case
								bool swapNeeded = false;
								if (!MainHand.empty() && lastSwapSet["mainhand"] != MainHand) {
									command += "/exchange \"" + MainHand + "\" mainhand; ";
									lastSwapSet["mainhand"] = MainHand;
									swapNeeded = true;
								}
								if (!OffHand.empty() && lastSwapSet["offhand"] != OffHand) {
									command += "/exchange \"" + OffHand + "\" offhand; ";
									lastSwapSet["offhand"] = OffHand;
									swapNeeded = true;
								}

								if (!swapNeeded) {
									// If no swap needed, just cast the song
									command = "/multiline ; /stopsong ; ";
								}
							}
							else {
								// Handle other swapSets
								for (const auto& [item, slot] : swapSettings[SongTodo.swapSet]) {
									if (lastSwapSet[slot] != item) {
										command += "/exchange \"" + item + "\" " + slot + " ; ";
										lastSwapSet[slot] = item; // Update lastSwapSet with the new item
									}
									else {
										// Debugging output when skipping an item-slot pair
										if (DebugMode) {
											WriteChatf("DEBUG: Skipping swap for %s in %s slot, already equipped.", item.c_str(), slot.c_str());
										}
									}
								}
							}

							command += "/cast " + std::to_string(gemNum);
							strcpy_s(szTemp, MAX_STRING, command.c_str());
						}
						else { // Use bandolier instead

							if (SongTodo.bandolier.c_str() == "1") {

								sprintf(szTemp, "/multiline ; /stopsong ; /cast %d", gemNum);
							}
							else {
								sprintf(szTemp, "/multiline ; /stopsong ; /bandolier activate %s ; /cast %d", SongTodo.bandolier.c_str(), gemNum);
								DebugSpew("MQ2Medley::doCast - /multiline ; /stopsong ; /bandolier activate %s ; /cast %d", SongTodo.bandolier.c_str(), gemNum);
							}
						}
						if (DebugMode) {
							WriteChatf("MQ2Medley::doCast -  %s", szTemp);
						}
						DebugSpew("MQ2Medley::doCast -  %s", szTemp);
						MQ2MedleyDoCommand(GetCharInfo()->pSpawn, szTemp);
						// FIXME: Narrowing conversion
						return SongTodo.getCastTimeMs();
					}
				}
				WriteChatf("MQ2Medley::doCast - could not find \"%s\" to cast, SKIPPING", SongTodo.name.c_str());

				return -1;
			case SongData::ITEM:
				DebugSpew("MQ2Medley::doCast - Next Song (Casting Item  \"%s\")", SongTodo.name.c_str());
				sprintf_s(szTemp, "/multiline ; /stopsong ; /useitem \"%s\"", SongTodo.name.c_str());
				MQ2MedleyDoCommand(GetCharInfo()->pSpawn, szTemp);
				// FIXME: Narrowing conversion
				return SongTodo.getCastTimeMs();
			case SongData::AA:
				DebugSpew("MQ2Medley::doCast - Next Song (Casting AA  \"%s\")", SongTodo.name.c_str());
				sprintf_s(szTemp, "/multiline ; /stopsong ; /alt act ${Me.AltAbility[%s].ID}", SongTodo.name.c_str());
				MQ2MedleyDoCommand(GetCharInfo()->pSpawn, szTemp);
				// FIXME: Narrowing conversion
				return SongTodo.getCastTimeMs();
			default:
				// This is the null song - do nothing.
				WriteChatf("MQ2Medley::doCast - unsupported type %d for \"%s\", SKIPPING", SongTodo.type, SongTodo.name.c_str());
				return -1; // todo
			}
		}
	}
	return -1;
}

void Update_INIFileName(PCHARINFO pCharInfo) {
	sprintf_s(INIFileName, "%s\\%s_%s.ini", gPathConfig, GetServerShortName(), pCharInfo->Name);
}

void Load_MQ2Medley_INI_Medley(PCHARINFO pCharInfo, const std::string& medleyNameIni);

void Load_MQ2Medley_INI(PCHARINFO pCharInfo)
{
	char szTemp[MAX_STRING] = { 0 };

	Update_INIFileName(pCharInfo);

	castPadTimeMs = GetPrivateProfileInt("MQ2Medley", "Delay", 3, INIFileName) * 100;
	// FIXME: Narrowing conversion
	WritePrivateProfileInt("MQ2Medley", "Delay", castPadTimeMs / 100, INIFileName);
	quiet = GetPrivateProfileInt("MQ2Medley", "Quiet", 0, INIFileName) ? 1 : 0;
	WritePrivateProfileInt("MQ2Medley", "Quiet", quiet, INIFileName);
	DebugMode = GetPrivateProfileInt("MQ2Medley", "Debug", 0, INIFileName) ? 1 : 0;
	WritePrivateProfileInt("MQ2Medley", "Debug", DebugMode, INIFileName);
	UseBandolier = GetPrivateProfileInt("MQ2Medley", "Bandolier", 0, INIFileName) ? 1 : 0;
	WritePrivateProfileInt("MQ2Medley", "Bandolier", UseBandolier, INIFileName);
	GetPrivateProfileString("MQ2Medley", "MainHand", "", szTemp, MAX_STRING, INIFileName);
	MainHand = szTemp;
	GetPrivateProfileString("MQ2Medley", "OffHand", "", szTemp, MAX_STRING, INIFileName);
	OffHand = szTemp;
	GetPrivateProfileString("MQ2Medley", "Medley", "", szTemp, MAX_STRING, INIFileName);
	if (szTemp[0] != 0)
	{
		Load_MQ2Medley_INI_Medley(pCharInfo, szTemp);
		bTwist = GetPrivateProfileInt("MQ2Medley", "Playing", 1, INIFileName) ? 1 : 0;
	}
}

bool isKnownSwapSet(const std::string& type) {
	// Check if the type starts with "swap" and is followed by a number
	if (type.rfind("swap", 0) == 0) {
		// Additional check to ensure there's a number after "swap"
		std::string numberPart = type.substr(4); // Extract the part after "swap"
		return !numberPart.empty() && std::all_of(numberPart.begin(), numberPart.end(), ::isdigit);
	}
	return false;
}

void parseOld(const char* szTemp, SongData& medleySong) {
	char* pNext;
	char* p = strtok_s(const_cast<char*>(szTemp), "^", &pNext);
	if (p) {
		medleySong = getSongData(p);
		if (medleySong.type == SongData::NOT_FOUND) {
			// Handle error...
			return;
		}
		p = strtok_s(nullptr, "^", &pNext);
		if (p) medleySong.durationExp = p;
		p = strtok_s(nullptr, "^", &pNext);
		if (p) medleySong.conditionalExp = p;
		p = strtok_s(nullptr, "^", &pNext);
		if (p) medleySong.targetExp = p;
	}
}

void parseNew(const char* szTemp, SongData& medleySong, const std::map<std::string, std::string>& conditions) {
	char* pNext;
	char* p = strtok_s(const_cast<char*>(szTemp), "|", &pNext);
	if (p) {
		medleySong = getSongData(p);
		if (medleySong.type == SongData::NOT_FOUND) {
			// Handle error...
			return;
		}
		p = strtok_s(nullptr, "|", &pNext);
		if (p) medleySong.durationExp = p;
		p = strtok_s(nullptr, "|", &pNext);
		if (p) {
			if (isKnownSwapSet(p)) {
				medleySong.swapSet = p;
				p = strtok_s(nullptr, "|", &pNext);
			} else {
				medleySong.swapSet = "noswap";
			}
		}
		if (p) {
			auto it = conditions.find(p);
			if (it != conditions.end()) {
				medleySong.conditionalExp = it->second;
			} else {
				medleySong.conditionalExp = p; // Use the key as a direct condition
			}
			p = strtok_s(nullptr, "|", &pNext);
		}
		if (p) medleySong.targetExp = p;
	}
}

void Load_MQ2Medley_INI_Medley(PCHARINFO pCharInfo, const std::string& medleyNameIni)
{
	char szTemp[MAX_STRING] = { 0 };
	char *pNext;
	swapSettings.clear();
	medley.clear();
	conditions.clear();
	Update_INIFileName(pCharInfo);
	std::string iniSection = "MQ2Medley-" + medleyNameIni;
	WriteChatf("DEBUG::IniSection %s", iniSection.c_str());

	// Parse swap settings
	int swapIndex = 1; // Start with swap1
	while (true) {
		std::string swapKey = "swap" + std::to_string(swapIndex);
		//if (DebugMode) { WriteChatf("DEBUG::Attempting to read: %s", swapKey.c_str()); }
		if (GetPrivateProfileString(iniSection.c_str(), swapKey.c_str(), "", szTemp, MAX_STRING, INIFileName)) {
			if (DebugMode) { WriteChatf("DEBUG::Read %s: %s", swapKey.c_str(), szTemp); }
			char *pItem = strtok_s(szTemp, "|", &pNext);
			while (pItem) {
				char *pSlot = strtok_s(nullptr, "|", &pNext);
				if (pItem && pSlot) {
					swapSettings[swapKey].emplace_back(pItem, pSlot);
				}
				pItem = strtok_s(nullptr, "|", &pNext); // Move to next pair
			}
			swapIndex++; // Move to the next swap
		}
		else {
			if (DebugMode) { WriteChatf("DEBUG::No entry found for %s", swapKey.c_str()); }
			break; // Exit the loop if no more swap settings are found
		}
	}
	if (DebugMode) { WriteChatf("DEBUG::Total Swap Count: %d", swapIndex - 1); }


	// Parse conditions dynamically
	int condIndex = 1; // Start with cond1
	while (true) {
		std::string conditionKey = "cond" + std::to_string(condIndex);
		if (GetPrivateProfileString(iniSection.c_str(), conditionKey.c_str(), "", szTemp, MAX_STRING, INIFileName)) {
			if (DebugMode) { WriteChatf("DEBUG::Read %s: %s", conditionKey.c_str(), szTemp); }
			conditions[conditionKey] = szTemp;
			condIndex++; // Move to the next condition
		}
		else {
			break; // Exit the loop if no more condition settings are found
		}
	}
	if (DebugMode) { WriteChatf("DEBUG::Total Condition Count: %d", condIndex - 1); }

	// Parse songs
	for (int i = 0; i < MAX_MEDLEY_SIZE; i++) {
		std::string songKey = "song" + std::to_string(i + 1);
		if (GetPrivateProfileString(iniSection.c_str(), songKey.c_str(), "", szTemp, MAX_STRING, INIFileName)) {
			// Initialize medleySong with default values
			SongData medleySong("", SongData::NOT_FOUND, 0);

			char separator = strchr(szTemp, '|') ? '|' : '^';
			if (separator == '^') {
				parseOld(szTemp, medleySong);
			} else {
				parseNew(szTemp, medleySong, conditions);
			}

			// Read bandolier information
			std::string bandolierKey = "bandolier" + std::to_string(i + 1);
			if (GetPrivateProfileString(iniSection.c_str(), bandolierKey.c_str(), "", szTemp, MAX_STRING, INIFileName)) {
				medleySong.bandolier = szTemp;
			} else {
				medleySong.bandolier = "1";
			}

			if (DebugMode) {
				WriteChatf("Debug: Processing song - Name: %s, Duration: %s, SwapSet: %s, Condition: %s, Bandolier: %s, Target: %s", 
					medleySong.name.c_str(), medleySong.durationExp.c_str(), medleySong.swapSet.c_str(), 
					medleySong.conditionalExp.c_str(), medleySong.bandolier.c_str(), medleySong.targetExp.c_str());
			}

			if (medleySong.type != SongData::NOT_FOUND) {
				medley.emplace_back(medleySong);
			}
		}
	}
	WriteChatf("MQ2Medley::loadMedley - [%s] %d song Medley loaded", medleyNameIni.c_str(), static_cast<int>(medley.size()));
	GetPrivateProfileString(iniSection.c_str(), "SongIF", "", SongIF, MAX_STRING, INIFileName);
	conditions.clear();
}

void StopTwistCommand(PSPAWNINFO pChar, PCHAR szLine)
{
	char szTemp[MAX_STRING] = { 0 };
	GetArg(szTemp, szLine, 1);
	bTwist = false;
	currentSong = nullSong;
	MQ2MedleyDoCommand(pChar, "/stopsong");
	if (_strnicmp(szTemp, "silent", 6))
		WriteChatf(PLUGIN_MSG "\atStopping Medley");
	WritePrivateProfileInt("MQ2Medley", "Playing", bTwist, INIFileName);
}

void DisplayMedleyHelp() {
	WriteChatf("\arMQ2Medley \au- \atSong Scheduler - read documentation online");
}

// **************************************************  *************************
// Function:      MedleyCommand
// Description:   Our /medley command. schedule songs to sing
// **************************************************  *************************
void MedleyCommand(PSPAWNINFO pChar, PCHAR szLine)
{
	char szTemp[MAX_STRING] = { 0 }, szTemp1[MAX_STRING] = { 0 };

	int argNum = 1;
	GetArg(szTemp, szLine, argNum);

	if ((!medley.empty() && (!strlen(szTemp)) || !_strnicmp(szTemp, "start", 5))) {
		GetArg(szTemp1, szLine, 2);
		if (_strnicmp(szTemp1, "silent", 6))
			WriteChatf(PLUGIN_MSG "\atStarting Twist.");
		bTwist = true;
		CastDue = 0;
		WritePrivateProfileInt("MQ2Medley", "Playing", bTwist, INIFileName);
		return;
	}

	if (!_strnicmp(szTemp, "debug", 5)) {
		DebugMode = !DebugMode;
		WriteChatf(PLUGIN_MSG "\atDebug mode is now %s\ax.", DebugMode ? "\ayON" : "\agOFF");
		WritePrivateProfileInt("MQ2Medley", "Debug", DebugMode, INIFileName);
		return;
	}

	if (!_strnicmp(szTemp, "bandolier", 9)) {
		UseBandolier = !UseBandolier;
		WriteChatf(PLUGIN_MSG "\atBandolier mode is now %s\ax.", UseBandolier ? "\ayON" : "\agOFF");
		WritePrivateProfileInt("MQ2Medley", "Bandolier", UseBandolier, INIFileName);
		return;
	}

	if (!_strnicmp(szTemp, "stop", 4) || !_strnicmp(szTemp, "end", 3) || !_strnicmp(szTemp, "off", 3)) {
		GetArg(szTemp1, szLine, 2);
		if (_strnicmp(szTemp1, "silent", 6))
			StopTwistCommand(pChar, szTemp);
		else
			StopTwistCommand(pChar, szTemp1);
		return;
	}

	if (!_strnicmp(szTemp, "reload", 6) || !_strnicmp(szTemp, "load", 4)) {
		WriteChatf(PLUGIN_MSG "\atReloading INI Values.");
		Load_MQ2Medley_INI(GetCharInfo());
		return;
	}

	if (!_strnicmp(szTemp, "delay", 5)) {
		GetArg(szTemp, szLine, 2);
		if (strlen(szTemp)) {
			int delay = GetIntFromString(szTemp, 0);
			if (delay < 0)
			{
				WriteChatf(PLUGIN_MSG "\ayWARNING: \arDelay cannot be less than 0, setting to 0");
				delay = 0;
			}
			castPadTimeMs = delay * 100;
			Update_INIFileName(GetCharInfo());
			WritePrivateProfileInt("MQ2Medley", "Delay", delay, INIFileName);
			WriteChatf(PLUGIN_MSG "\atSet delay to \ag%d\at, INI updated.", delay);
		}
		else
			WriteChatf(PLUGIN_MSG "\atDelay \ag%d\at.", castPadTimeMs / 100);
		return;
	}

	if (!_strnicmp(szTemp, "quiet", 5)) {
		quiet = !quiet;
		WritePrivateProfileInt("MQ2Medley", "Quiet", quiet, INIFileName);
		WriteChatf(PLUGIN_MSG "\atNow being %s\at.", quiet ? "\ayquiet" : "\agnoisy");
		return;
	}

	if (!_strnicmp(szTemp, "clear", 5)) {
		resetTwistData();
		StopTwistCommand(pChar, szTemp);
		if (!quiet)
			WriteChatf(PLUGIN_MSG "\ayMedley Cleared.");
		return;
	}

	// check help arg, or display if we have no songs defined and /twist was used
	if (!strlen(szTemp) || !_strnicmp(szTemp, "help", 4)) {
		DisplayMedleyHelp();
		return;
	}

	//boolean isQueue = false;
	//boolean isInterrupt = true;

	if (!_strnicmp(szTemp, "queue", 4) || !_strnicmp(szTemp, "once", 4)) {
		WriteChatf(PLUGIN_MSG "\ayAdding to once queue");
		argNum++;

		GetArg(szTemp, szLine, argNum++);
		if (!strlen(szTemp)) {
			WriteChatf(PLUGIN_MSG "\atqueue requires spell/item/aa to cast", szTemp);
			return;
		}
		SongData songData = getSongData(szTemp);
		if (songData.type == SongData::NOT_FOUND) {
			WriteChatf(PLUGIN_MSG "\atUnable to find spell for \"%s\", skipping", szTemp);
			return;
		}

		do {
			GetArg(szTemp, szLine, argNum++);
			if (szTemp[0] == 0) {
				break;
			}
			else if (!_strnicmp(szTemp, "-targetid|", 10)) {
				songData.targetID = GetIntFromString(&szTemp[10], 0);
				DebugSpew("MQ2Medley::TwistCommand  - queue \"%s\" targetid=%d", songData.name.c_str(), songData.targetID);
			}
			else if (!_strnicmp(szTemp, "-interrupt", 10)) {
				currentSong = nullSong;
				CastDue = 0;
				MQ2MedleyDoCommand(pChar, "/stopsong");
			}

		} while (true);
		songData.once = true;

		DebugSpew("MQ2Medley::TwistCommand  - altQueue.push_back(%s);", songData.name.c_str());
		//altQueue.push_back(songData);
		medley.push_front(songData);
		return;
	}

	//if (!_strnicmp(szTemp, "improv", 6)) {
	//	argNum++;
	//	WriteChatf(PLUGIN_MSG "\atImprov Medley.");
	//	medley.clear();
	//	medleyName = "improv";

	//	while (true) {
	//		GetArg(szTemp, szLine, argNum);
	//		argNum++;
	//		if (!strlen(szTemp))
	//			break;

	//		DebugSpew("MQ2Medley::TwistCommand Parsing song %s", szTemp);

	//		SongData songData = getSongData(szTemp);
	//		if (songData.type == SongData::NOT_FOUND) {
	//			WriteChatf(PLUGIN_MSG "\atUnable to find spell for \"%s\", skipping", szTemp);
	//			continue;
	//		}

	//		DebugSpew("MQ2Medley::TwistCommand  - medley.emplace_back.(%s);", songData.name.c_str());
	//		medley.emplace_back(songData);

	//	}

	//	if (!quiet)
	//		WriteChatf(PLUGIN_MSG "\atTwisting \ag%d \atsong%s.", medley.size(), medley.size()>1 ? "s" : "");

	//	if (medley.size()>0)
	//		bTwist = true;
	//	if (isInterrupt)
	//	{
	//		currentSong = nullSong;
	//		CastDue = 0;
	//		MQ2MedleyDoCommand(pChar, "/stopsong");
	//	}

	//}

	if (strlen(szTemp)) {
		WriteChatf(PLUGIN_MSG "\atLoading medley \"%s\"", szTemp);
		medleyName = szTemp;
		WritePrivateProfileString("MQ2Medley", "Medley", szTemp, INIFileName);
		Load_MQ2Medley_INI_Medley(GetCharInfo(), medleyName);
		bTwist = true;
		WritePrivateProfileInt("MQ2Medley", "Playing", bTwist, INIFileName);
		return;
	}
	else if (!medley.empty()) {
		WriteChatf(PLUGIN_MSG "\atResuming medley \"%s\"", medleyName.c_str());
		bTwist = true;
		WritePrivateProfileInt("MQ2Medley", "Playing", bTwist, INIFileName);
	}
	else {
		WriteChatf(PLUGIN_MSG "\atNo medley defined");
	}
}

/*
Checks to see if character is in a fit state to cast next song/item

Note 1: Do not try to correct SIT state, or you will have to stop the
twist before re-memming songs

Note 2: Since the auto-stand-on-cast bullcrap added to EQ a few patches ago,
chars would stand up every time it tried to twist a medley.  So now
we stop twisting at sit.
*/
bool CheckCharState()
{
	if (!bTwist)
		return false;

	if (GetCharInfo()) {
		if (!GetCharInfo()->pSpawn)
			return false;
		if (GetCharInfo()->Stunned == 1)
			return false;
		switch (GetCharInfo()->standstate) {
		case STANDSTATE_SIT:
			//WriteChatf(PLUGIN_MSG "\ayStopping Twist.");
			//bTwist = false;
			return false;
		case STANDSTATE_FEIGN:
			MQ2MedleyDoCommand(GetCharInfo()->pSpawn, "/stand");
			return false;
		case STANDSTATE_DEAD:
			WriteChatf(PLUGIN_MSG "\ayStopping Twist.");
			//bTwist = false;
			return false;
		default:
			break;
		}
		if (InHoverState()) {
			//bTwist = false;
			return false;
		}
		if (GetSelfBuff([](EQ_Spell* pSpell) { return HasSPA(pSpell, SPA_SILENCE); }) >= 0) {
			return false;
		}
		if (GetSelfBuff([](EQ_Spell* pSpell) { return HasSPA(pSpell, SPA_INVULNERABILITY); }) >= 0) {
			return false;
		}

	}

	return true;
}

class MQ2MedleyType* pMedleyType = 0;

class MQ2MedleyType : public MQ2Type
{
private:
	char szTemp[MAX_STRING] = { 0 };
public:
	enum MedleyMembers {
		Medley = 1,
		TTQE = 2,
		Tune = 3,
		Active
	};

	MQ2MedleyType() :MQ2Type("Medley") {
		TypeMember(Medley);
		TypeMember(TTQE);
		TypeMember(Tune);
		TypeMember(Active);
	}

	virtual bool GetMember(MQVarPtr VarPtr, const char* Member, char* Index, MQTypeVar& Dest) override {
		MQTypeMember* pMember = MQ2MedleyType::FindMember(Member);
		if (!pMember)
			return false;
		switch (pMember->ID) {
		case Medley:
			/* Returns: string
			current medley name
			empty string if no current medly
			*/
			sprintf_s(szTemp, "%s", medleyName.c_str());
			//medleyName.copy(szTemp, MAX_STRING);
			Dest.Ptr = szTemp;
			Dest.Type = mq::datatypes::pStringType;
			return true;
		case TTQE:
			/* Returns: double
			0 - if nothing is queued and performing normal medley
			#.# - double estimate time till queue is completed
			*/
			Dest.Double = getTimeTillQueueEmpty();
			Dest.Type = mq::datatypes::pDoubleType;
			return true;
		case Tune:
			/* Returns: int
			0 - (deprecated) always 0.  Because Tune Stuck in head was changed to passive AA, should no longer be used
			*/
			Dest.Int = 0;
			Dest.Type = mq::datatypes::pIntType;
			return true;
		case Active:
			/* Returns: boolean
			true - medley is active
			*/
			Dest.Int = bTwist;
			Dest.Type = mq::datatypes::pBoolType;
			return true;
		default:
			break;
		}
		return false;
	}

	bool ToString(MQVarPtr VarPtr, char* Destination) override
	{
		strcpy_s(Destination, MAX_STRING, bTwist ? "TRUE" : "FALSE");
		return true;
	}
};

bool dataMedley(const char* szIndex, MQTypeVar& Dest)
{
	Dest.DWord = 1;
	Dest.Type = pMedleyType;
	return true;
}

const SongData scheduleNextSong()
{
	uint64_t currentTickMs = MQGetTickCount64();

	if (DebugMode) WriteChatf("MQ2Medley::scheduleNextSong - currentTickMs=%I64u", currentTickMs);
	SongData* stalestSong = nullptr;
	for (auto song = medley.begin(); song != medley.end(); song++)
	{
		if (!song->isReady()) {
			DebugSpew("MQ2Medley::scheduleNextSong skipping[%s] (not ready)", song->name.c_str());
			continue;
		}
		if (!song->evalCondition()) {
			DebugSpew("MQ2Medley::scheduleNextSong skipping[%s] (condition not met)", song->name.c_str());
			continue;
		}

		if (song->once) {
			SongData nextSong = *song;
			medley.erase(song);
			return nextSong;
		}

		if (!stalestSong)
			stalestSong = &(*song);

		// for a 3s casting time song, we should recast if it will expire in the next 6 seconds
		// the constant 3 seconds is we will assume if we don't cast this song now, the next song will probably be a 3
		// second cast time song
		uint64_t startCastByMs = getSongExpires(*song) - song->getCastTimeMs() - 3000;
		if (DebugMode) WriteChatf("MQ2Medley::scheduleNextSong time till need to cast %s: %I64d ms", song->name.c_str(), startCastByMs - currentTickMs);

		// for a 3s casting time song, we should recast if it will expire in the next 6 seconds
		// the constant 3 seconds is we will assume if we don't cast this song now, the next song will probably be a 3
		// second cast time song
		if (startCastByMs < currentTickMs)
			return *song;

		if (getSongExpires(*song) < getSongExpires(*stalestSong))
			stalestSong = &(*song);
	}

	// we didn't find a song that had priority to cast, so we'll cast the song that will expirest instead
	if (stalestSong)
	{
		if (DebugMode) WriteChatf("MQ2Medley::scheduleNextSong no priority song found, returning stalest song: %s", stalestSong->name.c_str());
		return *stalestSong;
	}
	else {
		if (!quiet) WriteChatf(PLUGIN_MSG "\atFAILED to schedule a song, no songs ready or conditions not met");
		return nullSong;
	}
}

// ******************************
// **** MQ2 API Calls Follow ****
// ******************************

PLUGIN_API void InitializePlugin()
{
	DebugSpewAlways("Initializing MQ2Medley");
	AddCommand("/medley", MedleyCommand, 0, 1, 1);
	AddMQ2Data("Medley", dataMedley);
	pMedleyType = new MQ2MedleyType;
	WriteChatf("\atMQ2Medley \agv%1.2f \ax loaded.", MQ2Version);
}

PLUGIN_API void ShutdownPlugin()
{
	DebugSpewAlways("MQ2Medley::Shutting down");
	RemoveCommand("/medley");
	RemoveMQ2Data("Medley");
	swapSettings.clear();
	medley.clear();
	delete pMedleyType;

}

PLUGIN_API void OnPulse()
{
	char szTemp[MAX_STRING] = { 0 };

	//DebugSpew("MQ2Medley::pulse -OnPulse()");
	if (!MQ2MedleyEnabled || !CheckCharState())
		return;

	// if (medley.empty() && altQueue.empty()) return;
	if (medley.empty())
		return;

	if (TargetSave) {
		//DebugSpew("MQ2Medley::pulse -OnPulse() in if(TargetSave)");
		// wait for next song to start casting before switching target back
		//if (getCurrentCastingSpell() && currentSong.name.compare(szCurrentCastingSpell) == 0)
		//{
		DebugSpew("MQ2Medley::pulse - restoring target to SpawnID %d", TargetSave->SpawnID);
		pTarget = TargetSave;
		TargetSave = nullptr;
		//	return;
		//}
		//DebugSpew("MQ2Medley::pulse - state not ready to restore target");
		//return;
	}

	if (pCastingWnd && pCastingWnd->IsVisible()) {
		// Don't try to twist if the casting window is up, it implies the previous song
		// is still casting, or the user is manually casting a song between our twists
		return;
	}

	if (SongIF[0] != 0)
	{
		Evaluate(szTemp, "${If[%s,1,0]}", SongIF);
		// if (DebugMode) WriteChatf(PLUGIN_MSG "\atOnPulse SongIF[%s]=>[%s]=%d", SongIF, szTemp, GetIntFromString(szTemp, 0));
		if (GetIntFromString(szTemp, 0) == 0)
			return;
	}

	// get the next song
	//DebugSpew("MQ2Medley::Pulse (twist) before cast, CurrSong=%d, PrevSong = %d, CastDue -GetTime() = %d", CurrSong, PrevSong, (CastDue-GetTime()));
	if (MQGetTickCount64() > CastDue) {
		DebugSpew("MQ2Medley::Pulse - time for next cast");
		if (bWasInterrupted && currentSong.type != SongData::NOT_FOUND && currentSong.isReady())
		{
			bWasInterrupted = false;
			if (!quiet) WriteChatf("MQ2Medley::OnPulse Spell inturrupted - recast it");
			// current song is unchanged
		}
		else {
			if (bWasInterrupted)
			{
				if (!quiet) WriteChatf("MQ2Medley::OnPulse Spell inturrupted - spell not ready skip it");
				bWasInterrupted = false;
			}
			if (currentSong.type != SongData::NOT_FOUND)
			{
				// successful cast
				if (!currentSong.once)
					setSongExpires(currentSong, MQGetTickCount64() + (uint32_t)(currentSong.evalDuration() * 1000));
			}
			if (!medley.empty())
			{
				currentSong = scheduleNextSong();
				if (currentSong.type == 4) return;
				if (!quiet) WriteChatf(PLUGIN_MSG "\atScheduled: %s", currentSong.name.c_str());
				if (currentSong.targetExp.length() > 0)
					currentSong.targetID = currentSong.evalTarget();
			}
		}

		int32_t castTimeMs = doCast(currentSong);

		if (DebugMode) WriteChatf("MQ2Medley::OnPulse - casting time for %s - %d ms", currentSong.name.c_str(), castTimeMs);
		if (castTimeMs != -1)  // cast failed
		{
			// cast started successfully - update CastDue and PrevSong is now the song we're casting.
			CastDue = MQGetTickCount64() + castTimeMs + castPadTimeMs;
		}
		else {
			DebugSpew("MQ2Medley::OnPulse - cast failed for %s", currentSong.name.c_str());
			currentSong = nullSong;
		}

		DebugSpew("MQ2Medley::OnPulse - exit handling new song: %s", currentSong.name.c_str());
	}
}

//#Event Immune "Your target cannot be mesmerized#*#"

PLUGIN_API bool OnIncomingChat(const char* Line, DWORD Color)
{
	if (!bTwist || !MQ2MedleyEnabled)
		return false;
	// DebugSpew("MQ2Medley::OnIncomingChat(%s)",Line);

	// if (!strcmp(Line, "You haven't recovered yet...")) WriteChatf("MQ2Medley::Have not recovered");

	if ((strstr(Line, "You miss a note, bringing your ") && strstr(Line, " to a close!")) ||
		!strcmp(Line, "You haven't recovered yet...") ||
		(strstr(Line, "Your ") && strstr(Line, " spell is interrupted."))) {
		DebugSpew("MQ2Medley::OnIncomingChat - Song Interrupt Event: %s", Line);
		bWasInterrupted = true;
		CastDue = 0;
	}
	else if (!strcmp(Line, "You can't cast spells while stunned!")) {
		DebugSpew("MQ2Medley::OnIncomingChat - Song Interrupt Event (stun)");
		bWasInterrupted = true;
		// Wait one second before trying again, to avoid spamming the trigger text w/ cast attempts
		CastDue = MQGetTickCount64() + 10;
	}
	return false;
}

PLUGIN_API void OnRemoveSpawn(SPAWNINFO* pSpawn)
{
	songExpiresMob.erase(pSpawn->SpawnID);
}

// Called after entering a new zone
PLUGIN_API void OnZoned()
{
	songExpiresMob.clear();
}

PLUGIN_API void SetGameState(int GameState)
{
	DebugSpew("MQ2Medley::SetGameState()");
	if (GameState == GAMESTATE_INGAME) {
		MQ2MedleyEnabled = true;
		PCHARINFO pCharInfo = GetCharInfo();
		if (!Initialized && pCharInfo) {
			Initialized = true;
			Load_MQ2Medley_INI(pCharInfo);
		}
	}
	else {
		if (GameState == GAMESTATE_CHARSELECT)
			Initialized = false;
		MQ2MedleyEnabled = false;
	}
}

/**
* SongData Impl
*/
SongData::SongData(std::string spellName, SpellType spellType, uint32_t spellCastTime) {
	name = spellName;
	type = spellType;
	castTimeMs = spellCastTime;
	durationExp = "180";    // 3 min default
	targetID = 0;
	bandolier = "1";		// default no swap for bandolier
	swapSet = "noswap";		// default no swap for mq2exchange
	conditionalExp = "1";   // default always sing
	targetExp = "";         // expression for targetID
	once = false;
	isDot = spellName.find("Chant of Flame") != std::string::npos ||
		spellName.find("Chant of Frost") != std::string::npos ||
		spellName.find("Chant of Disease") != std::string::npos ||
		spellName.find("Chant of Poison") != std::string::npos;

	if (DebugMode) WriteChatf("MQ2Medley::SongDate(% s), isDot = % d", spellName.c_str(), isDot);
}

bool SongData::isReady() {
	char zOutput[MAX_STRING] = { 0 };
	switch (type) {
	case SongData::SONG:
		for (int i = 0; i < NUM_SPELL_GEMS; i++)
		{
			PSPELL pSpell = GetSpellByID(GetPcProfile()->MemorizedSpells[i]);
			if (pSpell && starts_with(pSpell->Name, name))
				return GetSpellGemTimer(i) == 0;
		}
		return false;
	case SongData::ITEM:
		sprintf_s(zOutput, "${FindItem[=%s].Timer}", name.c_str());
		ParseMacroData(zOutput, MAX_STRING);
		DebugSpew("MQ2Medley::SongData::IsReady() ${FindItem[=%s].Timer} returned=%s", name.c_str(), zOutput);

		if (!_stricmp(zOutput, "null"))
			return false;
		return GetIntFromString(zOutput, 0) == 0;
	case SongData::AA:
		sprintf_s(zOutput, "${Me.AltAbilityReady[%s]}", name.c_str());
		ParseMacroData(zOutput, MAX_STRING);
		DebugSpew("MQ2Medley::SongData::IsReady() ${Me.AltAbilityReady[%s]} returned=%s", name.c_str(), zOutput);
		return _stricmp(zOutput, "TRUE") == 0;
	default:
		WriteChatf("MQ2Medley::SongData::isReady - unsupported type %d for \"%s\", SKIPPING", type, name.c_str());
		return false; // todo
	}
}

uint32_t SongData::getCastTimeMs() const {
	switch (type) {
	case SongData::SONG:
		return GemCastTime(SongData::name);
	case SongData::ITEM:
		return castTimeMs;
	case SongData::AA:
		return castTimeMs;
	default:
		WriteChatf("MQ2Medley::SongData::getCastTimeMs - unsupported type %d for \"%s\", SKIPPING", type, name.c_str());
		return -1;
	}
}

double SongData::evalDuration() {
	char zOutput[MAX_STRING] = { 0 };
	sprintf_s(zOutput, "${Math.Calc[%s]}", durationExp.c_str());
	ParseMacroData(zOutput, MAX_STRING);
	if (DebugMode) WriteChatf("MQ2Medley::SongData::evalDuration() [%s] returned=%s", conditionalExp.c_str(), zOutput);

	return GetDoubleFromString(zOutput, 0.0);
}

bool SongData::evalCondition() {
	char zOutput[MAX_STRING] = { 0 };
	sprintf_s(zOutput, "${Math.Calc[%s]}", conditionalExp.c_str());
	ParseMacroData(zOutput, MAX_STRING);
	//	if (DebugMode) WriteChatf("MQ2Medley::SongData::evalCondition() [%s] returned=%s", conditionalExp.c_str(), zOutput);
	if (DebugMode) WriteChatf("MQ2Medley::SongData::evalCondition(%s) [%s] returned=%s", name.c_str(), conditionalExp.c_str(), zOutput);

	double result = GetDoubleFromString(zOutput, 0.0);
	return result != 0.0;
}

// FIXME: Does this need to be DWORD?
DWORD SongData::evalTarget() {
	char zOutput[MAX_STRING] = { 0 };
	sprintf_s(zOutput, "${Math.Calc[%s]}", targetExp.c_str());
	ParseMacroData(zOutput, MAX_STRING);
	if (DebugMode) WriteChatf("MQ2Medley::SongData::evalTarget(%s) [%s] returned=%s", name.c_str(), targetExp.c_str(), zOutput);

	const DWORD result = GetIntFromString(zOutput, 0);
	return result;
}