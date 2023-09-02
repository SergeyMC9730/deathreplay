// clang-format off

/**
 * Include the Geode headers.
 */
#include <Geode/Geode.hpp>
/**
 * Required to modify the MenuLayer class
 */
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <Geode/binding/FLAlertLayer.hpp>

#include "json/single_include/nlohmann/json.hpp"

#include <Geode/cocos/actions/CCActionInterval.h>

#include <filesystem>

#include <vector>

#include <Geode/binding/CCCircleWave.hpp>
#include <Geode/cocos/particle_nodes/CCParticleSystemQuad.h>
#include <Geode/binding/GameSoundManager.hpp>

using namespace geode::prelude;
// using namespace geode::modifier;

nlohmann::json _progresses;

namespace DMSettings {
	bool showDeaths = true;
	bool showParticles = true;
	bool playDeathEffect = true;
	int maxDeaths = 1024;
}

class $modify(PlayerObject) {
	void playDeathEffect() {
		nlohmann::json attempt = nlohmann::json::array();

		auto pl = PlayLayer::get();

		float percentage = getPositionX() / pl->m_realLevelLength * 100.f;

		attempt.push_back((float)getPositionX());
		attempt.push_back((float)getPositionY());
		attempt.push_back((int)1);

		_progresses["attempts"].push_back(attempt);

		int id = pl->m_level->m_levelID.value();

		std::string filename = "level_" + std::to_string(id) + ".json";

		if (id == 0) {
			std::string name = pl->m_level->m_levelName;
			int chk = pl->m_level->m_chk;
			int rev = pl->m_level->m_levelRev;
			int sid = pl->m_level->m_songID;

			filename = "level_C_" + name + std::to_string(chk) + std::to_string(rev) + std::to_string(sid) + ".json";
		}

		std::ofstream o(filename);

		o << _progresses;

		o.close();

		PlayerObject::playDeathEffect();
	}
};

class $modify(PlayLayer) {
	CCLayer *m_deathLayer;
	std::vector<CCSprite *> m_deaths;
	std::vector<CCNode *> m_nodes;

	bool init(GJGameLevel *level) {
		DMSettings::playDeathEffect = Mod::get()->getSettingValue<bool>("play-death-sound");
		DMSettings::showParticles = Mod::get()->getSettingValue<bool>("show-particles");
		DMSettings::showDeaths = Mod::get()->getSettingValue<bool>("show-deaths");

		printf("settings:\n - play death sound %d\n - show particles %d\n - show deaths %d\n", DMSettings::playDeathEffect, DMSettings::showParticles, DMSettings::showDeaths);

		m_fields->m_deaths.clear();
		m_fields->m_nodes.clear();

		m_fields->m_deathLayer = nullptr;
		
		int id = level->m_levelID.value();

		std::string filename = "level_" + std::to_string(id) + ".json";

		if (id == 0) {
			std::string name = level->m_levelName;
			int chk = level->m_chk;
			int rev = level->m_levelRev;
			int sid = level->m_songID;

			filename = "level_C_" + name + std::to_string(chk) + std::to_string(rev) + std::to_string(sid) + ".json";
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

		bool res = PlayLayer::init(level);
		if (!res) return false;

		m_fields->m_deathLayer = CCLayer::create();

		this->sortAllChildren();

		auto camera_layer = dynamic_cast<CCNode *>(this->getChildren()->objectAtIndex(3));

		camera_layer->addChild(m_fields->m_deathLayer, 65535);

		return true;
	}
	void update(float delta) {
		PlayLayer::update(delta);

		int i = 0;

		bool showDeaths = !m_isPracticeMode && DMSettings::showDeaths;

		if (!showDeaths) return;

		if (m_fields->m_nodes.size() > 1024) {
			for (auto node : m_fields->m_nodes) {
				node->removeMeAndCleanup();
			}

			m_fields->m_deaths.clear();
			m_fields->m_nodes.clear();
		}

		while (i < _progresses["attempts"].size()) {
			auto obj = _progresses["attempts"][i];
	
			bool created = obj[2].get<int>();
			float x = obj[0].get<float>();
			float y = obj[1].get<float>();

			if (!created && (x <= m_player1->getPositionX())) {
				_progresses["attempts"][i][2] = 1;

				CCNode *nd = CCNode::create();

				CCSprite *spr = CCSprite::createWithSpriteFrameName("GJ_deleteIcon_001.png");

				spr->setScale(1.3f);
				nd->setPositionX(x);
				nd->setPositionY(y);

				spr->setOpacity(0);
				spr->runAction(CCFadeIn::create(0.2f));

				nd->addChild(spr);

				if(DMSettings::showParticles) {
					CCCircleWave *wave = CCCircleWave::create(10.f, 80.f, 0.4f, false, true);	
					// wave->m_opacityMultiplier = 0.5f;
					wave->m_color = m_player1->m_playerColor1;

					nd->addChild(wave);

					auto particle = CCParticleSystemQuad::create("explodeEffect.plist");
					particle->setPositionX(0.f);
					particle->setPositionY(0.f);

					nd->addChild(particle);
				}

				if(DMSettings::playDeathEffect) {
					GameSoundManager::get()->playEffect("explode_11.ogg", 0.5f, 0.5f, 0.5f);
				}

				m_fields->m_deathLayer->addChild(nd);

				m_fields->m_deaths.push_back(spr);
				m_fields->m_nodes.push_back(nd);
			}

			i++;
		}
	}

	void resetLevel() {
		int i = 0;

		while (i < _progresses["attempts"].size()) {
			_progresses["attempts"][i][2] = 0;

			i++;
		}

		for (auto node : m_fields->m_nodes) {
			node->removeMeAndCleanup();
		}

		m_fields->m_deaths.clear();
		m_fields->m_nodes.clear();

		PlayLayer::resetLevel();
	}
};