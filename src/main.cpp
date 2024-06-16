// clang-format off

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/EditLevelLayer.hpp>
#include <Geode/modify/LevelSelectLayer.hpp>

#include <Geode/binding/FLAlertLayer.hpp>

#include "json/single_include/nlohmann/json.hpp"

#include <Geode/cocos/actions/CCActionInterval.h>

#include <filesystem>

#include <vector>

#include <Geode/binding/CCCircleWave.hpp>
#include <Geode/cocos/particle_nodes/CCParticleSystemQuad.h>
#include <Geode/binding/FMODAudioEngine.hpp>

#include <map>

using namespace geode::prelude;
// using namespace geode::modifier;

nlohmann::json _progresses;

std::string generateFilename(GJGameLevel *level) {
	int id = level->m_levelID.value();

	std::string path = Mod::get()->getSaveDir().generic_string();
	std::string filename = path + "/level_" + std::to_string(id) + ".json";

	if (id == 0) {
		std::string name = level->m_levelName;
		int chk = level->m_chk;
		int rev = level->m_levelRev;
		int sid = level->m_songID;

		filename = path + "/level_C_" + name + std::to_string(chk) + std::to_string(rev) + std::to_string(sid) + ".json";
	}

	return filename;
}

class DRDelegate : public FLAlertLayerProtocol {
public:
	GJGameLevel *lvl = nullptr;

	void FLAlert_Clicked(FLAlertLayer *l, bool idk) override {
		if (idk != 1) return;

		auto filepath = generateFilename(this->lvl);

		std::filesystem::remove(filepath);

		FLAlertLayer *al = FLAlertLayer::create("Success!", "Death Replay file has been <cr>removed</c> for this <cy>level</c>.", "OK");
		al->show(); 
	}
};

class XPlayLayer;

namespace DRSettings {
	bool showDeaths = true;
	bool showParticles = true;
	bool playDeathEffect = true;
	bool withGlobed = false;
	
	bool recordPractice = false;
	bool currentlyInPractice = false;

	bool showGhost = false;
	float ghostFPS = 60.f;

	int maxDeaths = 1024;

	double playTime = 0.f;
	int currentAttempt = 1;
	bool inPlatformer = false;

	bool isOldLevel = false;

	DRDelegate delegate;
	GJGameLevel *levelInstance;
	XPlayLayer *actionInstance;
}

namespace DRGlobal {
	std::map<std::string, int> getOfficialLevels() {
		return {
			{"Stereo Madness", 1},
			{"Back On Track", 2},
			{"Polargeist", 3},
			{"Dry Out", 4},
			{"Base After Base", 5},
			{"Cant Let Go", 6},
			{"Jumper", 7},
			{"Time Machine", 8},
			{"Cycles", 9},
			{"xStep", 10},
			{"Clutterfunk", 11},
			{"Theory of Everything", 12},
			{"Electroman Adventures", 13},
			{"Clubstep", 14},
			{"Electrodynamix", 15},
			{"Hexagon Force", 16},
			{"Blast Processing", 17},
			{"Theory of Everything 2", 18},
			{"Geometrical Dominator", 19},
			{"Deadlocked", 20},
			{"Fingerdash", 21},
			{"Dash", 22}
		};
	}

	bool hasLevel(int id) {
		auto map = getOfficialLevels();

		for (auto [key, val] : map) {
			if (val == id) return true;
		}

		return false;
	}
};

class $modify(XLevelInfoLayer, LevelInfoLayer) {
	static void onModify(auto & self) {
		(void) self.setHookPriority("LevelInfoLayer::init", INT32_MIN + 1);
	}
	void onDeathReplay(CCObject *target) {
		DRSettings::delegate.lvl = DRSettings::levelInstance;

		FLAlertLayer *l = FLAlertLayer::create(&DRSettings::delegate, "Death Replay", "Are you sure you want to <cr>remove</c> the Death Replay file for this <cy>level</c>?", "No", "Yes");
		l->show(); 
	}

	bool init(GJGameLevel *lvl, bool idk) {
		if (!LevelInfoLayer::init(lvl, idk)) { return false; }

		DRSettings::levelInstance = lvl;

		CCSprite *btn_spr = CCSprite::createWithSpriteFrameName("backArrowPlain_01_001.png");
		btn_spr->setColor(ccRED);
		btn_spr->setFlipX(true);

		CCMenuItemSpriteExtra *spr_men = CCMenuItemSpriteExtra::create(btn_spr, this, menu_selector(XLevelInfoLayer::onDeathReplay));
		spr_men->setID("dr-remove-file");

		getChildByID("back-menu")->addChild(spr_men);
		getChildByID("back-menu")->updateLayout();

		return true;
	}
};

