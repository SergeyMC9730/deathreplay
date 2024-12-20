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

	bool debugMode = false;

	DRDelegate delegate;
	GJGameLevel *levelInstance;
	XPlayLayer *actionInstance;

	double ghostOffset;
	double xOpacity;
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

	template<typename T>
	std::vector<T *> findInstancesOfObj(cocos2d::CCNode *node) {
		CCArray *children = node->getChildren();

		std::vector<T *> objs = {};

		for (int i = 0; i < children->count(); i++) {
			cocos2d::CCObject *_obj = children->objectAtIndex(i);

			T *dyn = typeinfo_cast<T *>(_obj);

			if (dyn != nullptr) {
				objs.push_back(dyn);
			}
		}

		return objs;
	}

	bool deadPlayerIsOffline = true;
};

class DrPercentageBar : public CCNode {
private:
	CCSprite* _spr;
public:
	void update(float delta) override {

	}

	bool init(int p, float maxHeight) {
		CCSprite* square = CCSprite::create("square.png");
		square->setAnchorPoint({ 0, 0 });

		float r = (float)p / 100.f;
		float scaleY = (maxHeight * r) / (float)square->getContentHeight();
		float scaleX = 0.3f;

		square->setScaleX(scaleX);

		setContentHeight(maxHeight);
		setContentWidth(square->getContentWidth() * scaleX);

		square->runAction(CCEaseInOut::create(CCScaleTo::create(0.5f, scaleX, scaleY), 2.f));

		addChild(square);

		_spr = square;

		return true;
	}

	void setBarColor(const ccColor3B &col){
		_spr->setColor(col);
	}
	void setBarOpacity(GLubyte opacity) {
		_spr->setOpacity(opacity);
	}

	static DrPercentageBar* create(int p, float maxHeight) {
		DrPercentageBar* pRet = new DrPercentageBar();
		if (pRet && pRet->init(p, maxHeight)) {
			pRet->autorelease();
			return pRet;
		}
		else {
			delete pRet;
			pRet = 0;
			return 0;
		}
	}
};

class DrInfoPopup : public FLAlertLayer {
public:
	void onExitButton(CCObject* sender) {
		removeMeAndCleanup();
	}

	void update(float delta) override {

	}

