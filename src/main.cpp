// clang-format off

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/EditLevelLayer.hpp>

#include <Geode/binding/FLAlertLayer.hpp>

#include "json/single_include/nlohmann/json.hpp"

#include <Geode/cocos/actions/CCActionInterval.h>

#include <filesystem>

#include <vector>

#include <Geode/binding/CCCircleWave.hpp>
#include <Geode/cocos/particle_nodes/CCParticleSystemQuad.h>
#include <Geode/binding/FMODAudioEngine.hpp>

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

namespace DMSettings {
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
	PlayLayer *actionInstance;
}

class $modify(XLevelInfoLayer, LevelInfoLayer) {
	static void onModify(auto & self)
	{
		(void) self.setHookPriority("LevelInfoLayer::init", INT32_MIN + 1);
	}
	void onDeathReplay(CCObject *target) {
		DMSettings::delegate.lvl = DMSettings::levelInstance;

		FLAlertLayer *l = FLAlertLayer::create(&DMSettings::delegate, "Death Replay", "Are you sure you want to <cr>remove</c> the Death Replay file for this <cy>level</c>?", "No", "Yes");
		l->show(); 
	}

	bool init(GJGameLevel *lvl, bool idk) {
		if (!LevelInfoLayer::init(lvl, idk)) { return false; }

		DMSettings::levelInstance = lvl;

		CCSprite *btn_spr = CCSprite::createWithSpriteFrameName("backArrowPlain_01_001.png");
		btn_spr->setColor(ccRED);
		btn_spr->setFlipX(true);

		CCMenuItemSpriteExtra *spr_men = CCMenuItemSpriteExtra::create(btn_spr, this, menu_selector(XLevelInfoLayer::onDeathReplay));
		spr_men->setID("remove-file"_spr);

		getChildByID("back-menu")->addChild(spr_men);
		getChildByID("back-menu")->updateLayout();

		return true;
	}
};

class $modify(XEditLevelLayer, EditLevelLayer) {
	void onDeathReplay(CCObject *target) {
		DMSettings::delegate.lvl = DMSettings::levelInstance;

		FLAlertLayer *l = FLAlertLayer::create(&DMSettings::delegate, "Death Replay", "Are you sure you want to <cr>remove</c> Death Replay file for this <cy>level</c>?", "No", "Yes");
		l->show(); 
	}
	