class $modify(XLevelSelectLayer, LevelSelectLayer) {
	int detectLevelID() {
		int level_id = -1;
		
		auto batch = getChildByIDRecursive("page-buttons");
		CCArray *ch = batch->getChildren();

		for (int i = 0; i < ch->count(); i++) {
			CCObject *as_object = ch->objectAtIndex(i);
			CCSprite *as_sprite = typeinfo_cast<CCSprite *>(as_object);

			if (as_sprite == nullptr) continue;

			auto color = as_sprite->getColor();
			if (color.r == 255 && color.g == 255 && color.b == 255) {
				level_id = i + 1;

				break;
			}
		}

		return level_id;
	}

	void onDeathReplay(CCObject *target) {
		int level_id = detectLevelID();
		if (level_id <= 0) return;

		DRSettings::levelInstance = LevelTools::getLevel(level_id, true);
		DRSettings::delegate.lvl = DRSettings::levelInstance;

		std::string levelName = DRSettings::levelInstance->m_levelName;

		std::string a = "Are you sure you want to <cr>remove</c> Death Replay file for <cy>" + levelName + "</c>?";

		FLAlertLayer *l = FLAlertLayer::create(&DRSettings::delegate, "Death Replay", a, "No", "Yes");
		l->show(); 
	}

	void hideButton() {
		auto btn = (CCMenuItemSpriteExtra *)getChildByIDRecursive("dr-remove-file");
		btn->runAction(CCFadeTo::create(0.1f, 0.f));
		btn->setEnabled(false);
	}
	void showButton() {
		auto btn = (CCMenuItemSpriteExtra *)getChildByIDRecursive("dr-remove-file");
		btn->runAction(CCFadeTo::create(0.1f, 255.f));
		btn->setEnabled(true);
	}

	void checkPossibility() {
		int level_id = detectLevelID();

		if (DRGlobal::hasLevel(level_id)) {
			showButton();

			return;
		}

		hideButton();
	}

	void checkPossibilityS(float delta) {
		checkPossibility();
	}

	bool init(int p0) {
		if (!LevelSelectLayer::init(p0)) return false;
		
		CCSprite *btn_spr = CCSprite::createWithSpriteFrameName("backArrowPlain_01_001.png");
		btn_spr->setColor(ccRED);
		btn_spr->setFlipX(true);

		CCMenuItemSpriteExtra *spr_men = CCMenuItemSpriteExtra::create(btn_spr, this, menu_selector(XLevelSelectLayer::onDeathReplay));
		spr_men->setID("dr-remove-file");

		CCMenu *menu = CCMenu::create();

		menu->addChild(spr_men);

		CCDirector *dir = CCDirector::sharedDirector();
		auto sz = dir->getWinSize();

		menu->setPosition(65, sz.height - btn_spr->getContentSize().height + 8);

		addChild(menu);

		schedule(schedule_selector(XLevelSelectLayer::checkPossibilityS), 0.1f);

#ifndef _WIN32
		menu->setPosition({9999, 9999});
#endif

		return true;
	}
};

class $modify(XEditLevelLayer, EditLevelLayer) {
	void onDeathReplay(CCObject *target) {
		DRSettings::delegate.lvl = DRSettings::levelInstance;

		FLAlertLayer *l = FLAlertLayer::create(&DRSettings::delegate, "Death Replay", "Are you sure you want to <cr>remove</c> Death Replay file for this <cy>level</c>?", "No", "Yes");
		l->show(); 
	}
	
	bool init(GJGameLevel* p0) {
		if (!EditLevelLayer::init(p0)) return false;

		DRSettings::levelInstance = p0;

		CCSprite *btn_spr = CCSprite::createWithSpriteFrameName("backArrowPlain_01_001.png");
		btn_spr->setColor(ccRED);
		btn_spr->setFlipX(true);

		CCMenuItemSpriteExtra *spr_men = CCMenuItemSpriteExtra::create(btn_spr, this, menu_selector(XEditLevelLayer::onDeathReplay));

		CCMenu *menu = CCMenu::create();

		menu->addChild(spr_men);

		CCDirector *dir = CCDirector::sharedDirector();
		auto sz = dir->getWinSize();

		menu->setPosition(65, sz.height - btn_spr->getContentSize().height + 8);

		addChild(menu);

		return true;
	}
};

