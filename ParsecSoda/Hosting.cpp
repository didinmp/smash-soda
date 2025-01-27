#include "Hosting.h"
#include "WebSocket.h"

using namespace std;

#if defined(_WIN32)
	#if !defined(BITS)
		#define BITS 64
	#endif
	#if (BITS == 64)
		#define SDK_PATH "./parsec.dll"
	#else
		#define SDK_PATH "./parsec32.dll"
	#endif
#endif

// ============================================================
// 
//  PUBLIC
// 
// ============================================================

Hosting::Hosting()
{
	_hostConfig = EMPTY_HOST_CONFIG;
	MetadataCache::loadPreferences();
	setHostConfig(
		MetadataCache::preferences.roomName,
		MetadataCache::preferences.gameID,
		MetadataCache::preferences.guestCount,
		MetadataCache::preferences.publicRoom,
		MetadataCache::preferences.secret
	);
	setHostVideoConfig(MetadataCache::preferences.fps, MetadataCache::preferences.bandwidth);

	_sfxList.init("./SFX/custom/_sfx.json");
	
	_tierList.loadTiers();
	_tierList.saveTiers();

	vector<GuestData> banned = MetadataCache::loadBannedUsers();
	_banList = BanList(banned);

	vector<GuestData> modded = MetadataCache::loadModdedUsers();
	_modList = ModList(modded);

	vector<GuestData> vip = MetadataCache::loadVIPUsers();
	_vipList = VIPList(vip);

	vector<GameData> games = MetadataCache::loadGamesList();
	_gamesList = GameDataList(games);

	_parsec = nullptr;

	SDL_Init(SDL_INIT_JOYSTICK);
	_masterOfPuppets.init(_gamepadClient);
	_masterOfPuppets.start();

	_latencyLimitEnabled = MetadataCache::preferences.latencyLimitEnabled;
	_latencyLimitValue = MetadataCache::preferences.latencyLimitValue;
	_disableMicrophone = MetadataCache::preferences.disableMicrophone;
	_disableGuideButton = MetadataCache::preferences.disableGuideButton;
	_disableKeyboard = MetadataCache::preferences.disableKeyboard;
	_lockedGamepad.bLeftTrigger = MetadataCache::preferences.lockedGamepadLeftTrigger;
	_lockedGamepad.bRightTrigger = MetadataCache::preferences.lockedGamepadRightTrigger;
	_lockedGamepad.sThumbLX = MetadataCache::preferences.lockedGamepadLX;
	_lockedGamepad.sThumbLY = MetadataCache::preferences.lockedGamepadLY;
	_lockedGamepad.sThumbRX = MetadataCache::preferences.lockedGamepadRX;
	_lockedGamepad.sThumbRY = MetadataCache::preferences.lockedGamepadRY;
	_lockedGamepad.wButtons = MetadataCache::preferences.lockedGamepadButtons;
}

void Hosting::applyHostConfig()
{
	if (isRunning())
	{
		ParsecHostSetConfig(_parsec, &_hostConfig, _parsecSession.sessionId.c_str());
	}
}

void Hosting::broadcastChatMessage(string message)
{
	vector<Guest> guests = _guestList.getGuests();
	vector<Guest>::iterator gi;

	for (gi = guests.begin(); gi != guests.end(); ++gi)
	{
		ParsecHostSendUserData(_parsec, (*gi).id, HOSTING_CHAT_MSG_ID, message.c_str());
	}
}

void Hosting::init()
{
	_parsecStatus = ParsecInit(NULL, NULL, (char *)SDK_PATH, &_parsec);
	_dx11.init();
	_gamepadClient.setParsec(_parsec);
	_gamepadClient.init();

	MetadataCache::Preferences preferences = MetadataCache::loadPreferences();

	_createGamepadsThread = thread([&]() {
		_gamepadClient.createAllGamepads();
		_createGamepadsThread.detach();
		_macro.init(_gamepadClient, _masterOfPuppets);
	});

	audioOut.fetchDevices();
	vector<AudioOutDevice> audioOutDevices = audioOut.getDevices();
	if (preferences.audioOutputDevice >= audioOutDevices.size()) {
		preferences.audioOutputDevice = 0;
	}
	audioOut.setOutputDevice(preferences.audioOutputDevice);		// TODO Fix leak in setOutputDevice
	audioOut.captureAudio();
	audioOut.volume = 0.3f;
	audioOut.setFrequency((Frequency)MetadataCache::preferences.speakersFrequency);

	vector<AudioInDevice> audioInputDevices = audioIn.listInputDevices();
	if (preferences.audioInputDevice >= audioInputDevices.size()) {
		preferences.audioInputDevice = 0;
	}
	AudioInDevice device = audioIn.selectInputDevice(preferences.audioInputDevice);
	audioIn.init(device);
	audioIn.volume = 0.8f;
	audioIn.setFrequency((Frequency)MetadataCache::preferences.micFrequency);

	preferences.isValid = true;
	MetadataCache::savePreferences(preferences);
	_parsecSession.loadThumbnails();
	_parsecSession.loadSessionCache();

	fetchAccountData();

	_chatBot = new ChatBot(
		audioIn, audioOut, _banList, _dx11, _modList, _vipList,
		_gamepadClient, _guestList, _guestHistory, _parsec,
		_hostConfig, _parsecSession, _sfxList, _tierList,
		_tournament, _macro, _isRunning, _host
	);

	CommandBonk::init();
}