	bool init(GJGameLevel* p0) {
		if (!EditLevelLayer::init(p0)) return false;

		DMSettings::levelInstance = p0;

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

class $modify(PlayerObject) {
	void playDeathEffect() {
		if (DMSettings::currentlyInPractice && !DMSettings::recordPractice) return PlayerObject::playDeathEffect();
		
		nlohmann::json attempt = nlohmann::json::array();

		auto pl = PlayLayer::get();

		if (!DMSettings::withGlobed) {
			if (pl->m_player1 == this || pl->m_player2 == this) {
				// do nothing
			} else {
				return PlayerObject::playDeathEffect();
			}
		}

		double extra = (DMSettings::currentAttempt == 1);
		extra = 0;

		attempt.push_back((float)getPositionX());
		attempt.push_back((float)getPositionY());
		attempt.push_back((int)1);

		if (!DMSettings::inPlatformer) attempt.push_back((double)DMSettings::playTime + extra);

		//printf("added death notif with: %f %f %d %f\n", getPositionX(), getPositionY(), 1, (float)DMSettings::playTime);

		_progresses["attempts"].push_back(attempt);

		int id = pl->m_level->m_levelID.value();

		std::string filename = generateFilename(pl->m_level);

		std::ofstream o(filename);

		o << _progresses;

		o.close();

		PlayerObject::playDeathEffect();
	}
};

class AdvancedCCPoint : public cocos2d::CCPoint {
public:
	float rotation;
	float scale;
};

// class GhostPlayer {
// 	std::vector<AdvancedCCPoint> _recordedPosition;
// };

class GhostPosition {
public:
	std::vector<AdvancedCCPoint> _players;
};

class $modify(XPlayLayer, PlayLayer) {
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

	std::vector<PlayerObject *> getAttachablePlayers() {
		return { m_player1, m_player2 };
	}

	void updatePlaytime(float delta) {
		DMSettings::playTime += (double)delta;
		DMSettings::currentAttempt = m_fields->m_XcurrentAttempt;
	}

	void addPlayerPosition(float delta) {
		std::vector<PlayerObject *> players = getAttachablePlayers();

		GhostPosition pos;

		for (auto pl : players) {
			auto player_pos = pl->getPosition();

			AdvancedCCPoint point;

			point.x = player_pos.x;
			point.y = player_pos.y;

			point.rotation = pl->getRotation();
			point.scale = pl->getScale();

			pos._players.push_back(point);
		}

		m_fields->m_ghostPosition.push_back(pos);
	}
	void addPlayerPositionWait(float delta) {
		if (DMSettings::showGhost) {
			m_fields->m_processGhost = true;
		}
		m_fields->m_processPlaytime = true;
		DMSettings::playTime = 1;
	}

	void playGhost(float delta) {
		if (m_fields->m_ghIndex >= m_fields->m_currentGhost.size() || m_isPracticeMode) return;

		// std::vector<PlayerObject *> attachedPlayers = getAttachablePlayers();

		auto frame = m_fields->m_currentGhost[m_fields->m_ghIndex];

		size_t i = 0;
		while (i < frame._players.size() && i < m_fields->m_ghosts.size()) {
			////printf("ghost playback %d at frame %d\n", i, m_fields->m_ghIndex);
			auto player_pos = frame._players[i];
			// auto player = attachedPlayers[i];
			auto ghost = m_fields->m_ghosts[i];

			CCPoint pos;

			pos.x = player_pos.x;
			pos.y = player_pos.y;

			ghost->setPosition(pos);
			ghost->setRotation(player_pos.rotation);
			ghost->setScale(player_pos.scale);

			i++;
		}

		m_fields->m_ghIndex++;
	}

	bool init(GJGameLevel *level, bool a, bool b) {
        if (level && level->m_levelLength == 5) DMSettings::inPlatformer = true;

		DMSettings::playDeathEffect = Mod::get()->getSettingValue<bool>("play-death-sound");
		DMSettings::showParticles = Mod::get()->getSettingValue<bool>("show-particles");
		DMSettings::showDeaths = Mod::get()->getSettingValue<bool>("show-deaths");
		DMSettings::showGhost = Mod::get()->getSettingValue<bool>("show-ghost");
		DMSettings::recordPractice = Mod::get()->getSettingValue<bool>("record-practice");
		DMSettings::ghostFPS = 1.f / CCDirector::sharedDirector()->getAnimationInterval();
		DMSettings::playTime = 0;
		DMSettings::currentAttempt = 1;
		// TEMP
		DMSettings::showGhost = false;
		DMSettings::withGlobed = Mod::get()->getSettingValue<bool>("record-globed-players");
		DMSettings::actionInstance = this;

		if (level->m_gameVersion <= 21) {
			DMSettings::isOldLevel = true;
		}  else {
			DMSettings::isOldLevel = false;
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

		auto pllist = getAttachablePlayers();

		i = 0;
		while (i < pllist.size()) {
			if (pllist[i] == nullptr) continue;

			auto po = PlayerObject::create(rand() % 12, rand() % 12, dynamic_cast<GJBaseGameLayer *>(this), dynamic_cast<CCLayer *>(this), true);

			po->setSecondColor({(unsigned char)(rand() % 255), (unsigned char)(rand() % 255), (unsigned char)(rand() % 255)});
			po->setColor({(unsigned char)(rand() % 255), (unsigned char)(rand() % 255), (unsigned char)(rand() % 255)});
		
			CCPoint pos;

			pos.x = pllist[i]->getPositionX();
			pos.y = pllist[i]->getPositionY();

			po->setPosition(pos);
			po->setOpacity(128);

			if (!DMSettings::showGhost || DMSettings::currentlyInPractice) {
				po->setOpacity(0);
			}	

			// idk
			// m_batchNodePlayer->addChild(po);

			m_fields->m_ghosts.push_back(po);

			i++;
		}

		if (m_fields->m_XcurrentAttempt == 1) {
			this->schedule(schedule_selector(XPlayLayer::addPlayerPositionWait), 1.f, false, 1.f);
		} else {
			addPlayerPositionWait(0.f);
			DMSettings::playTime = 0;
		}

		// this->schedule(schedule_selector(XPlayLayer::playGhost), 1.f / DMSettings::ghostFPS);

		return true;
	}
	void updateVisibility(float delta) {
		updatePlaytime(delta);

		DMSettings::currentlyInPractice = m_isPracticeMode;

		if (m_fields->m_processGhost) {
			playGhost(delta);
		}

		PlayLayer::updateVisibility(delta);

		if (m_fields->m_processPlaytime && !DMSettings::inPlatformer) {
			m_fields->m_cameraPrev = m_fields->m_cameraCur;
			m_fields->m_cameraCur = m_fields->m_camera1->getPositionX();
			m_fields->m_cameraDelta = m_fields->m_cameraCur - m_fields->m_cameraPrev;
		}

		DMSettings::currentlyInPractice = m_isPracticeMode;

		if (m_fields->m_processGhost) {
			addPlayerPosition(delta);
		}

		int i = 0;

		bool showDeaths = !m_isPracticeMode && DMSettings::showDeaths;

		if (DMSettings::showGhost) {
			for (auto ghost : m_fields->m_ghosts) {
				if (ghost == nullptr) continue;

				if (!DMSettings::showGhost || DMSettings::currentlyInPractice) {
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
			bool playTimeIgnored = DMSettings::inPlatformer;

			// v1.2.0 new parameters
			if (obj.size() >= 4) {
				playTime = obj[3].get<double>();
			}
			if (playTime == -1) playTimeIgnored = true;
			if (DMSettings::isOldLevel) playTimeIgnored = true;

			if (!created && 
				(
					(x <= m_player1->getPositionX())
					||
					((DMSettings::playTime >= playTime) && !playTimeIgnored)
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

				if(DMSettings::showParticles) {
					CCCircleWave *wave = CCCircleWave::create(10.f, 80.f, 0.4f, false, true);	
					// // wave->m_opacityMultiplier = 0.5f;
					wave->m_color = m_player1->getColor();

					nd->addChild(wave);

					spawnParticle("explodeEffect.plist", 0, kCCPositionTypeRelative, {x, y});
				}

				if(DMSettings::playDeathEffect) {
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
		DMSettings::playTime = 0;

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

		if (!DMSettings::showGhost) return;

		m_fields->m_currentGhost = m_fields->m_ghostPosition;
		m_fields->m_ghostPosition.clear();
		m_fields->m_ghIndex = 0;

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