class AdvancedCCPoint : public cocos2d::CCPoint {
public:
	float rotation;
	float scale;
};

struct PlayerEvent {
	enum PlayerEventEnum {
		ToggleBirdMode = 0,
		ToggleDartMode,
		ToggleFlyMode,
		TogglePlatformerMode,
		TogglePlayerScale,
		ToggleRobotMode,
		ToggleRollMode,
		ToggleSpiderMode,
		ToggleSwingMode,
		ToggleVisibility,
		PlayDeathEffect,
		PlaySpawnEffect
	};

	enum PlayerEventEnum event;

	unsigned char p0;
	unsigned char p1;

	operator PlayerEventEnum() {
		return event;
	};
};

class GhostPlayerFrame {
protected:
	void processEventOnPlayer(struct PlayerEvent event, PlayerObject *obj) {
		log::info("+ Running event with ID {}: ", (int)((PlayerEvent::PlayerEventEnum)event));

		bool success = true;

		switch ((PlayerEvent::PlayerEventEnum)event) {
			case PlayerEvent::ToggleBirdMode: {
				obj->toggleBirdMode((bool)event.p0, (bool)event.p1);
				break;
			}
			case PlayerEvent::ToggleDartMode: {
				obj->toggleDartMode((bool)event.p0, (bool)event.p1);
				break;
			}
			case PlayerEvent::ToggleFlyMode: {
				obj->toggleFlyMode((bool)event.p0, (bool)event.p1);
				break;
			}
			case PlayerEvent::TogglePlatformerMode: {
				obj->togglePlatformerMode((bool)event.p0);
				break;
			}
			case PlayerEvent::TogglePlayerScale: {
				obj->togglePlayerScale((bool)event.p0, (bool)event.p1);
				break;
			}
			case PlayerEvent::ToggleRobotMode: {
				obj->toggleRobotMode((bool)event.p0, (bool)event.p1);
				break;
			}
			case PlayerEvent::ToggleRollMode: {
				obj->toggleRollMode((bool)event.p0, (bool)event.p1);
				break;
			}
			case PlayerEvent::ToggleSpiderMode: {
				obj->toggleSpiderMode((bool)event.p0, (bool)event.p1);
				break;
			}
			case PlayerEvent::ToggleSwingMode: {
				obj->toggleSwingMode((bool)event.p0, (bool)event.p1);
				break;
			}
			case PlayerEvent::ToggleVisibility: {
				obj->toggleVisibility((bool)event.p0);
				break;
			}
			case PlayerEvent::PlayDeathEffect: {
				obj->playDeathEffect();
				obj->m_iconSprite->runAction(CCFadeTo::create(0.2f, 0));
				obj->m_iconSpriteSecondary->runAction(CCFadeTo::create(0.2f, 0));
				break;
			}
			case PlayerEvent::PlaySpawnEffect: {
				obj->playSpawnEffect();
				break;
			}
			default: {
				success = false;
				break;
			}
		}

		if (success) {
			log::info("+ SUCCESS");
		} else {
			log::info("- FAIL");
		}
	}
public:
	AdvancedCCPoint _pos;
	std::vector<struct PlayerEvent> _events;

	void processEventsOnPlayer(PlayerObject *obj) {
		for (auto event : _events) {
			processEventOnPlayer(event, obj);
		}
	}
};

class GhostPosition {
public:
	std::vector<struct GhostPlayerFrame> _players;

	float _delta;
};

class $modify(XPlayLayer, PlayLayer) {
	struct Fields {
		std::vector<CCSprite *> m_deaths = {};
		std::vector<CCNode *> m_nodes = {};
		std::vector<cocos2d::CCParticleSystemQuad *> m_particles;
		std::vector<GhostPosition> m_ghostPosition = {};
		std::vector<GhostPosition> m_currentGhost = {};
		bool m_processGhost = false;
		bool m_processPlaytime = false;

		CCLayer *m_deathLayer = nullptr;

		std::vector<PlayerObject *> m_ghosts = {};

		int m_ghIndex = 0;
		int m_XcurrentAttempt = 1;