	bool init() override {
		if (!FLAlertLayer::init(0)) return false;

		CCLayer* objectSelector = CCLayer::create();

		CCScale9Sprite* spr1 = CCScale9Sprite::create("GJ_square05.png");
		auto winsize = CCDirector::sharedDirector()->getWinSize();

		spr1->setContentSize({ 300, 160 });
		spr1->setAnchorPoint({ 0, 0 });

		objectSelector->setPosition({ winsize.width / 2 - spr1->getContentWidth() / 2, winsize.height / 2 - spr1->getContentHeight() / 2});
		objectSelector->setContentSize(spr1->getContentSize());

		objectSelector->addChild(spr1);

		std::string title = DRSettings::levelInstance->m_levelName;

		CCLabelBMFont* bmf = CCLabelBMFont::create(title.c_str(), "bigFont.fnt");
		bmf->setScale(0.7f);
		bmf->setPositionX(spr1->getContentWidth() / 2);
		bmf->setPositionY(spr1->getContentSize().height - 15);

		objectSelector->addChild(bmf, 1);

		auto exitBtn = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
		auto btn3 = CCMenuItemSpriteExtra::create(
			exitBtn, this, menu_selector(DrInfoPopup::onExitButton)
		);
		btn3->setPosition(0, spr1->getContentHeight());

		CCMenu* men2 = CCMenu::create();
		men2->setPosition({
			0, 0
		});
		men2->addChild(btn3);

		objectSelector->addChild(men2, 2);

		m_mainLayer->addChild(objectSelector);

		auto base = CCSprite::create("square.png");
		base->setPosition({ 0, 0 });
		base->setScale(500.f);
		base->setColor({ 0, 0, 0 });
		base->setOpacity(0);
		base->runAction(CCFadeTo::create(0.3f, 125));

		this->addChild(base, -1);

		auto bars = CCLayer::create();
		RowLayout* layout = RowLayout::create();

		layout->setGrowCrossAxis(false);
		layout->setCrossAxisOverflow(false);
		layout->setAutoScale(true);
		layout->setAxisAlignment(AxisAlignment::Start);
		layout->setCrossAxisAlignment(AxisAlignment::Start);
		layout->setCrossAxisLineAlignment(AxisAlignment::Start);
		layout->setGap(0.f);

		int spaceX = 60;
		int spaceY = 40;

		bars->setLayout(layout);
		bars->setContentWidth(objectSelector->getContentWidth() - spaceX);
		bars->setContentHeight(objectSelector->getContentHeight() - spaceY);
		bars->setPosition(spaceX / 2, 10);
		bars->setAnchorPoint({ 0, 0 });

		for (int i = 0; i < 100; i++) {
			auto bar = DrPercentageBar::create(rand() % 100, bars->getContentHeight());

			if ((i % 2) == 0) {
				bar->setBarOpacity(200);
			}
			else {
				bar->setBarOpacity(128);
			}

			bars->addChild(bar);
		}

		bars->updateLayout();

		objectSelector->addChild(bars);

		CCDrawNode* drawing = CCDrawNode::create();
		drawing->setContentSize(objectSelector->getContentSize());
		drawing->setAnchorPoint({ 0, 0 });
		CCRect r = { {0, 0}, drawing->getContentSize() };
		drawing->enableDrawArea(r);

		CCPoint p1 = { 0, 0 };
		CCPoint p2 = { 50, 50 };

		drawing->drawRect(p1, p2, {255, 255, 255, 255}, 3.f, {255, 0, 0, 255});

		objectSelector->addChild(drawing);

		show();

		scheduleUpdate();

		return true;
	}
	void registerWithTouchDispatcher() override {
		CCTouchDispatcher* dispatcher = cocos2d::CCDirector::sharedDirector()->getTouchDispatcher();

		dispatcher->addTargetedDelegate(this, -502, true);
	}
	static DrInfoPopup* create() {
		DrInfoPopup* pRet = new DrInfoPopup();
		if (pRet && pRet->init()) {
			pRet->autorelease();
			return pRet;
		}
		else {
			delete pRet;
			pRet = 0;
			return 0;
		}
	}
};

class $modify(XLevelInfoLayer, LevelInfoLayer) {
	static void onModify(auto & self) {
		(void) self.setHookPriority("LevelInfoLayer::init", INT32_MIN + 1);
	}
	void onDeathReplay(CCObject *target) {
		if (!DRSettings::debugMode) {
			DRSettings::delegate.lvl = DRSettings::levelInstance;

			FLAlertLayer *l = FLAlertLayer::create(&DRSettings::delegate, "Death Replay", "Are you sure you want to <cr>remove</c> the Death Replay file for this <cy>level</c>?", "No", "Yes");
			l->show();

			return;
		}

		int id = DRSettings::levelInstance->m_levelID;
		auto level = DRSettings::levelInstance;

		std::string path = Mod::get()->getSaveDir().generic_string();
		std::string filename = path + "/level_" + std::to_string(id) + ".json";

		if (id == 0) {
			std::string name = level->m_levelName;
			int chk = level->m_chk;
			int rev = level->m_levelRev;
			int sid = level->m_songID;

			filename = path + "/level_C_" + name + std::to_string(chk) + std::to_string(rev) + std::to_string(sid) + ".json";
		}

		if (!std::filesystem::exists(filename)) {
			FLAlertLayer::create("Death Replay", "<cr>No information available</c> about this level.", "OK")->show();

			return;
		}

		std::ifstream f(filename);

		_progresses = nlohmann::json::parse(f);

		f.close();

		DrInfoPopup* popup = DrInfoPopup::create();
	}