void Hosting::release()
{
	stopHosting();
	while (_isRunning)
	{
		Sleep(5);
	}
	_dx11.clear();
	_gamepadClient.release();
	_masterOfPuppets.stop();
}

bool Hosting::isReady()
{
	return _parsecStatus == PARSEC_OK;
}

bool Hosting::isRunning()
{
	return _isRunning;
}

bool& Hosting::isGamepadLock()
{
	return _gamepadClient.lock;
}

bool& Hosting::isGamepadLockButtons()
{
	return _gamepadClient.lockButtons;
}

Guest& Hosting::getHost()
{
	return _host;
}

ParsecSession& Hosting::getSession()
{
	return _parsecSession;
}

void Hosting::fetchAccountData(bool sync)
{
	_host.name = "Host";
	_host.status = Guest::Status::INVALID;
	if (isReady())
	{
		if (sync)
		{
			_parsecSession.fetchAccountDataSync(&_host);
		}
		else
		{
			_parsecSession.fetchAccountData(&_host);
		}
	}
}

ParsecHostConfig& Hosting::getHostConfig()
{
	return _hostConfig;
}

DX11& Hosting::getDX11()
{
	return _dx11;
}

ChatBot* Hosting::getChatBot()
{
	return _chatBot;
}

vector<string>& Hosting::getMessageLog()
{
	return _chatLog.getMessageLog();
}

vector<string>& Hosting::getCommandLog()
{
	return _chatLog.getCommandLog();
}

vector<Guest>& Hosting::getGuestList()
{
	return _guestList.getGuests();
}

vector<GuestData>& Hosting::getGuestHistory()
{
	return _guestHistory.getGuests();
}

MyMetrics Hosting::getMetrics(uint32_t id)
{
	return _guestList.getMetrics(id);
}

BanList& Hosting::getBanList()
{
	return _banList;
}

ModList& Hosting::getModList()
{
	return _modList;
}

VIPList& Hosting::getVIPList()
{
	return _vipList;
}

GameDataList& Hosting::getGameList()
{
	return _gamesList;
}

vector<AGamepad*>& Hosting::getGamepads()
{
	return _gamepadClient.gamepads;
}

GamepadClient& Hosting::getGamepadClient()
{
	return _gamepadClient;
}

MasterOfPuppets& Hosting::getMasterOfPuppets()
{
	return _masterOfPuppets;
}

const char** Hosting::getGuestNames()
{
	return _guestList.guestNames;
}

void Hosting::toggleGamepadLock()
{
	_gamepadClient.toggleLock();
}

void Hosting::toggleGamepadLockButtons()
{
	_gamepadClient.toggleLockButtons();
}

void Hosting::setGameID(string gameID)
{
	try
	{
		strcpy_s(_hostConfig.gameID, gameID.c_str());
	}
	catch (const std::exception&) {}
}

void Hosting::setMaxGuests(uint8_t maxGuests)
{
	_hostConfig.maxGuests = maxGuests;
}

void Hosting::setHostConfig(string roomName, string gameId, uint8_t maxGuests, bool isPublicRoom)
{
	setRoomName(roomName);
	setGameID(gameId);
	setMaxGuests(maxGuests);
	setPublicRoom(isPublicRoom);
}

void Hosting::setHostConfig(string roomName, string gameId, uint8_t maxGuests, bool isPublicRoom, string secret)
{
	setRoomName(roomName);
	setGameID(gameId);
	setMaxGuests(maxGuests);
	setPublicRoom(isPublicRoom);
	setRoomSecret(secret);
}

void Hosting::setHostVideoConfig(uint32_t fps, uint32_t bandwidth)
{
	_hostConfig.video->encoderFPS = fps;
	_hostConfig.video->encoderMaxBitrate = bandwidth;
	MetadataCache::preferences.fps = fps;
	MetadataCache::preferences.bandwidth = bandwidth;
}

void Hosting::setPublicRoom(bool isPublicRoom)
{
	_hostConfig.publicGame = isPublicRoom;
}

void Hosting::setRoomName(string roomName)
{
	try
	{
		strcpy_s(_hostConfig.name, roomName.c_str());
	}
	catch (const std::exception&) {}
}

void Hosting::setRoomSecret(string secret)
{
	try
	{
		strcpy_s(_hostConfig.secret, secret.c_str());
	}
	catch (const std::exception&) {}
}

void Hosting::startHosting()
{

	if (!_isRunning)
	{
		_isRunning = true;
		initAllModules();

		try
		{
			if (_parsec != nullptr)
			{
				_mediaThread = thread ( [this]() {liveStreamMedia(); } );
				_inputThread = thread ([this]() {pollInputs(); });
				_eventThread = thread ([this]() {pollEvents(); });
				_latencyThread = thread([this]() {pollLatency(); });
				_smashSodaThread = thread([this]() { pollSmashSoda(); });
				//_gamepadThread = thread([this]() {pollGamepad(); });
				_mainLoopControlThread = thread ([this]() {mainLoopControl(); });

				// Start kiosk mode
				if (MetadataCache::preferences.kioskMode)
					kioskMode();

				// Init overlay
				

			}
		}
		catch (const exception&)
		{}
	}

	bool debug = true;
}

void Hosting::stopHosting()
{
	_isRunning = false;
	_guestList.clear();
	hotseatError = false;

	for (size_t i = 0; i < _gamepadClient.gamepads.size(); i++) {
		_gamepadClient.gamepads[i]->clearOwner();
		if (MetadataCache::preferences.hotseat) {
			_gamepadClient.gamepads[i]->disconnect();
		}
	}
}