		float m_cameraDelta = 0.f;
		float m_cameraPrev = 0.f;
		float m_cameraCur = 0.f;
		CCNode *m_camera1 = nullptr;

		bool m_saveCalled = false;

		bool m_gameBegan = false;

		// std::vector<struct PlayerEvent> m_requestedEvents;
		std::map<int, std::vector<struct PlayerEvent>> m_requestedEvents;
	};

	void offsetGhostBySeconds(float _sec) {
		if (!m_fields->m_processGhost) return;

		float delta = 0.f;

		for (float sec = 0.f; sec < _sec; sec += delta) {
			if (m_fields->m_ghIndex >= m_fields->m_currentGhost.size()) {
				m_fields->m_processGhost = false;
				return;
			}

			GhostPosition pos = m_fields->m_currentGhost[m_fields->m_ghIndex];
			delta = pos._delta;

			m_fields->m_ghIndex++;
		}
	}

	bool verifyLayerIntegrity() {
		CCScene *scene = CCScene::get();
		auto self = scene->getChildByID("PlayLayer");

		return self == this;
	}

	std::vector<PlayerObject *> getAttachablePlayers() {
		bool check = verifyLayerIntegrity();

		if (!check) return {};

		return { m_player1, m_player2 };
	}

	void updatePlaytime(float delta) {
		if (!m_fields->m_gameBegan) return;

		DRSettings::playTime += (double)delta;
		DRSettings::currentAttempt = m_fields->m_XcurrentAttempt;
	}

	void addPlayerPosition(float delta) {
		std::vector<PlayerObject *> players = getAttachablePlayers();

		GhostPosition pos;
		pos._delta = delta;

		int i = 0;

		for (auto pl : players) {
			auto player_pos = pl->getPosition();

			AdvancedCCPoint point;

			point.x = player_pos.x;
			point.y = player_pos.y;

			point.rotation = pl->getRotation();
			point.scale = pl->getScale();

			GhostPlayerFrame _pl;
			_pl._pos = point;

			if (m_fields->m_requestedEvents.count(i)) {
				_pl._events = m_fields->m_requestedEvents[i];
			}

			pos._players.push_back(_pl);

			i++;
		}

		m_fields->m_requestedEvents.clear();

		m_fields->m_ghostPosition.push_back(pos);
	}
	void addPlayerPositionWait(float delta) {
		if (DRSettings::showGhost) {
			m_fields->m_processGhost = true;
		}
		m_fields->m_processPlaytime = true;
	}

	void playGhost(float delta) {
		if (m_fields->m_ghIndex >= m_fields->m_currentGhost.size() || m_isPracticeMode) return;

		// std::vector<PlayerObject *> attachedPlayers = getAttachablePlayers();

		auto frame = m_fields->m_currentGhost[m_fields->m_ghIndex];

		size_t i = 0;
		while (i < frame._players.size() && i < m_fields->m_ghosts.size()) {
			////printf("ghost playback %d at frame %d\n", i, m_fields->m_ghIndex);
			auto player = frame._players[i];
			auto player_pos = player._pos;
			// auto player = attachedPlayers[i];
			auto ghost = m_fields->m_ghosts[i];

			CCPoint pos;

			pos.x = player_pos.x;
			pos.y = player_pos.y;

			ghost->setPosition(pos);
			ghost->setRotation(player_pos.rotation);
			ghost->setScale(player_pos.scale);

			player.processEventsOnPlayer(ghost);

			i++;
		}

		m_fields->m_ghIndex++;
	}

	void setupGhostPlayers() {
		auto pllist = getAttachablePlayers();

		int i = 0;

		i = 0;
		while (i < pllist.size()) {
			if (pllist[i] == nullptr) continue;

			auto po = PlayerObject::create(rand() % 12, rand() % 12, typeinfo_cast<GJBaseGameLayer *>(this), typeinfo_cast<CCLayer *>(this), true);

			po->setSecondColor({(unsigned char)(rand() % 255), (unsigned char)(rand() % 255), (unsigned char)(rand() % 255)});
			po->setColor({(unsigned char)(rand() % 255), (unsigned char)(rand() % 255), (unsigned char)(rand() % 255)});

			// po->unscheduleUpdate();

			CCPoint pos;

			pos.x = pllist[i]->getPositionX();
			pos.y = pllist[i]->getPositionY();

			po->setPosition(pos);
			po->setOpacity(128);

			if (!DRSettings::showGhost || DRSettings::currentlyInPractice) {
				log::info("SETTING GHOST OPACITY TO 0");
				po->setOpacity(0);
			}

			std::string po_id = "ghost-player-" + std::to_string(i);
			po->setID(po_id);

			// idk
			CCNode *mainNode = this->getChildByID("main-node");
			CCNode *batchLayer = mainNode->getChildByID("batch-layer");

			batchLayer->addChild(po, 99);

			m_fields->m_ghosts.push_back(po);

			i++;
		}
	}