	bool init(GJGameLevel *lvl, bool idk) {
		if (!LevelInfoLayer::init(lvl, idk)) { return false; }

		DRSettings::levelInstance = lvl;

		
		CCSprite* btn_spr;
		if (DRSettings::debugMode) {
			btn_spr = CCSprite::create("DR_infoIcon_001.png"_spr);
			btn_spr->setScale(1.4f);
		}
		else {
			btn_spr = CCSprite::createWithSpriteFrameName("backArrowPlain_01_001.png");
			btn_spr->setColor(ccRED);
			btn_spr->setFlipX(true);
		}

		CCMenuItemSpriteExtra *spr_men = CCMenuItemSpriteExtra::create(btn_spr, this, menu_selector(XLevelInfoLayer::onDeathReplay));
		spr_men->setID("dr-remove-file");

		CCNode* label = getChildByID("title-label");
		CCMenu* drMenu = CCMenu::create();
		drMenu->setPosition({ 0, 0 });

		spr_men->setPosition({
			label->getPosition().x - ((label->getContentWidth() / 2) * label->getScale()) - ((btn_spr->getContentWidth() / 2) * btn_spr->getScale()) - 5.f,
			label->getPosition().y
		});
		spr_men->setAnchorPoint({ 0.35f, 0.6f });

		drMenu->setID("dr-button-menu");

		drMenu->addChild(spr_men);
		addChild(drMenu);

		return true;
	}
};

class $modify(XLevelSelectLayer, LevelSelectLayer) {
	int detectLevelID() {
		int level_id = -1;
		
		auto batch = getChildByIDRecursive("page-buttons");
		if (batch == nullptr) return level_id;
		
		CCArray *ch = batch->getChildren();
		if (ch == nullptr) return level_id;

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
		TogglePlayerScale,
		ToggleRobotMode,
		ToggleRollMode,
		ToggleSpiderMode,
		ToggleSwingMode,
		ToggleVisibility,
		PlayDeathEffect,
		PlaySpawnEffect,
		StopDashing,
		StartDashing,
		SpiderDashEffect,
		SpiderRun
	};

	enum PlayerEventEnum event;

	using Argument = std::variant<unsigned char, DashRingObject*, CCPoint>;

	Argument _p0 = (unsigned char)0;
	Argument _p1 = (unsigned char)0;

	operator PlayerEventEnum() const {
		return event;
	};

	inline unsigned char p0c() const {
		return std::get<unsigned char>(_p0);
	}
	inline DashRingObject* p0d() const {
		return std::get<DashRingObject*>(_p0);
	}
	inline CCPoint p0p() const {
		return std::get<CCPoint>(_p0);
	}

	inline unsigned char p1c() const {
		return std::get<unsigned char>(_p1);
	}
	inline DashRingObject* p1d() const {
		return std::get<DashRingObject*>(_p1);
	}
	inline CCPoint p1p() const {
		return std::get<CCPoint>(_p1);
	}
};