void Hosting::kioskMode() {

	ofstream kioskBatch;

	kioskBatch.open("kioskbatch.bat", ios::out);
	kioskBatch << "@echo OFF\n";
	kioskBatch << ":Start\n";
	kioskBatch << "\"" + MetadataCache::preferences.kioskApplication + "\" " + MetadataCache::preferences.kioskParameters + "\n";
	kioskBatch << ":: Wait 30 seconds before restarting.\n";
	kioskBatch << "TIMEOUT /T 30\n";
	kioskBatch << "GOTO:Start\n";

	kioskBatch.close();

	ShellExecute(NULL, _T("open"), _T("cmd.exe"), _T("/C kioskbatch.bat"), 0, SW_SHOW);
	//remove("kioskbatch.bat");

}

void Hosting::stripGamepad(int index)
{
	_gamepadClient.clearOwner(index);
}

void Hosting::setOwner(AGamepad& gamepad, Guest newOwner, int padId)
{
	bool found = _gamepadClient.findPreferences(newOwner.userID, [&](GamepadClient::GuestPreferences& prefs) {
		gamepad.setOwner(newOwner, padId, prefs.mirror);
	});

	if (!found)
	{
		gamepad.setOwner(newOwner, padId, false);
	}
}

void Hosting::handleMessage(const char* message, Guest& guest, bool isHost, bool isHidden, bool outside)
{
	// Has the guest been muted?
	for (int i = 0; i < MetadataCache::preferences.mutedGuests.size(); i++) {
		if (guest.userID == MetadataCache::preferences.mutedGuests[i].id) {

			// Has their suffering gone on long enough?
			if (MetadataCache::preferences.mutedGuests[i].stopwatch.isFinished()) {
				MetadataCache::preferences.mutedGuests.erase(MetadataCache::preferences.mutedGuests.begin() + i);
				MetadataCache::preferences.mutedGuests.shrink_to_fit();
				break;
			}
			else {

				// Shhhhhhhhh
				return;

			}

		}
	}

	ACommand* command = _chatBot->identifyUserDataMessage(message, guest, isHost);
	command->run();

	// Non-blocked default message
	if (!isFilteredCommand(command))
	{
		Tier tier = _tierList.getTier(guest.userID);

		if (_webSocket.connected() && !outside)
		{
			MTY_JSON* jmsg = MTY_JSONObjCreate();
			MTY_JSONObjSetString(jmsg, "type", "chat");
			MTY_JSONObjSetUInt(jmsg, "id", guest.id);
			MTY_JSONObjSetUInt(jmsg, "userid", guest.userID);
			MTY_JSONObjSetString(jmsg, "username", guest.name.c_str());
			MTY_JSONObjSetString(jmsg, "content", message);
			char* finmsg = MTY_JSONSerialize(jmsg);
			_webSocket.handle_message(finmsg);
			MTY_JSONDestroy(&jmsg);
			MTY_Free(finmsg);

		}

		CommandDefaultMessage defaultMessage(message, guest, _chatBot->getLastUserId(), tier);
		defaultMessage.run();
		_chatBot->setLastUserId(guest.userID);

		if (!defaultMessage.replyMessage().empty() && !isHidden)
		{
			broadcastChatMessage(defaultMessage.replyMessage());
			
			string adjustedMessage = defaultMessage.replyMessage();
			Stringer::replacePatternOnce(adjustedMessage, "%", "%%");
			_chatLog.logMessage(adjustedMessage);
		}
	}

	// Chatbot's command reply
	if (!command->replyMessage().empty() && command->type() != COMMAND_TYPE::DEFAULT_MESSAGE)
	{

		if (_webSocket.connected())
		{
			MTY_JSON* jmsg = MTY_JSONObjCreate();
			MTY_JSONObjSetString(jmsg, "type", "command");
			MTY_JSONObjSetUInt(jmsg, "id", guest.id);
			MTY_JSONObjSetUInt(jmsg, "userid", guest.userID);
			MTY_JSONObjSetString(jmsg, "username", guest.name.c_str());
			MTY_JSONObjSetString(jmsg, "content", command->replyMessage().c_str());
			char* finmsg = MTY_JSONSerialize(jmsg);
			_webSocket.handle_message(finmsg);
			MTY_JSONDestroy(&jmsg);
			MTY_Free(finmsg);
		}

		broadcastChatMessage(command->replyMessage());
		_chatLog.logCommand(command->replyMessage());
		_chatBot->setLastUserId();
	}

	delete command;

}

void Hosting::sendHostMessage(const char* message, bool isHidden)
{
	static bool isAdmin = true;
	handleMessage(message, _host, true, isHidden);
}


// ============================================================
// 
//  PRIVATE
// 
// ============================================================
void Hosting::initAllModules()
{
	// Instance all gamepads at once
	_connectGamepadsThread = thread([&]() {
		_gamepadClient.sortGamepads();
		_connectGamepadsThread.detach();
	});

	parsecArcadeStart();
}

void Hosting::submitSilence()
{
	ParsecHostSubmitAudio(_parsec, PCM_FORMAT_INT16, audioOut.getFrequencyHz(), nullptr, 0);
}