	void startGame() {
		geode::log::info("STARTGAME\n");

		if (m_fields->m_ghosts.size() == 0) {
			setupGhostPlayers();
		}

		addPlayerPositionWait(0.f);
		DRSettings::playTime = 0;

		m_fields->m_gameBegan = true;

		PlayLayer::startGame();
	}

	bool init(GJGameLevel *level, bool a, bool b) {
        if (level && level->m_levelLength == 5) DRSettings::inPlatformer = true;

		DRSettings::playDeathEffect = Mod::get()->getSettingValue<bool>("play-death-sound");
		DRSettings::showParticles = Mod::get()->getSettingValue<bool>("show-particles");
		DRSettings::showDeaths = Mod::get()->getSettingValue<bool>("show-deaths");
		DRSettings::showGhost = Mod::get()->getSettingValue<bool>("show-ghost");
		DRSettings::recordPractice = Mod::get()->getSettingValue<bool>("record-practice");
		DRSettings::ghostFPS = 1.f / CCDirector::sharedDirector()->getAnimationInterval();
		DRSettings::playTime = 0;
		DRSettings::currentAttempt = 1;
		// DRSettings::showGhost = false; // TEMP
		DRSettings::withGlobed = Mod::get()->getSettingValue<bool>("record-globed-players");
		DRSettings::actionInstance = this;

		if (level->m_gameVersion <= 21) {
			DRSettings::isOldLevel = true;
		}  else {
			DRSettings::isOldLevel = false;
		}

		m_fields->m_deathLayer = CCLayer::create();

		m_fields->m_deaths.clear();
		m_fields->m_nodes.clear();
		m_fields->m_ghosts.clear();
		m_fields->m_ghostPosition.clear();
		m_fields->m_currentGhost.clear();
		m_fields->m_particles.clear();

		bool res = PlayLayer::init(level, a, b);
		if (!res) return false;

		m_fields->m_camera1 = m_player1;
		
		int id = level->m_levelID.value();

		std::string path = Mod::get()->getSaveDir().generic_string();
		std::string filename = path + "/level_" + std::to_string(id) + ".json";

		if (id == 0) {
			std::string name = level->m_levelName;
			int chk = level->m_chk;
			int rev = level->m_levelRev;
			int sid = level->m_songID;

			filename = path + "/level_C_" + name + std::to_string(chk) + std::to_string(rev) + std::to_string(sid) + ".json";
		}

		if (std::filesystem::exists(filename)) {
			std::ifstream f(filename);

			_progresses = nlohmann::json::parse(f);
		} else {
			_progresses["attempts"] = nlohmann::json::array();
		}

		int i = 0;

		while (i < _progresses["attempts"].size()) {
			_progresses["attempts"][i][2] = 0;

			i++;
		}

		auto object_layer = this->m_objectLayer;

		object_layer->addChild(m_fields->m_deathLayer, 65535);

		setupGhostPlayers();

		// this->schedule(schedule_selector(XPlayLayer::playGhost), 1.f / DRSettings::ghostFPS);

		return true;
	}
	void updateVisibility(float delta) {
		updatePlaytime(delta);

		DRSettings::currentlyInPractice = m_isPracticeMode;

		if (m_fields->m_processGhost) {
			playGhost(delta);
		}

		PlayLayer::updateVisibility(delta);

		if (m_fields->m_processPlaytime && !DRSettings::inPlatformer) {
			m_fields->m_cameraPrev = m_fields->m_cameraCur;
			m_fields->m_cameraCur = m_fields->m_camera1->getPositionX();
			m_fields->m_cameraDelta = m_fields->m_cameraCur - m_fields->m_cameraPrev;
		}

		DRSettings::currentlyInPractice = m_isPracticeMode;

		if (m_fields->m_processGhost) {
			addPlayerPosition(delta);
		}

		int i = 0;

		bool showDeaths = !m_isPracticeMode && DRSettings::showDeaths;

		if (DRSettings::showGhost) {
			for (auto ghost : m_fields->m_ghosts) {
				if (ghost == nullptr) continue;

				if (!DRSettings::showGhost || DRSettings::currentlyInPractice) {
					ghost->setOpacity(0);
				} else {
					ghost->setOpacity(128);
				}
			}
		}

		if (!showDeaths) return;

		if (m_fields->m_nodes.size() > 1024) {
			for (auto node : m_fields->m_nodes) {
				if (node != nullptr) node->removeMeAndCleanup();
			}

			m_fields->m_deaths.clear();
			m_fields->m_nodes.clear();
		}

		while (i < _progresses["attempts"].size()) {
			auto obj = _progresses["attempts"][i];
	
			float x = obj[0].get<float>();
			float y = obj[1].get<float>();
			bool created = obj[2].get<int>();
			double playTime = -1; // obj[3]
			bool playTimeIgnored = DRSettings::inPlatformer;

			// v1.2.0 new parameters
			if (obj.size() >= 4) {
				playTime = obj[3].get<double>();
			}
			if (playTime == -1) playTimeIgnored = true;
			if (DRSettings::isOldLevel) playTimeIgnored = true;

			if (!created && 
				(
					(x <= m_player1->getPositionX())
					||
					((DRSettings::playTime >= playTime) && !playTimeIgnored)
				)
			) {
				_progresses["attempts"][i][2] = 1;

				CCNode *nd = CCNode::create();

				CCSprite *spr = CCSprite::createWithSpriteFrameName("GJ_deleteIcon_001.png");

				spr->setScale(1.3f);

				nd->setPositionX(x);
				nd->setPositionY(y);

				spr->setOpacity(0);
				spr->runAction(CCFadeTo::create(0.2f, 128));
				
				// for (auto ghost : m_fields->m_ghosts) {
					
				// }

				// // if (x == m_fields->m_ghost->getPositionX() && y == m_fields->m_ghost->getPositionY()) {
				// // 	m_fields->m_ghost->runAction(CCFadeOut::create(0.2f));
				// // }

				nd->addChild(spr);

				if(DRSettings::showParticles) {
#ifdef _WIN32
					CCCircleWave *wave = CCCircleWave::create(10.f, 80.f, 0.4f, false, true);	
					// // wave->m_opacityMultiplier = 0.5f;
					wave->m_color = m_player1->getColor();

					nd->addChild(wave);
#endif

#ifndef _WIN32
					spawnParticle("explodeEffect.plist", 0, kCCPositionTypeRelative, {x, y});
#endif
				}

				if(DRSettings::playDeathEffect) {
					FMODAudioEngine *engine = FMODAudioEngine::sharedEngine();
					engine->stopAllEffects();
					engine->playEffect("explode_11.ogg", 1.f, 0.5f, 0.5f);
				}

				m_fields->m_deathLayer->addChild(nd);

				m_fields->m_deaths.push_back(spr);
				m_fields->m_nodes.push_back(nd);
			}

			i++;
		}
	}