class GhostPlayerFrame {
protected:
	void processEventOnPlayer(const struct PlayerEvent &event, PlayerObject *obj) {
		if (DRSettings::debugMode) log::info("+ Running event with ID {}: ", (int)((PlayerEvent::PlayerEventEnum)event));

		// bool success = true;

		switch ((PlayerEvent::PlayerEventEnum)event) {
			case PlayerEvent::ToggleBirdMode: {
				obj->toggleBirdMode((bool)event.p0c(), (bool)event.p1c());
				break;
			}
			case PlayerEvent::ToggleDartMode: {
				obj->toggleDartMode((bool)event.p0c(), (bool)event.p1c());
				break;
			}
			case PlayerEvent::ToggleFlyMode: {
				obj->toggleFlyMode((bool)event.p0c(), (bool)event.p1c());
				break;
			}
			case PlayerEvent::TogglePlayerScale: {
				obj->togglePlayerScale((bool)event.p0c(), (bool)event.p1c());
				break;
			}
			case PlayerEvent::ToggleRobotMode: {
				obj->toggleRobotMode((bool)event.p0c(), (bool)event.p1c());
				break;
			}
			case PlayerEvent::ToggleRollMode: {
				obj->toggleRollMode((bool)event.p0c(), (bool)event.p1c());
				break;
			}
			case PlayerEvent::ToggleSpiderMode: {
				obj->toggleSpiderMode((bool)event.p0c(), (bool)event.p1c());
				break;
			}
			case PlayerEvent::ToggleSwingMode: {
				obj->toggleSwingMode((bool)event.p0c(), (bool)event.p1c());
				break;
			}
			case PlayerEvent::ToggleVisibility: {
				obj->toggleVisibility((bool)event.p0c());
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
			case PlayerEvent::StartDashing: {
				obj->startDashing(event.p0d());
				break;
			}
			case PlayerEvent::StopDashing: {
				obj->stopDashing();
				break;
			}
			case PlayerEvent::SpiderDashEffect: {
				obj->playSpiderDashEffect(event.p0p(), event.p1p());
				break;
			}
			case PlayerEvent::SpiderRun: {
				obj->playDynamicSpiderRun();
				break;
			}
			default: {
				// success = false;
				break;
			}
		}

		// if (success) {
		// 	if (DRSettings::debugMode) log::info("+ SUCCESS");
		// } else {
		// 	if (DRSettings::debugMode) log::info("- FAIL");
		// }
	}
public:
	AdvancedCCPoint _pos;
	std::vector<struct PlayerEvent> _events;

	void processEventsOnPlayer(PlayerObject *obj) {
		for (const auto &event : _events) {
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
		bool m_shouldDelayGhost = false;

		bool m_shouldSendGamemodeInfo = false;

		// std::vector<struct PlayerEvent> m_requestedEvents;
		std::map<int, std::vector<struct PlayerEvent>> m_requestedEvents;
	};

	void offsetGhostBySeconds(double _sec) {
		if (!m_fields->m_processGhost) return;

		if (_sec <= 0.f) return;

		float delta = 0.f;

		for (double sec = 0.f; sec < _sec; sec += delta) {
			if (m_fields->m_ghIndex >= m_fields->m_currentGhost.size()) {
				m_fields->m_processGhost = false;
				return;
			}

			GhostPosition frame = m_fields->m_currentGhost[m_fields->m_ghIndex];
			if (frame._delta != 0.f) {
				delta = frame._delta;
			};

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

	void pushEvent(PlayerObject *obj, struct PlayerEvent ev) {
		if (!DRSettings::actionInstance || !PlayLayer::get() || !obj) return;

		std::map<PlayerObject*, int> m;
		std::vector<PlayerObject*> pl = DRSettings::actionInstance->getAttachablePlayers();

		for (int i = 0; i < pl.size(); i++) {
			m[pl[i]] = i;
		}

		if (!m.count(obj)) return;
		if (!DRSettings::actionInstance->m_fields->m_requestedEvents.count(m[obj])) {
			DRSettings::actionInstance->m_fields->m_requestedEvents[m[obj]] = {};
		}

		DRSettings::actionInstance->m_fields->m_requestedEvents[m[obj]].push_back(ev);
	}

	void setupPlayerInfoInFrame() {
		if (DRSettings::debugMode) {
			log::info("setting up player info");
		}

		std::vector<PlayerObject*> players = getAttachablePlayers();

		for (auto pl : players) {
			if (!pl) continue;

			unsigned char p0 = 1, p1 = 1;

			if (pl->m_isShip) pushEvent(pl, { PlayerEvent::ToggleFlyMode, p0, p1 });
			else if (pl->m_isBird) pushEvent(pl, { PlayerEvent::ToggleBirdMode, p0, p1 });
			else if (pl->m_isBall) pushEvent(pl, { PlayerEvent::ToggleRollMode, p0, p1 });
			else if (pl->m_isDart) pushEvent(pl, { PlayerEvent::ToggleDartMode, p0, p1 });
			else if (pl->m_isRobot) pushEvent(pl, { PlayerEvent::ToggleRobotMode, p0, p1 });
			else if (pl->m_isSpider) pushEvent(pl, { PlayerEvent::ToggleSpiderMode, p0, p1 });
			else if (pl->m_isSwing) pushEvent(pl, { PlayerEvent::ToggleSwingMode, p0, p1 });
			if (pl->m_isDashing) pushEvent(pl, { PlayerEvent::StartDashing, pl->m_dashRing, p1 });
		}
	}

	void addPlayerPosition(float delta) {
		// log::info("recording!");

		if (m_fields->m_shouldSendGamemodeInfo) {
			m_fields->m_shouldSendGamemodeInfo = false;

			setupPlayerInfoInFrame();
		}

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
		if (DRSettings::debugMode) log::info("addPlayerPositionWait({});", delta);

		if (DRSettings::showGhost) {
			m_fields->m_processGhost = true;
		}

		setupPlayerInfoInFrame();
	}

	void playGhost(float delta) {
		if (!m_fields->m_processGhost || m_fields->m_shouldDelayGhost) return;
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

	CCNode *findBatchNode() {
		auto shaders = DRGlobal::findInstancesOfObj<ShaderLayer>(this);
		CCNode *searchNode = this;

		if (shaders.size() != 0) {
			searchNode = shaders[0];
		}

		CCNode *mainNode = searchNode->getChildByID("main-node");
		if (mainNode == nullptr) return nullptr;

		CCNode *batchLayer = mainNode->getChildByID("batch-layer");

		return batchLayer;
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
				if (DRSettings::debugMode) log::info("SETTING GHOST OPACITY TO 0");
				po->setOpacity(0);
			}

			std::string po_id = "ghost-player-" + std::to_string(i);
			po->setID(po_id);

			// idk
			CCNode *batchLayer =  findBatchNode();

			if (batchLayer != nullptr) {
				batchLayer->addChild(po, 99);
				m_fields->m_ghosts.push_back(po);
			} else {
				if (DRSettings::debugMode) log::info("batchLayer is nullptr!");
			}

			i++;
		}
	}

	void startGame() {
		if (DRSettings::debugMode) log::info("STARTGAME\n");

		if (m_fields->m_ghosts.size() == 0) {
			setupGhostPlayers();
		}

		if (m_fields->m_shouldDelayGhost) {
			float time = -DRSettings::ghostOffset;

			if (DRSettings::debugMode) log::info("delaying ghost by {} seconds!", time);

			schedule(schedule_selector(XPlayLayer::addPlayerPositionWait), 0, 0, time);
		} else {
			addPlayerPositionWait(0.f);
		}

		DRSettings::playTime = 0;

		m_fields->m_gameBegan = true;

		PlayLayer::startGame();

		setupPlayerInfoInFrame();
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
		DRSettings::debugMode = Mod::get()->getSettingValue<bool>("debug-mode");
		DRSettings::ghostOffset = Mod::get()->getSettingValue<double>("ghost-offset");
		DRSettings::xOpacity = Mod::get()->getSettingValue<double>("death-marker-opacity");

		m_fields->m_shouldDelayGhost = DRSettings::ghostOffset < 0.f;

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

		m_fields->m_processGhost = false;

		// this->schedule(schedule_selector(XPlayLayer::playGhost), 1.f / DRSettings::ghostFPS);

		return true;
	}

	void updateVisibility(float delta) {
		updatePlaytime(delta);

		DRSettings::currentlyInPractice = m_isPracticeMode;

		if (m_fields->m_processGhost) {
			playGhost(delta);
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

		if (!showDeaths) return PlayLayer::updateVisibility(delta);

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

			bool isOffline = true;

			// v1.2.0 new parameters
			if (obj.size() >= 4) {
				playTime = obj[3].get<double>();
			}
			if (playTime == -1) playTimeIgnored = true;
			if (DRSettings::isOldLevel) playTimeIgnored = true;

			// v1.2.2 new parameters
			if (obj.size() >= 5) {
				isOffline = obj[4].get<int>();
			}

			if (!created && 
				(
					(x <= m_player1->getPositionX())
					||
					((DRSettings::playTime >= playTime) && !playTimeIgnored)
				)
			) {
				_progresses["attempts"][i][2] = 1;

				CCNode *nd = CCNode::create();

				CCSprite *spr;

				if (!isOffline) {
					spr = CCSprite::create("dogotrigger.deathreplay/DR_globedIcon_001.png");
				} else {
					spr = CCSprite::createWithSpriteFrameName("GJ_deleteIcon_001.png");
				}

				spr->setScale(1.15f);

				nd->setPositionX(x);
				nd->setPositionY(y);

				spr->setOpacity(0);
				spr->runAction(CCFadeTo::create(0.2, (unsigned char)(255.f * DRSettings::xOpacity)));
				
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

				if(DRSettings::playDeathEffect && isOffline) {
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

		PlayLayer::updateVisibility(delta);
	}

	void resetLevel() {
		if (DRSettings::debugMode) log::info("RESET LEVEL\n");

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
		m_fields->m_processGhost = false;
		m_fields->m_shouldSendGamemodeInfo = true;

		if (m_fields->m_shouldDelayGhost) {
			float time = -DRSettings::ghostOffset;

			if (DRSettings::debugMode) log::info("delaying ghost by {} seconds!", time);

			schedule(schedule_selector(XPlayLayer::addPlayerPositionWait), 0, 0, time);
		} else {
			addPlayerPositionWait(0.f);
		}

		if (DRSettings::showGhost && m_fields->m_currentGhost.size() > 0 && DRSettings::ghostOffset > 0.f) {
			offsetGhostBySeconds(DRSettings::ghostOffset);
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
		PlayerObject::playerDestroyed(p0);

		if (PlayLayer::get() == nullptr) return;

		if (DRSettings::currentlyInPractice && !DRSettings::recordPractice) return PlayerObject::playDeathEffect();
		
		nlohmann::json attempt = nlohmann::json::array();

		auto pl = PlayLayer::get();

		if (pl->m_player1 == this || pl->m_player2 == this) {
			DRGlobal::deadPlayerIsOffline = true;
		} else {
			DRGlobal::deadPlayerIsOffline = false;
		}

		if (DRSettings::debugMode) log::info("PLAYER DESTROYED WITH P0={} (ONLINE={})", p0, !DRGlobal::deadPlayerIsOffline);

		if (!DRSettings::withGlobed) {
			if (pl->m_player1 == this || pl->m_player2 == this) {
				DRGlobal::deadPlayerIsOffline = true;
			} else {
				return PlayerObject::playerDestroyed(p0);
			}
		}

		attempt.push_back((float)getPositionX());
		attempt.push_back((float)getPositionY());
		attempt.push_back((int)0);

		if (!DRSettings::inPlatformer) attempt.push_back((double)DRSettings::playTime);
		else {
			attempt.push_back((double)-1);
		}

		attempt.push_back((int)DRGlobal::deadPlayerIsOffline);
		attempt.push_back((int)pl->getCurrentPercentInt());

		//printf("added death notif with: %f %f %d %f\n", getPositionX(), getPositionY(), 1, (float)DRSettings::playTime);

		_progresses["attempts"].push_back(attempt);

		int id = pl->m_level->m_levelID.value();

		std::string filename = generateFilename(pl->m_level);

		std::ofstream o(filename);

		o << _progresses;

		o.close();
	}

	void pushEvent(struct PlayerEvent ev) {
		if (DRSettings::actionInstance == nullptr || PlayLayer::get() == nullptr) return;

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

	void eventDebugger(std::string str = "unknown", unsigned char p0 = 0, unsigned char p1 = 1) {
		if (!DRSettings::debugMode) return;

		log::info("{}->{},{}", str, p0, p1);
	}

	void playDeathEffect() {
		eventDebugger("death");
		pushEvent({PlayerEvent::PlayDeathEffect});

		PlayerObject::playDeathEffect();
	}
	void toggleBirdMode(bool p0, bool p1) {
		eventDebugger("bird");
		PlayerObject::toggleBirdMode(p0, p1);

		pushEvent({PlayerEvent::ToggleBirdMode, p0, p1});
	}
	void toggleDartMode(bool p0, bool p1) {
		eventDebugger("dart", p0, p1);
		PlayerObject::toggleDartMode(p0, p1);

		pushEvent({PlayerEvent::ToggleDartMode, p0, p1});
	}
	void toggleFlyMode(bool p0, bool p1) {
		eventDebugger("fly", p0, p1);
		PlayerObject::toggleFlyMode(p0, p1);

		pushEvent({PlayerEvent::ToggleFlyMode, p0, p1});
	}
	void togglePlayerScale(bool p0, bool p1) {
		eventDebugger("scale", p0, p1);
		PlayerObject::togglePlayerScale(p0, p1);

		pushEvent({PlayerEvent::TogglePlayerScale, p0, p1});
	}
	void toggleRobotMode(bool p0, bool p1) {
		eventDebugger("robot", p0, p1);
		PlayerObject::toggleRobotMode(p0, p1);

		pushEvent({PlayerEvent::ToggleRobotMode, p0, p1});
	}
	void toggleRollMode(bool p0, bool p1) {
		eventDebugger("roll", p0, p1);
		PlayerObject::toggleRollMode(p0, p1);

		pushEvent({PlayerEvent::ToggleRollMode, p0, p1});
	}
	void toggleSpiderMode(bool p0, bool p1) {
		eventDebugger("spider", p0, p1);
		PlayerObject::toggleSpiderMode(p0, p1);

		pushEvent({PlayerEvent::ToggleSpiderMode, p0, p1});
	}
	void toggleSwingMode(bool p0, bool p1) {
		eventDebugger("swing", p0, p1);
		PlayerObject::toggleSwingMode(p0, p1);

		pushEvent({PlayerEvent::ToggleSwingMode, p0, p1});
	}
	void toggleVisibility(bool p0) {
		eventDebugger("visibility", p0);
		PlayerObject::toggleVisibility(p0);

		pushEvent({PlayerEvent::ToggleVisibility, p0});
	}
	void playSpawnEffect() {
		eventDebugger("spawn");
		pushEvent({PlayerEvent::PlaySpawnEffect});

		PlayerObject::playSpawnEffect();
	}
	void startDashing(DashRingObject* p0) {
		eventDebugger("start dashing");
		pushEvent({ PlayerEvent::StartDashing, p0 });

		PlayerObject::startDashing(p0);
	}
	void stopDashing() {
		eventDebugger("stop dashing");
		pushEvent({ PlayerEvent::StopDashing });

		PlayerObject::stopDashing();
	}
	void playSpiderDashEffect(CCPoint p0, CCPoint p1) {
		eventDebugger("spider effect");
		pushEvent({ PlayerEvent::SpiderDashEffect, p0, p1 });

		PlayerObject::playSpiderDashEffect(p0, p1);
	}
	void playDynamicSpiderRun() {
		eventDebugger("spider run");
		pushEvent({ PlayerEvent::SpiderRun });

		PlayerObject::playDynamicSpiderRun();
	}
};