void Hosting::liveStreamMedia()
{
	_mediaMutex.lock();
	_isMediaThreadRunning = true;

	static uint32_t sleepTimeMs = 4;
	_mediaClock.setDuration(sleepTimeMs);
	_mediaClock.start();

	while (_isRunning)
	{
		_mediaClock.reset();

		_dx11.captureScreen(_parsec);

		if (!_disableMicrophone && audioIn.isEnabled && audioOut.isEnabled)
		{
			audioIn.captureAudio();
			audioOut.captureAudio();
			if (audioIn.isReady() && audioOut.isReady())
			{
				vector<int16_t> mixBuffer = _audioMix.mix(audioIn.popBuffer(), audioOut.popBuffer());
				ParsecHostSubmitAudio(_parsec, PCM_FORMAT_INT16, audioOut.getFrequencyHz(), mixBuffer.data(), (uint32_t)mixBuffer.size() / 2);
			}
			else submitSilence();
		}
		else if (audioOut.isEnabled)
		{
			audioOut.captureAudio();
			if (audioOut.isReady())
			{
				vector<int16_t> buffer = audioOut.popBuffer();
				ParsecHostSubmitAudio(_parsec, PCM_FORMAT_INT16, audioOut.getFrequencyHz(), buffer.data(), (uint32_t)buffer.size() / 2);
			}
			else submitSilence();
		}
		else if (!_disableMicrophone && audioIn.isEnabled)
		{
			audioIn.captureAudio();
			if (audioIn.isReady())
			{
				vector<int16_t> buffer = audioIn.popBuffer();
				ParsecHostSubmitAudio(_parsec, PCM_FORMAT_INT16, (uint32_t)audioIn.getFrequency(), buffer.data(), (uint32_t)buffer.size() / 2);
			}
			else submitSilence();
		}
		else submitSilence();

		sleepTimeMs = _mediaClock.getRemainingTime();
		if (sleepTimeMs > 0)
		{
			Sleep(sleepTimeMs);
		}
	}

	_isMediaThreadRunning = false;
	_mediaMutex.unlock();
	_mediaThread.detach();
}

void Hosting::mainLoopControl()
{
	do
	{
		Sleep(50);
	} while (!_isRunning);

	_isRunning = true;

	_mediaMutex.lock();
	_inputMutex.lock();
	_eventMutex.lock();

	ParsecHostStop(_parsec);
	_isRunning = false;

	_mediaMutex.unlock();
	_inputMutex.unlock();
	_eventMutex.unlock();

	_mainLoopControlThread.detach();
}

void Hosting::pollEvents()
{
	_eventMutex.lock();
	_isEventThreadRunning = true;

	string chatBotReply;

	ParsecGuest* guests = nullptr;
	int guestCount = 0;

	ParsecHostEvent event;

	while (_isRunning)
	{
		if (ParsecHostPollEvents(_parsec, 30, &event)) {
			ParsecGuest parsecGuest = event.guestStateChange.guest;
			ParsecGuestState state = parsecGuest.state;
			Guest guest = Guest(parsecGuest.name, parsecGuest.userID, parsecGuest.id);
			guestCount = ParsecHostGetGuests(_parsec, GUEST_CONNECTED, &guests);
			_guestList.setGuests(guests, guestCount);

			switch (event.type)
			{
			case HOST_EVENT_GUEST_STATE_CHANGE:
				onGuestStateChange(state, guest, event.guestStateChange.status);
				break;

			case HOST_EVENT_USER_DATA:
				char* msg = (char*)ParsecGetBuffer(_parsec, event.userData.key);

				if (event.userData.id == PARSEC_APP_CHAT_MSG)
				{
					handleMessage(msg, guest);
				}

				ParsecFree(_parsec, msg);
				break;
			}
		}
	}

	ParsecFree(_parsec, guests);
	_isEventThreadRunning = false;
	_eventMutex.unlock();
	_eventThread.detach();
}