	void resetLevel() {
		geode::log::info("RESET LEVEL\n");

		DRSettings::playTime = 0;

		PlayLayer::resetLevel();

		m_fields->m_XcurrentAttempt++;

		size_t i = 0;

		while (i < _progresses["attempts"].size()) {
			_progresses["attempts"][i][2] = 0;

			i++;
		}

		for (auto node : m_fields->m_nodes) {
			node->removeMeAndCleanup();
		}

		m_fields->m_deaths.clear();
		m_fields->m_nodes.clear();
		m_fields->m_particles.clear();

		if (!DRSettings::showGhost) return;

		m_fields->m_currentGhost = m_fields->m_ghostPosition;
		m_fields->m_ghostPosition.clear();
		m_fields->m_ghIndex = 0;

		if (DRSettings::showGhost && m_fields->m_currentGhost.size() > 0) {
			offsetGhostBySeconds(0.3f);
		}

		std::vector<PlayerObject *> attachedPlayers = getAttachablePlayers();

		i = 0;
		while (i < attachedPlayers.size() && i < m_fields->m_ghosts.size()) {
			auto ghost = m_fields->m_ghosts[i];
			auto player = attachedPlayers[i];

			if (ghost != nullptr && player != nullptr) {
				ghost->setPosition(player->getPosition());
				ghost->setRotation(player->getRotation());
			}

			i++;
		}
	}
};