void Hosting::pollLatency()
{
	_latencyMutex.lock();
	_isLatencyThreadRunning = true;
	int guestCount = 0;
	ParsecGuest* guests = nullptr;
	while (_isRunning)
	{
		Sleep(2000);
		guestCount = ParsecHostGetGuests(_parsec, GUEST_CONNECTED, &guests);
		if (guestCount > 0) {
			_guestList.updateMetrics(guests, guestCount);

			// Test (v6 uncommented this...but seems to work...hmmmmmmm)
			if (MetadataCache::preferences.latencyLimitEnabled) {
				for (size_t mi = 0; mi < guestCount; mi++) {
					MyMetrics m = _guestList.getMetrics(guests[mi].id);
					if (m.metrics.networkLatency > 0.1f && m.averageNetworkLatencySize <= 3) {
						//_chatLog.logCommand(string("!") + guests[mi].name + " P:" + to_string(m.metrics.packetsSent) + " L:" + to_string(m.metrics.networkLatency));
					}
					if (m.averageNetworkLatencySize > 5 && m.averageNetworkLatency > _latencyLimitValue) {
						ParsecHostKickGuest(_parsec, guests[mi].id);
						//_chatLog.logCommand(string("!>") + to_string(_latencyLimitValue) + " \t\t " + guests[mi].name + " #" + to_string(guests[mi].userID));
					}
				}
			}

		}
		if (_webSocket.connected())
		{
			// submit guests/metrics
			MTY_JSON* jmsg = MTY_JSONObjCreate();
			MTY_JSON* jdata = MTY_JSONArrayCreate();
			for (size_t mi = 0; mi < guestCount; mi++)
			{
				MTY_JSON* jmetric = MTY_JSONObjCreate();
				MTY_JSONObjSetUInt(jmetric, "id", guests[mi].id);
				MTY_JSONObjSetUInt(jmetric, "userid", guests[mi].userID);
				MTY_JSONObjSetString(jmetric, "username", guests[mi].name);
				MTY_JSONObjSetBool(jmetric, "banned", _banList.isBanned(guests[mi].userID));
				MTY_JSONObjSetFloat(jmetric, "networkLatency", guests[mi].metrics[0].networkLatency);
				MTY_JSONObjSetUInt(jmetric, "fastRTs", guests[mi].metrics[0].fastRTs);
				MTY_JSONObjSetUInt(jmetric, "slowRTs", guests[mi].metrics[0].slowRTs);
				MTY_JSONArrayAppendItem(jdata, jmetric);
			}
			MTY_JSONObjSetString(jmsg, "type", "metrics");
			MTY_JSONObjSetItem(jmsg, "content", jdata);
			char* finmsg = MTY_JSONSerialize(jmsg);
			_webSocket.handle_message(finmsg);
			MTY_JSONDestroy(&jmsg);
			MTY_Free(finmsg);

			// submit all gamepads
			MTY_JSON* jmsg2 = MTY_JSONObjCreate();
			MTY_JSON* jdata2 = MTY_JSONArrayCreate();
			for (size_t gi = 0; gi < _gamepadClient.gamepads.size(); ++gi)
			{
				MTY_JSON* jmetric = MTY_JSONObjCreate();
				MTY_JSONObjSetUInt(jmetric, "index", gi);
				if (_gamepadClient.gamepads[gi]->isOwned())
				{
					MTY_JSONObjSetUInt(jmetric, "id", _gamepadClient.gamepads[gi]->owner.guest.id);
					MTY_JSONObjSetUInt(jmetric, "userid", _gamepadClient.gamepads[gi]->owner.guest.userID);
					MTY_JSONObjSetString(jmetric, "username", _gamepadClient.gamepads[gi]->owner.guest.name.c_str());
				}
				MTY_JSONArrayAppendItem(jdata2, jmetric);
			}
			MTY_JSONObjSetString(jmsg2, "type", "gamepadall");
			MTY_JSONObjSetItem(jmsg2, "content", jdata2);
			char* finmsg2 = MTY_JSONSerialize(jmsg2);
			_webSocket.handle_message(finmsg2);
			MTY_JSONDestroy(&jmsg2);
			MTY_Free(finmsg2);
		}
	}
	_isLatencyThreadRunning = false;
	_latencyMutex.unlock();
	_latencyThread.detach();
}

/// <summary>
/// This thread handles all the new Smash Soda features!
/// </summary>
void Hosting::pollSmashSoda() {
	_smashSodaMutex.lock();
	_isSmashSodaThreadRunning = true;
	while (_isRunning) {

		Sleep(200);

		// Handles all the automatic button press stuff
		_macro.run();

		// Handles the hotseat cycling
		Hosting::hotseat();

		// Handles welcome messages for new guests
		//Hosting::welcomeMessage();

	}
	_isSmashSodaThreadRunning = false;
	_smashSodaMutex.unlock();
	_smashSodaThread.detach();

}

/// <summary>
/// Handles all the hotseat stuff.
/// </summary>
bool Hosting::hotseat() {

	// Hotseat enabled and gamepad 1 connected?
	if (MetadataCache::preferences.hotseat) {

		// Need to do several checks to see if hotseat mode can be enabled
		if (!hotseatModeTest()) return false;

		// Start timer if it is not running
		if (!MetadataCache::hotseat.hotseatClock.isRunning())
			startHotseatTimer();

		// Are there guests in the room?
		if (_guestList.getGuests().size() > 0) {

			// Hotseat guest is still here
			if (MetadataCache::hotseat.guest.isValid()) {

				// Get current hotseat guest index
				hotseatIndex = findHotseatGuest();

				// Remaining time
				uint32_t time = round(MetadataCache::hotseat.hotseatClock.getRemainingTime() / 60000);

				// Reminder in chat
				if (MetadataCache::hotseat.reminderClock.isFinished()) {
					broadcastChatMessage("[HOTSEAT] | " + _guestList.getGuests()[hotseatIndex].name + " has " + std::to_string(time) + " minutes left to play.\0");
					MetadataCache::hotseat.reminderClock.reset();
				}

				// 10 second warning
				if (MetadataCache::hotseat.isWarning && MetadataCache::hotseat.hotseatClock.getRemainingTime() < 10000) {
					broadcastChatMessage("[HOTSEAT] | The gamepad is about to be swapped in 10 SECONDS!\0");
					MetadataCache::hotseat.isWarning = false;
				}

				// Time is up. is spectating or gamepad been dropped?
				if (!_gamepadClient.getGamepad(0)->isOwned() || isSpectator(hotseatIndex) || MetadataCache::hotseat.hotseatClock.isFinished()) {

					// Find next hotseat guest
					do {
						if (hotseatIndex < _guestList.getGuests().size() - 1) {
							hotseatIndex++;
						} 
						else {
							hotseatIndex = 0;
						}
							
					} while (isSpectator(hotseatIndex));

					// Set new hotseat guest
					setHotseatGuest(hotseatIndex);

				}

			}

			// Hotseat guest left or something wrong
			else setHotseatGuest(hotseatIndex);

		}

		// Nobody here, so we can't hotseat yet
		else stopHotseatTimer();

	}
	else {

		// Stop hotseat timers if running
		if (MetadataCache::hotseat.hotseatClock.isRunning())
			stopHotseatTimer();

	}

	return true;

}

/// <summary>
/// Starts the hotseat and warning timers.
/// </summary>
void Hosting::startHotseatTimer() {
	uint32_t minutes = MetadataCache::preferences.hotseatTime * 60000;

	MetadataCache::hotseat.hotseatClock.reset(minutes);
	MetadataCache::hotseat.reminderClock.reset(minutes / 4);

	MetadataCache::hotseat.hotseatClock.start();
	MetadataCache::hotseat.reminderClock.start();

	MetadataCache::hotseat.isWarning = true;
}

/// <summary>
/// Stops the hotseat and warning timers.
/// </summary>
void Hosting::stopHotseatTimer() {
	MetadataCache::hotseat.hotseatClock.reset();
	MetadataCache::hotseat.reminderClock.reset();

	MetadataCache::hotseat.hotseatClock.stop();
	MetadataCache::hotseat.reminderClock.stop();
}

/// <summary>
/// Loops through guest list to find the next
/// hotseat guest.
/// </summary>
int Hosting::findHotseatGuest() {

	// Loop through guest list until we find current hotseat guest
	for (int i = 0; i < _guestList.getGuests().size(); i++) {
		if (_guestList.getGuests()[i].userID == MetadataCache::hotseat.guest.userID)
			return i;
	}

	// Hotseat guest not valid or not chosen yet, put first guest in hotseat
	return 0;

}

/// <summary>
/// Puts a guest in the guest list in to the hotseat.
/// </summary>
/// <param name="index"></param>
void Hosting::setHotseatGuest(int index) {

	// Stop vibrating last guest
	// I dungeddit :(
	//ParsecHostSubmitRumble(_gamepadClient.getGamepad(0)->parsec, MetadataCache::hotseat.guest.id,_gamepadClient.getGamepad(0)->owner.deviceID, 0, 0);

	// Set new hotseat guest
	MetadataCache::hotseat.guest = _guestList.getGuests()[index];

	// Set gamepad
	_gamepadClient.getGamepad(0)->setOwner(MetadataCache::hotseat.guest, 0, false);

	// Press pause for new player
	if (MetadataCache::preferences.hotseatPause)
		MetadataCache::autoGamepad.buttonList.push_back(ParsecGamepadButton::GAMEPAD_BUTTON_START);

	// Chatbot
	broadcastChatMessage("[ChatBot] | " + MetadataCache::hotseat.guest.name + " now has control of the gamepad!\0");

	// Reset timers
	startHotseatTimer();

	// Start AFK timer
	if (MetadataCache::preferences.hotseatAFK) {
		_afkClock.reset(MetadataCache::preferences.hotseatAFKTime * 60000);
	}

}

/// <summary>
/// Checks to see if hotseat mode can be started.
/// Messing with gamepads is BSOD territory, so here we want
/// to make absolutely sure we can go in to hotseat mode.
/// </summary>
/// <returns>boolean</returns>
bool Hosting::hotseatModeTest() {

	// Are there people in the room?
	if (_guestList.getGuests().size() < 1) {
		if (hotseatError != 1) {
			_chatLog.logMessage("[HOTSEAT] There needs to be at least one guest in the room.");
			hotseatError = 1;
		}
		return false;
	}

	// Ensure at least one gamepad has been enabled
	if (_gamepadClient.gamepads.size() < 1) {
		if (hotseatError != 1) {
			_chatLog.logMessage("[HOTSEAT] Please enable at least one Xbox gamepad.");
			hotseatError = 1;
		}
		return false;
	}

	// Ensure gamepad 1 is attached and connected
	if (!_gamepadClient.getGamepad(0)->isAttached()) {
		if (hotseatError != 2) {
			_chatLog.logMessage("[HOTSEAT] Please enable the first gamepad.");
			hotseatError = 2;
		}
		return false;
	}

	// Xbox controller only
	if (_gamepadClient.getGamepad(0)->type() != AGamepad::Type::XBOX) {
		if (hotseatError != 3) {
			_chatLog.logMessage("[HOTSEAT] Make sure the first gamepad is an Xbox gamepad (not Playstation controller).");
			hotseatError = 3;
		}
		return false;
	}

	// Xbox index 1?
	if (_gamepadClient.getGamepad(0)->getIndex() != 0) {
		if (hotseatError != 4) {
			_chatLog.logMessage("[HOTSEAT] Most single player games will only work with the first Xbox controller connected. The host needs to disconnect their controller.");
			hotseatError = 4;
		}
		return false;
	}

	hotseatError = -1;
	return true;

}

/// <summary>
/// Checks to see if a guest is a spectator.
/// </summary>
/// <param name="index"></param>
bool Hosting::isSpectator(int index) {

	return MetadataCache::isSpectating(_guestList.getGuests()[index]);

}

void Hosting::welcomeMessage() {

	if (_showWelcome && _welcomeClock.isFinished()) {

		broadcastChatMessage("This is just a test.");

		_showWelcome = false;

	}

}

bool Hosting::isLatencyRunning()
{
	return _isLatencyThreadRunning;
}