class $modify(PlayerObject) {
	void playerDestroyed(bool p0) {
		log::info("PLAYER DESTROYED WITH P0={}", p0);

		PlayerObject::playerDestroyed(p0);

		if (DRSettings::currentlyInPractice && !DRSettings::recordPractice) return PlayerObject::playDeathEffect();
		
		nlohmann::json attempt = nlohmann::json::array();

		auto pl = PlayLayer::get();

		if (!DRSettings::withGlobed) {
			if (pl->m_player1 == this || pl->m_player2 == this) {
				// do nothing
			} else {
				return PlayerObject::playerDestroyed(p0);
			}
		}

		attempt.push_back((float)getPositionX());
		attempt.push_back((float)getPositionY());
		attempt.push_back((int)1);

		if (!DRSettings::inPlatformer) attempt.push_back((double)DRSettings::playTime);

		//printf("added death notif with: %f %f %d %f\n", getPositionX(), getPositionY(), 1, (float)DRSettings::playTime);

		_progresses["attempts"].push_back(attempt);

		int id = pl->m_level->m_levelID.value();

		std::string filename = generateFilename(pl->m_level);

		std::ofstream o(filename);

		o << _progresses;

		o.close();
	}

	void pushEvent(struct PlayerEvent ev) {
		if (DRSettings::actionInstance == nullptr) return;

		std::map<PlayerObject *, int> m;
		std::vector<PlayerObject *> pl = DRSettings::actionInstance->getAttachablePlayers();

		for (int i = 0; i < pl.size(); i++) {
			m[pl[i]] = i;
		}

		if (!m.count(this)) return;
		if (!DRSettings::actionInstance->m_fields->m_requestedEvents.count(m[this])) {
			DRSettings::actionInstance->m_fields->m_requestedEvents[m[this]] = {};
		}

		DRSettings::actionInstance->m_fields->m_requestedEvents[m[this]].push_back(ev);
	}

	void playDeathEffect() {
		pushEvent({PlayerEvent::PlayDeathEffect});

		PlayerObject::playDeathEffect();
	}

	void toggleBirdMode(bool p0, bool p1) {
		PlayerObject::toggleBirdMode(p0, p1);

		pushEvent({PlayerEvent::ToggleBirdMode, p0, p1});
	}
	void toggleDartMode(bool p0, bool p1) {
		PlayerObject::toggleDartMode(p0, p1);

		pushEvent({PlayerEvent::ToggleDartMode, p0, p1});
	}
	void toggleFlyMode(bool p0, bool p1) {
		PlayerObject::toggleFlyMode(p0, p1);

		pushEvent({PlayerEvent::ToggleFlyMode, p0, p1});
	}
	void togglePlatformerMode(bool p0) {
		PlayerObject::togglePlatformerMode(p0);

		pushEvent({PlayerEvent::TogglePlatformerMode, p0});
	}
	void togglePlayerScale(bool p0, bool p1) {
		PlayerObject::togglePlayerScale(p0, p1);

		pushEvent({PlayerEvent::TogglePlayerScale, p0, p1});
	}
	void toggleRobotMode(bool p0, bool p1) {
		PlayerObject::toggleRobotMode(p0, p1);

		pushEvent({PlayerEvent::ToggleRobotMode, p0, p1});
	}
	void toggleRollMode(bool p0, bool p1) {
		PlayerObject::toggleRollMode(p0, p1);

		pushEvent({PlayerEvent::ToggleRollMode, p0, p1});
	}
	void toggleSpiderMode(bool p0, bool p1) {
		PlayerObject::toggleSpiderMode(p0, p1);

		pushEvent({PlayerEvent::ToggleSpiderMode, p0, p1});
	}
	void toggleSwingMode(bool p0, bool p1) {
		PlayerObject::toggleSwingMode(p0, p1);

		pushEvent({PlayerEvent::ToggleSwingMode, p0, p1});
	}
	void toggleVisibility(bool p0) {
		PlayerObject::toggleVisibility(p0);

		pushEvent({PlayerEvent::ToggleVisibility, p0});
	}
	void playSpawnEffect() {
		pushEvent({PlayerEvent::PlaySpawnEffect});

		PlayerObject::playSpawnEffect();
	}
};