void Hosting::pollGamepad()
{
	_gamepadMutex.lock();
	_isGamepadThreadRunning = true;
	while (_isRunning)
	{
		Sleep(33);
		if (_gamepadClient.gamepads.size() > 0 && _webSocket.connected())
		{
			MTY_JSON* jmsg3 = MTY_JSONObjCreate();
			MTY_JSON* jdata3 = MTY_JSONArrayCreate();
			for (size_t i = 0; i < _gamepadClient.gamepads.size(); i++)
			{
				if (!_gamepadClient.gamepads[i]->isConnected()) continue;
				XINPUT_STATE test = _gamepadClient.gamepads[i]->getState();
				MTY_JSON* jmetric = MTY_JSONObjCreate();
				MTY_JSONObjSetUInt(jmetric, "i", i);
				MTY_JSONObjSetInt(jmetric, "lt", test.Gamepad.bLeftTrigger);
				MTY_JSONObjSetInt(jmetric, "rt", test.Gamepad.bRightTrigger);
				MTY_JSONObjSetInt(jmetric, "lx", test.Gamepad.sThumbLX);
				MTY_JSONObjSetInt(jmetric, "ly", test.Gamepad.sThumbLY);
				MTY_JSONObjSetInt(jmetric, "rx", test.Gamepad.sThumbRX);
				MTY_JSONObjSetInt(jmetric, "ry", test.Gamepad.sThumbRY);
				MTY_JSONObjSetInt(jmetric, "b", test.Gamepad.wButtons);
				MTY_JSONArrayAppendItem(jdata3, jmetric);
			}
			MTY_JSONObjSetString(jmsg3, "type", "gamepadstate");
			MTY_JSONObjSetItem(jmsg3, "content", jdata3);
			char* finmsg3 = MTY_JSONSerialize(jmsg3);
			_webSocket.handle_message(finmsg3);
			MTY_JSONDestroy(&jmsg3);
			MTY_Free(finmsg3);
		}
	}
	_isGamepadThreadRunning = false;
	_gamepadMutex.unlock();
	_gamepadThread.detach();
}

bool Hosting::isGamepadRunning()
{
	return _isGamepadThreadRunning;
}

void Hosting::pollInputs()
{
	_inputMutex.lock();
	_isInputThreadRunning = true;

	ParsecGuest inputGuest;
	ParsecMessage inputGuestMsg;

	while (_isRunning)
	{
		if (ParsecHostPollInput(_parsec, 4, &inputGuest, &inputGuestMsg))
		{
			if (!_gamepadClient.lock)
			{
					_gamepadClient.sendMessage(inputGuest, inputGuestMsg);
			}

			// Hotseat AFK Timer
			if (MetadataCache::preferences.hotseat && MetadataCache::preferences.hotseatAFK) {
				_afkClock.reset(MetadataCache::preferences.hotseatAFKTime * 60000);
			}

		}
		else {

			// Hotseat enabled
			if (MetadataCache::preferences.hotseat) {

				// Guest has been afk for a minute 
				if (MetadataCache::preferences.hotseatAFK && _afkClock.isFinished()) {
					if (_guestList.getGuests().size() > 0 && MetadataCache::hotseat.guest.isValid()) {
						broadcastChatMessage(MetadataCache::hotseat.guest.name + "  was AFK and has now been set to !spectate");
						MetadataCache::hotseat.spectators.push_back(MetadataCache::hotseat.guest.userID);
					}
					_afkClock.reset(MetadataCache::preferences.hotseatAFKTime * 60000);
				}

			}

		}
	}

	_isInputThreadRunning = false;
	_inputMutex.unlock();
	_inputThread.detach();
}

void Hosting::webSocketStart(string uri, string password)
{
	if (!_isWebSocketThreadRunning)
	{
		_webSocketThread = thread([this,uri,password]() {webSocketRun(uri,password); });
	}
}

void Hosting::webSocketStop()
{
	if (_webSocket.connected())
	{
		_webSocket.close();
	}
}

WebSocket& Hosting::getWebSocket()
{
	return _webSocket;
}

void Hosting::webSocketRun(string uri, string password)
{
	_webSocketMutex.lock();
	_isWebSocketThreadRunning = true;

	_webSocket.start(uri, password);

	_isWebSocketThreadRunning = false;
	_webSocketMutex.unlock();
	_webSocketThread.detach();
}

bool Hosting::webSocketRunning()
{
	return _isWebSocketThreadRunning;
}

void Hosting::updateButtonLock(LockedGamepadState lockedGamepad)
{
	_lockedGamepad = lockedGamepad;
	MetadataCache::preferences.lockedGamepadLeftTrigger = _lockedGamepad.bLeftTrigger;
	MetadataCache::preferences.lockedGamepadRightTrigger = _lockedGamepad.bRightTrigger;
	MetadataCache::preferences.lockedGamepadLX = _lockedGamepad.sThumbLX;
	MetadataCache::preferences.lockedGamepadLY = _lockedGamepad.sThumbLY;
	MetadataCache::preferences.lockedGamepadRX = _lockedGamepad.sThumbRX;
	MetadataCache::preferences.lockedGamepadRY = _lockedGamepad.sThumbRY;
	MetadataCache::preferences.lockedGamepadButtons = _lockedGamepad.wButtons;
}

bool Hosting::parsecArcadeStart()
{
	if (isReady()) {
		//ParsecSetLogCallback(logCallback, NULL);
		ParsecStatus status = ParsecHostStart(_parsec, HOST_GAME, &_hostConfig, _parsecSession.sessionId.c_str());
		return status == PARSEC_OK;
	}
	return false;
}

bool Hosting::isFilteredCommand(ACommand* command)
{
	static vector<COMMAND_TYPE> filteredCommands{ COMMAND_TYPE::IP };

	COMMAND_TYPE type;
	for (vector<COMMAND_TYPE>::iterator it = filteredCommands.begin(); it != filteredCommands.end(); ++it)
	{
		type = command->type();
		if (command->type() == *it)
		{
			return true;
		}
	}

	return false;
}

void Hosting::onGuestStateChange(ParsecGuestState& state, Guest& guest, ParsecStatus& status)
{
	static string logMessage;

	static string trickDesc = "";
	static Debouncer debouncer = Debouncer(500, [&]() {
		if (_hostConfig.maxGuests > 0 && _guestList.getGuests().size() + 1 == _hostConfig.maxGuests)
		{
			try
			{
				if (trickDesc.size() > 0) trickDesc = "";
				else trickDesc = "-";
				strcpy_s(_hostConfig.desc, trickDesc.c_str());
				applyHostConfig();
			}
			catch (const std::exception&) {}
		}
	});

	if (_webSocket.connected())
	{
		MTY_JSON* jmsg = MTY_JSONObjCreate();
		MTY_JSONObjSetString(jmsg, "type", "gueststate");
		MTY_JSONObjSetUInt(jmsg, "id", guest.id);
		MTY_JSONObjSetUInt(jmsg, "userid", guest.userID);
		MTY_JSONObjSetString(jmsg, "username", guest.name.c_str());
		MTY_JSONObjSetUInt(jmsg, "state", state);
		MTY_JSONObjSetInt(jmsg, "status", status);
		MTY_JSONObjSetBool(jmsg, "banned", _banList.isBanned(guest.userID));
		char* finmsg = MTY_JSONSerialize(jmsg);
		_webSocket.handle_message(finmsg);
		MTY_JSONDestroy(&jmsg);
		MTY_Free(finmsg);
	}

	// Is the connecting guest the host?
	if ((state == GUEST_CONNECTED || state == GUEST_CONNECTING) && (_host.userID == guest.userID))
	{
		_tierList.setTier(guest.userID, Tier::GOD);
		MetadataCache::addActiveGuest(guest);
	}
	else

	if ((state == GUEST_CONNECTED || state == GUEST_CONNECTING) && _banList.isBanned(guest.userID))
	{
		ParsecHostKickGuest(_parsec, guest.id);
		logMessage = _chatBot->formatBannedGuestMessage(guest);
		broadcastChatMessage(logMessage);
		_chatLog.logCommand(logMessage);
	}
	else if ((state == GUEST_CONNECTED || state == GUEST_CONNECTING) && _modList.isModded(guest.userID))
	{
		logMessage = _chatBot->formatModGuestMessage(guest);
		broadcastChatMessage(logMessage);
		_tierList.setTier(guest.userID, Tier::MOD);
		_chatLog.logCommand(logMessage);
		MetadataCache::addActiveGuest(guest);
	}
	else if (state == GUEST_FAILED)
	{
		logMessage = _chatBot->formatGuestConnection(guest, state, status);
		broadcastChatMessage(logMessage);
		_chatLog.logCommand(logMessage);
	}
	else if (state == GUEST_CONNECTED || state == GUEST_DISCONNECTED)
	{
		static string guestMsg;
		guestMsg.clear();
		guestMsg = string(guest.name);

		if (_banList.isBanned(guest.userID)) {
			logMessage = _chatBot->formatBannedGuestMessage(guest);
			broadcastChatMessage(logMessage);
			_chatLog.logCommand(logMessage);
			if (_hostConfig.maxGuests > 0 && _guestList.getGuests().size() + 1 == _hostConfig.maxGuests)
				debouncer.start();
		}
		else
		{
			logMessage = _chatBot->formatGuestConnection(guest, state, status);
			broadcastChatMessage(logMessage);
			_chatLog.logCommand(logMessage);
		}

		if (state == GUEST_CONNECTED) {
			_guestHistory.add(GuestData(guest.name, guest.userID));
			MetadataCache::addActiveGuest(guest);
		}
		else
		{
			// Were extra spots made?
			if (status != 11 && MetadataCache::preferences.extraSpots > 0) {
				_hostConfig.maxGuests = _hostConfig.maxGuests - 1;
				MetadataCache::preferences.extraSpots--;
				ParsecHostSetConfig(_parsec, &_hostConfig, _parsecSession.sessionId.c_str());
			}

			// Remove from spectator list
			MetadataCache::removeActiveGuest(guest);
			if (MetadataCache::hotseat.spectators.empty() == false) {
				for (int i = MetadataCache::hotseat.spectators.size() - 1; i >= 0; i--) {
					if (MetadataCache::hotseat.spectators.at(i) == guest.userID) {
						MetadataCache::hotseat.spectators.erase(MetadataCache::hotseat.spectators.begin() + i);
					}
				}
			}

			_guestList.deleteMetrics(guest.id);
			int droppedPads = 0;
			CommandFF command(guest, _gamepadClient);
			command.run();
			if (droppedPads > 0)
			{
				logMessage = command.replyMessage();
				broadcastChatMessage(logMessage);
				_chatLog.logCommand(logMessage);
			}
		}
	}
}

bool Hosting::removeGame(string name) {

	_gamesList.remove(name, [&](GameData& guest) {
		
	});

	return true;

}