#include "HomeScene.h"
#include "SettingScene.h"
#include "StickerScene.h"
#include "AlbumScene.h"
#include "Trading/TradingScene.h"

#define kTagFacebookPage 0
#define kTagMoreGame 1
#define kTagRating 2
#define kTagSetting 3
#define kTagFreePacket 4
#define kTagCommonPacket 6
#define kTagUncommonPacket 7
#define kTagRarePacket 8
#define kTagNewSticker 9
#define kTagFriendList 10
#define kTagTradeLayerElements 11
#define kTagVeryRarePacket 12
#define kTagRarestPacket 13

TTFConfig configControlButton(s_font, 65 * s_font_ratio);
TTFConfig configLabelSticker(s_font, 60 * s_font_ratio);

//---------------------------------------------------------------------Constructor methods
HomeScene* HomeScene::instance = nullptr;
HomeScene* HomeScene::getInstance() {
	if (HomeScene::instance == nullptr) {
		HomeScene::instance = HomeScene::create();
	}
	return HomeScene::instance;
}

Scene* HomeScene::scene() {
	// 'scene' is an autorelease object
	Scene *scene = Scene::create();

	// 'layer' is an autorelease object
	HomeScene::instance = nullptr;
	HomeScene *layer = HomeScene::getInstance();
	// add layer as a child to scene
	scene->addChild(layer);

	// return the scene
	return scene;
}

// on "init" you need to initialize your instance
bool HomeScene::init() {
	isHomeScene = true;
	bool result = BaseScene::init();

	//Set delegate to get response from FacebookHandler and FirebaseHandler
	FirebaseHandler::getInstance()->setFirebaseDelegate(this);
	FirebaseHandler::getInstance()->setFirebaseTradeFeatureDelegate(this);
	FacebookHandler::getInstance()->setFacebookConnectDelegate(this);
#ifdef SDKBOX_ENABLED
	sdkbox::IAP::setListener(this);
#endif

	//Show ads
	auto funcShowAds = CallFunc::create([=]() {
		showFullscreenAds();
	});
	this->runAction(
			Sequence::create(DelayTime::create(2), funcShowAds, nullptr));

	//////////////////////////////
	// 1. super init first
	if (!LayerColor::initWithColor(Color4B(255, 255, 255, 255))) {
		return false;
	}

	//Init default variables
	initDefaultVariables();

	//Add background
	Sprite* background = Sprite::create(s_homescene_background);
	background->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	background->setPosition(winSize.width / 2, winSize.height / 2);
	this->addChild(background);

	//Init views
	initPacketButtons();
	initOtherViews();
	initSettingMenu();
	initControlButtons();

	//Set visibily for some views at first time
	setVisibilityPacket();

	//Handling touch event
	auto listener = EventListenerTouchOneByOne::create();
	listener->setSwallowTouches(true);
	listener->onTouchBegan = CC_CALLBACK_2(HomeScene::onTouchBegan, this);
	_eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

	//Keyboard handling
	auto keyboardListener = EventListenerKeyboard::create();
	keyboardListener->onKeyReleased = CC_CALLBACK_2(HomeScene::onKeyReleased,
			this);
	_eventDispatcher->addEventListenerWithSceneGraphPriority(keyboardListener,
			this);

	//Login facebook
	auto func = CallFunc::create([=]() {
		if (FacebookHandler::getInstance()->isFacebookLoggedIn())
		{
			FirebaseHandler::getInstance()->checkFacebookIdExistOnFirebase();
		}
		else
		{
//							loadingSprite->setVisible(false);
//							loadingSprite_child->setVisible(false);
			isRequestDone = true;
		}
	});
	this->runAction(Sequence::create(DelayTime::create(0.5f), func, nullptr));

	//Schedule game loops
	scheduleUpdate();
	schedule(schedule_selector(HomeScene::timer), 1);
    timer(1); //Call it immediately 2 times
    timer(1); //Call it immediately 2 times
	return result;
}
//---------------------------------------------------------------------End of constructor methods
//---------------------------------------------------------------------Init methods
void HomeScene::initDefaultVariables() {
	isRequestDone = true;
	isTradeLayerHasContent = false;
	currentStickers = StickerHelper::getCurrentExistStickerNumber(true);

	timeToGetFreeStickerInSecond = UserDefault::getInstance()->getIntegerForKey(
	TIME_TO_GET_FREE_STICKER_IN_SECOND, time(nullptr));

	isMenuBarShowing = false;
	isAPacketAvailable = isAPacketAvailableFunc();

	//Cut animation
	int frameAmount_cut = 2;
	Vector<SpriteFrame*> animFrames_cut(frameAmount_cut);
	for (int i = 0; i < frameAmount_cut; i++) {
		Rect rect = Rect(0, 0, 144, 85);
		auto frame = SpriteFrame::create(s_homescene_cut_sheet[i], rect);
		animFrames_cut.pushBack(frame);
	}
	auto animation_cut = Animation::createWithSpriteFrames(animFrames_cut,
			0.1f);
	cut_animate = Animate::create(animation_cut);
	cut_animate->retain();

	//TODO get probability from Firebase
	FirebaseHandler::getInstance()->getProbabilityFreePacket();
	FirebaseHandler::getInstance()->getProbabilityCommonPacket();
	FirebaseHandler::getInstance()->getProbabilityUncommonPacket();
	FirebaseHandler::getInstance()->getProbabilityRarePacket();
	FirebaseHandler::getInstance()->getProbabilityVeryRarePacket();
	FirebaseHandler::getInstance()->getProbabilityRarestPacket();
}

void HomeScene::initPacketButtons() {
	//Sprite to show time to get free sticker
	spriteTimeFreeSticker = Sprite::create(
			s_homescene_sprite_time_to_get_free_sticker);
	spriteTimeFreeSticker->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	spriteTimeFreeSticker->setPosition(
			spriteTimeFreeSticker->getContentSize().width / 2 + 10,
			spriteTimeFreeSticker->getContentSize().height / 2 + 10);
	this->addChild(spriteTimeFreeSticker);

	labelTimeToGetFreeSticker = Label::createWithTTF(configControlButton, "",
			TextHAlignment::CENTER);
	labelTimeToGetFreeSticker->setPosition(
			Vec2(spriteTimeFreeSticker->getContentSize().width / 2,
					spriteTimeFreeSticker->getContentSize().height / 2 - 120));
	labelTimeToGetFreeSticker->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	labelTimeToGetFreeSticker->setColor(Color3B::BLACK);
	spriteTimeFreeSticker->addChild(labelTimeToGetFreeSticker);
	auto scale = ScaleBy::create(0.7f, 1.1f);
	auto scale2 = scale->reverse();
	auto seq = Sequence::create(scale, scale2, nullptr);
	auto repeat = RepeatForever::create(seq);
	spriteTimeFreeSticker->runAction(repeat);

	//Add btn packets
	addPacketFromUserDefault();
}

void HomeScene::addPacketFromUserDefault() {
	for (Button* button : vtButtonBottomPackets) {
		if (button->getParent() != nullptr) {
			button->getParent()->removeChild(button, true);
		}
	}
	for (Button* button : vtButtonTopPackets) {
		if (button->getParent() != nullptr) {
			button->getParent()->removeChild(button, true);
		}
	}
	vtButtonBottomPackets.clear();
	vtButtonTopPackets.clear();

	vector<Rect> vtButtonRect;
	vector<STICKER_RARITY> vtStickerPacketRarity =
			StickerHelper::getCurrentPacketsFromSharePreferences();
	for (STICKER_RARITY packetRarity : vtStickerPacketRarity) {
		string imageResourceTop;
		string imageResourceBottom;
		int tag;

		if (packetRarity == STICKER_RARITY::UNKNOWN) //Free packet
				{
			imageResourceTop = s_homescene_btn_free_packet_top;
			imageResourceBottom = s_homescene_btn_free_packet_bottom;
			tag = kTagFreePacket;
		} else if (packetRarity == STICKER_RARITY::COMMON) {
			imageResourceTop = s_homescene_btn_common_packet_top;
			imageResourceBottom = s_homescene_btn_common_packet_bottom;
			tag = kTagCommonPacket;
		} else if (packetRarity == STICKER_RARITY::UNCOMMON) {
			imageResourceTop = s_homescene_btn_uncommon_packet_top;
			imageResourceBottom = s_homescene_btn_uncommon_packet_bottom;
			tag = kTagUncommonPacket;
		} else if (packetRarity == STICKER_RARITY::RARE) {
			imageResourceTop = s_homescene_btn_rare_packet_top;
			imageResourceBottom = s_homescene_btn_rare_packet_bottom;
			tag = kTagRarePacket;
		} else if (packetRarity == STICKER_RARITY::VERYRARE) {
			imageResourceTop = s_homescene_btn_very_rare_packet_top;
			imageResourceBottom = s_homescene_btn_very_rare_packet_bottom;
			tag = kTagVeryRarePacket;
		} else if (packetRarity == STICKER_RARITY::RAREST) {
			imageResourceTop = s_homescene_btn_rarest_packet_top;
			imageResourceBottom = s_homescene_btn_rarest_packet_bottom;
			tag = kTagRarestPacket;
		}

		Button* btnPacketTop = Button::create(imageResourceTop);
		btnPacketTop->setZoomScale(0);
		btnPacketTop->setPosition(
				Vec2(-btnPacketTop->getContentSize().width / 2,
						btnPacketTop->getContentSize().height / 2));
		btnPacketTop->setTag(tag);
		this->addChild(btnPacketTop);
		Button* btnPacketBottom = Button::create(imageResourceBottom);
		btnPacketBottom->setZoomScale(0);
		btnPacketBottom->setPressedActionEnabled(false);
		btnPacketBottom->setPosition(
				Vec2(-btnPacketBottom->getContentSize().width / 2,
						btnPacketBottom->getContentSize().height / 2));
		btnPacketBottom->addTouchEventListener(
				CC_CALLBACK_2(HomeScene::packetButtonsCallback, this));
		btnPacketBottom->setTag(tag);
		this->addChild(btnPacketBottom);

		Vec2 packetRandomPosition = Vec2(
				CppUtils::randomBetween(winSize.width * 0.18,
						winSize.width * 0.82),
				CppUtils::randomBetween(winSize.height * 0.3,
						winSize.height * 0.65));

		//Create a rect of the packet -> put to vector and make sure random position generated will not be conflicted
		Rect packetRect;
		packetRect.size = btnPacketTop->getContentSize() * 2 / 3; //Half of the packet
		packetRect.origin = packetRandomPosition;

		int index = 0;
		while (index < 100) {
			bool conflicted = false;
			for (Rect rect : vtButtonRect) {
				if (rect.intersectsRect(packetRect)) {
					conflicted = true;
					break;
				}
			}
			if (conflicted) {
				//Generate another random position if conflicted
				packetRandomPosition = Vec2(
						CppUtils::randomBetween(winSize.width * 0.18,
								winSize.width * 0.82),
						CppUtils::randomBetween(winSize.height * 0.3,
								winSize.height * 0.65));
				packetRect.origin = packetRandomPosition;
				index++;
			} else {
				break;
			}
		}

		Sequence* actionMoveTo = Sequence::create(DelayTime::create(0.5),
				MoveTo::create(1, packetRandomPosition), nullptr);
		btnPacketBottom->runAction(actionMoveTo);
		btnPacketTop->runAction(actionMoveTo->clone());
		vtButtonBottomPackets.push_back(btnPacketBottom);
		vtButtonTopPackets.push_back(btnPacketTop);
		vtButtonRect.push_back(packetRect);
		CCLog("bambi HomeScene - > init packet buttons: %d", tag);
	}
	isPacketRunningTransactionDone = false;
	auto func = CallFunc::create([=]() {
		isPacketRunningTransactionDone = true;
	});
	this->runAction(Sequence::create(DelayTime::create(1.5), func, nullptr));

}

void HomeScene::setVisibilityViewsOfTradingFeature() {
	bool isFacebookLoggedIn =
			FacebookHandler::getInstance()->isFacebookLoggedIn();
	btnFacebookConnect->setVisible(!isFacebookLoggedIn);
	btnTrade->setVisible(isFacebookLoggedIn);
	btnFriend->setVisible(isFacebookLoggedIn);
}

bool isInviting = false;
bool isAcceptingRequest = false;
bool isDenyingRequest = false;
void HomeScene::initOtherViews() {
	//Add btnFacebookConnect
	btnFacebookConnect = Button::create(s_homescene_btn_facebook_connect);
	btnFacebookConnect->setPosition(
			Vec2(
					winSize.width
							- btnFacebookConnect->getContentSize().width / 2
							- 5,
					btnFacebookConnect->getContentSize().height / 2 + 10));
	btnFacebookConnect->setTouchEnabled(true);
	btnFacebookConnect->setPressedActionEnabled(true);
	btnFacebookConnect->addTouchEventListener(
			CC_CALLBACK_2(HomeScene::facebookConnectButtonCallback, this));
	this->addChild(btnFacebookConnect);
	Sprite* spriteFacebookConnect = Sprite::create(
			s_homescene_sprite_facebook_connect);
	spriteFacebookConnect->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	spriteFacebookConnect->setPosition(
			btnFacebookConnect->getContentSize().width
					- spriteFacebookConnect->getContentSize().width / 2,
			btnFacebookConnect->getContentSize().height
					- spriteFacebookConnect->getContentSize().height / 2);
	btnFacebookConnect->addChild(spriteFacebookConnect);
	spriteFacebookConnect->runAction(
			RepeatForever::create(
					Sequence::create(ScaleTo::create(0.5, 0.95),
							ScaleTo::create(0.5, 1), nullptr)));

	//Add btn friend
	btnFriend = Button::create(s_homescene_btn_friend);
	btnFriend->setPosition(
			Vec2(winSize.width - btnFriend->getContentSize().width / 2 - 10,
					btnFriend->getContentSize().height / 2 + 10));
	btnFriend->setTouchEnabled(true);
	btnFriend->setPressedActionEnabled(true);
	btnFriend->addTouchEventListener(
			CC_CALLBACK_2(HomeScene::friendButtonCallback, this));
	this->addChild(btnFriend);
	btnFriend->setEnabled(false);

	//Add btn trade
	btnTrade = Button::create(s_homescene_btn_trade);
	btnTrade->setPosition(
			Vec2(
					winSize.width - btnTrade->getContentSize().width / 2
							- btnFriend->getContentSize().width - 20 - 10,
					btnTrade->getContentSize().height / 2 + 10));
	btnTrade->setTouchEnabled(true);
	btnTrade->setPressedActionEnabled(true);
	btnTrade->addTouchEventListener(
			CC_CALLBACK_2(HomeScene::tradeButtonCallback, this));
	this->addChild(btnTrade);
	btnTrade->setEnabled(false);

	//Set show btnFacebookConnect if user hasn't logged in Facebook, vice versa
	setVisibilityViewsOfTradingFeature();

	//Add btn rewarded ads
	btnRewardedAds = Button::create(s_homescene_btn_rewarded_ads);
	btnRewardedAds->setPosition(
			Vec2(
					winSize.width / 2
							- btnRewardedAds->getContentSize().width / 2 - 30,
					winSize.height * 0.5));
	btnRewardedAds->setTouchEnabled(true);
	btnRewardedAds->setPressedActionEnabled(true);
	btnRewardedAds->addTouchEventListener(
			CC_CALLBACK_2(HomeScene::rewardedButtonsCallback, this));
	this->addChild(btnRewardedAds);

	//Add btn iap
	btnIAP = Button::create(s_homescene_btn_iap);
	btnIAP->setPosition(
			Vec2(winSize.width / 2 + btnIAP->getContentSize().width / 2 + 30,
					winSize.height * 0.5));
	btnIAP->setTouchEnabled(true);
	btnIAP->setPressedActionEnabled(true);
	btnIAP->addTouchEventListener(
			CC_CALLBACK_2(HomeScene::iapButtonsCallback, this));
	this->addChild(btnIAP);

	//Progress bar
	progressBar = LoadingBar::create();
	progressBar->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	progressBar->loadTexture(s_homescene_progress);
	progressBar->setPercent(currentStickers * 100 / vt_stickers.size());
	progressBar->setPosition(
			Vec2(winSize.width - progressBar->getContentSize().width / 2 - 40,
					winSize.height * 0.8));
	this->addChild(progressBar);

	Sprite* progressBackground = Sprite::create(s_homescene_bg_progress);
	progressBackground->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	progressBackground->setPosition(progressBar->getPositionX(),
			progressBar->getPositionY());
	this->addChild(progressBackground);

	labelSticker = Label::createWithTTF(configLabelSticker,
			String::createWithFormat("%d/%d stickers", currentStickers,
			vt_stickers.size())->getCString(), TextHAlignment::CENTER);
	labelSticker->setPosition(
			Vec2(progressBackground->getPositionX(),
					progressBackground->getPositionY()));
	labelSticker->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	labelSticker->setColor(Color3B::BLACK);
	this->addChild(labelSticker);

	//Add cut sprite
	cut = Sprite::create(s_homescene_cut_sheet[0]);
	cut->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	cut->setPositionX(-cut->getContentSize().width / 2);
	cut->setPositionY(winSize.height / 2 + 200);
	this->addChild(cut);

	//Add blur layer
	blurLayer = LayerColor::create(Color4B(0, 0, 0, 255));
	blurLayer->setContentSize(winSize);
	blurLayer->setPosition(Vec2::ZERO);
	blurLayer->setAnchorPoint(Vec2(0.0f, 0.0f));
	blurLayer->setVisible(false);
	this->addChild(blurLayer);

	//Add background to blurLayer
	Sprite* background = Sprite::create(s_homescene_background);
	background->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	background->setPosition(winSize.width / 2, winSize.height / 2);
	background->setOpacity(123);
	blurLayer->addChild(background);

	//Add btn continue
	btnContinue = Button::create(s_homescene_btn_continue);
	btnContinue->setPosition(
			Vec2(winSize.width - btnContinue->getContentSize().width / 2 - 30,
					btnContinue->getContentSize().height / 2 + 30));
	btnContinue->setTouchEnabled(true);
	btnContinue->setPressedActionEnabled(true);
	btnContinue->addTouchEventListener([this](Ref *pSender,
			Widget::TouchEventType type) {
		if (type == cocos2d::ui::Widget::TouchEventType::ENDED)
		{
            if(isSound){
                CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(s_click);
            }
			closeBlurLayer();
		}});
	blurLayer->addChild(btnContinue);

	//Add layer to show friend list
	friendLayer = LayerColor::create(Color4B(0, 0, 0, 100));
	friendLayer->setContentSize(winSize);
	friendLayer->setPosition(Vec2::ZERO);
	friendLayer->setAnchorPoint(Vec2(0.0f, 0.0f));
	friendLayer->setVisible(false);
	this->addChild(friendLayer);

	//Add board
	Sprite* boardFriend = Sprite::create(s_homescene_board_friends);
	boardFriend->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	boardFriend->setPosition(winSize / 2);
	friendLayer->addChild(boardFriend);

	//Add btn close
	Button* btnClose = Button::create(s_homescene_btn_close);
	btnClose->setPosition(
			Vec2(
					boardFriend->getPositionX()
							+ boardFriend->getContentSize().width / 2 - 10,
					boardFriend->getPositionY()
							+ boardFriend->getContentSize().height / 2 - 10));
	btnClose->setTouchEnabled(true);
	btnClose->setPressedActionEnabled(true);
	btnClose->addTouchEventListener([this](Ref *pSender,
			Widget::TouchEventType type) {
		if (type == cocos2d::ui::Widget::TouchEventType::ENDED)
		{
            if(isSound){
                CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(s_click);
            }
			closeFriendLayer();
		}});
	friendLayer->addChild(btnClose);

	//Add btn invite facebook
	Button* btnInviteFacebook = Button::create(s_homescene_btn_invite);
	btnInviteFacebook->setPosition(Vec2(winSize.width / 2, 310));
	btnInviteFacebook->setTouchEnabled(true);
	btnInviteFacebook->setPressedActionEnabled(true);
	btnInviteFacebook->addTouchEventListener(
			[this](Ref *pSender,
					Widget::TouchEventType type) {
				if (type == cocos2d::ui::Widget::TouchEventType::ENDED && !isInviting)
				{
                    if(isSound){
                        CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(s_click);
                    }
					isInviting = true;
					auto func = CallFunc::create([=]() {
								isInviting = false;
							});
					this->runAction(Sequence::create(DelayTime::create(1), func, nullptr));

					sdkbox::PluginFacebook::inviteFriends(FACEBOOK_INVITE_APP_URL,FACEBOOK_INVITE_IMAGE_URL);
				}});
	friendLayer->addChild(btnInviteFacebook);

	//Add layer to show trade list
	tradeLayer = LayerColor::create(Color4B(0, 0, 0, 100));
	tradeLayer->setContentSize(winSize);
	tradeLayer->setPosition(Vec2::ZERO);
	tradeLayer->setAnchorPoint(Vec2(0.0f, 0.0f));
	tradeLayer->setVisible(false);
	this->addChild(tradeLayer);

	//Add board
	Sprite* boardTrade = Sprite::create(s_homescene_board_trade);
	boardTrade->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	boardTrade->setPosition(winSize / 2);
	tradeLayer->addChild(boardTrade);

	//Add btn close
	Button* btnClose2 = Button::create(s_homescene_btn_close);
	btnClose2->setPosition(
			Vec2(
					boardTrade->getPositionX()
							+ boardTrade->getContentSize().width / 2 - 40,
					boardTrade->getPositionY()
							+ boardTrade->getContentSize().height / 2 - 10));
	btnClose2->setTouchEnabled(true);
	btnClose2->setPressedActionEnabled(true);
	btnClose2->addTouchEventListener([this](Ref *pSender,
			Widget::TouchEventType type) {
		if (type == cocos2d::ui::Widget::TouchEventType::ENDED)
		{
            if(isSound){
                CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(s_click);
            }
			closeTradeLayer();
		}});
	tradeLayer->addChild(btnClose2);

	//Add number of pending request
	Sprite* backgroundNumber = Sprite::create(s_homescene_bg_number);
	backgroundNumber->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	backgroundNumber->setPosition(
			Vec2(
					boardTrade->getPositionX()
							- boardTrade->getContentSize().width / 2 + 40,
					boardTrade->getPositionY()
							+ boardTrade->getContentSize().height / 2 - 10));
	tradeLayer->addChild(backgroundNumber);

	//Add layer to show IAP list
	iapLayer = LayerColor::create(Color4B(0, 0, 0, 100));
	iapLayer->setContentSize(winSize);
	iapLayer->setPosition(Vec2::ZERO);
	iapLayer->setAnchorPoint(Vec2(0.0f, 0.0f));
	iapLayer->setVisible(false);
	this->addChild(iapLayer);

	//Add board
	Sprite* boardIAP = Sprite::create(s_homescene_board_iap);
	boardIAP->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	boardIAP->setPosition(winSize / 2);
	iapLayer->addChild(boardIAP);

	//Add btn close
	Button* btnClose3 = Button::create(s_homescene_btn_close);
	btnClose3->setPosition(
			Vec2(
					boardIAP->getPositionX()
							+ boardIAP->getContentSize().width / 2 - 40,
					boardIAP->getPositionY()
							+ boardIAP->getContentSize().height / 2 - 10));
	btnClose3->setTouchEnabled(true);
	btnClose3->setPressedActionEnabled(true);
	btnClose3->addTouchEventListener([this](Ref *pSender,
			Widget::TouchEventType type) {
		if (type == cocos2d::ui::Widget::TouchEventType::ENDED)
		{
            if(isSound){
                CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(s_click);
            }
			iapLayer->setVisible(false);
		}});
	iapLayer->addChild(btnClose3);

	//Add btn iap 1
	Button* btnIAP1 = Button::create(s_homescene_btn_iap_sub);
	btnIAP1->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	btnIAP1->setPosition(
			Vec2(iapLayer->getContentSize().width / 2 - 300,
					iapLayer->getContentSize().height / 2 - 350));
	btnIAP1->setTouchEnabled(true);
	btnIAP1->setPressedActionEnabled(true);
	btnIAP1->addTouchEventListener([this](Ref *pSender,
			Widget::TouchEventType type) {
		if (type == cocos2d::ui::Widget::TouchEventType::ENDED)
		{
            if(isSound){
                CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(s_click);
            }
#if(CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
            sdkbox::IAP::purchase(IAP_IOS_PACK1_KEY);
#endif
#if(CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
            sdkbox::IAP::purchase(IAP_ANDROID_PACK1_KEY);
#endif
            
//			//FIXME for testing
//			sdkbox::Product product;
//			product.name = IAP_IOS_PACK1_KEY;
//			onSuccess(product);
		}});
	iapLayer->addChild(btnIAP1);

	//Add btn iap 2
	Button* btnIAP2 = Button::create(s_homescene_btn_iap_sub);
	btnIAP2->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	btnIAP2->setPosition(
			Vec2(iapLayer->getContentSize().width / 2,
					iapLayer->getContentSize().height / 2 - 350));
	btnIAP2->setTouchEnabled(true);
	btnIAP2->setPressedActionEnabled(true);
	btnIAP2->addTouchEventListener([this](Ref *pSender,
			Widget::TouchEventType type) {
		if (type == cocos2d::ui::Widget::TouchEventType::ENDED)
        {
            if(isSound){
                CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(s_click);
            }
            
#if(CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
            sdkbox::IAP::purchase(IAP_IOS_PACK2_KEY);
#endif
#if(CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
            sdkbox::IAP::purchase(IAP_ANDROID_PACK2_KEY);
#endif
            
		}});
	iapLayer->addChild(btnIAP2);

	//Add btn iap 3
	Button* btnIAP3 = Button::create(s_homescene_btn_iap_sub);
	btnIAP3->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	btnIAP3->setPosition(
			Vec2(iapLayer->getContentSize().width / 2 + 300,
					iapLayer->getContentSize().height / 2 - 350));
	btnIAP3->setTouchEnabled(true);
	btnIAP3->setPressedActionEnabled(true);
	btnIAP3->addTouchEventListener([this](Ref *pSender,
			Widget::TouchEventType type) {
		if (type == cocos2d::ui::Widget::TouchEventType::ENDED)
        {
            if(isSound){
                CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(s_click);
            }
#if(CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
            sdkbox::IAP::purchase(IAP_IOS_PACK3_KEY);
#endif
#if(CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
            sdkbox::IAP::purchase(IAP_ANDROID_PACK3_KEY);
#endif
            
		}});
	iapLayer->addChild(btnIAP3);
}

void HomeScene::initSettingMenu() {
	//Add menu bar
	menuBar = Sprite::create(s_homescene_menu_bar);
	menuBar->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	this->addChild(menuBar);

	//Set menubar position
	menuBarVisiblePosition = Vec2(5 + menuBar->getContentSize().width / 2,
			winSize.height - menuBar->getContentSize().height / 2);
	menuBarInvisiblePosition = Vec2(5 + menuBar->getContentSize().width / 2,
			winSize.height + menuBar->getContentSize().height / 2 - 170);
	menuBar->setPosition(menuBarVisiblePosition);

	//Add btn setting
	Button* btnSetting = Button::create(s_homescene_btn_setting);
	btnSetting->setPosition(
			Vec2(menuBar->getContentSize().width / 2,
					menuBar->getContentSize().height * 0.3));
	btnSetting->setTouchEnabled(true);
	btnSetting->setPressedActionEnabled(true);
	btnSetting->addTouchEventListener(
			CC_CALLBACK_2(HomeScene::settingButtonsCallback, this));
	btnSetting->setTag(kTagSetting);
	menuBar->addChild(btnSetting);
	HomeScene::invalidateMenuBarPosition();

	//Add btn rating
	Button* btnRating = Button::create(s_homescene_btn_rating);
	btnRating->setPosition(
			Vec2(menuBar->getContentSize().width / 2,
					menuBar->getContentSize().height * 0.5));
	btnRating->setTouchEnabled(true);
	btnRating->setPressedActionEnabled(true);
	btnRating->addTouchEventListener(
			CC_CALLBACK_2(HomeScene::settingButtonsCallback, this));
	btnRating->setTag(kTagRating);
	menuBar->addChild(btnRating);

	//Add btn more game
	Button* btnMoreGame = Button::create(s_homescene_btn_more_game);
	btnMoreGame->setPosition(
			Vec2(menuBar->getContentSize().width / 2,
					menuBar->getContentSize().height * 0.7));
	btnMoreGame->setTouchEnabled(true);
	btnMoreGame->setPressedActionEnabled(true);
	btnMoreGame->addTouchEventListener(
			CC_CALLBACK_2(HomeScene::settingButtonsCallback, this));
	btnMoreGame->setTag(kTagMoreGame);
	menuBar->addChild(btnMoreGame);

	//Add btn rating
	Button* btnFacebookPage = Button::create(s_homescene_btn_facebook_page);
	btnFacebookPage->setPosition(
			Vec2(menuBar->getContentSize().width / 2,
					menuBar->getContentSize().height * 0.9));
	btnFacebookPage->setTouchEnabled(true);
	btnFacebookPage->setPressedActionEnabled(true);
	btnFacebookPage->addTouchEventListener(
			CC_CALLBACK_2(HomeScene::settingButtonsCallback, this));
	btnFacebookPage->setTag(kTagFacebookPage);
	menuBar->addChild(btnFacebookPage);
}
void HomeScene::initControlButtons() {

	//Add btn sticker scene
	Button* btnStickerScene = Button::create(s_homescene_btn_sticker);
	btnStickerScene->setPosition(
			Vec2(
					winSize.width - btnStickerScene->getContentSize().width / 2
							- 5,
					winSize.height
							- btnStickerScene->getContentSize().height / 2
							- 10));
	btnStickerScene->setTouchEnabled(true);
	btnStickerScene->setPressedActionEnabled(true);
	btnStickerScene->addTouchEventListener(
			[this](Ref *pSender,
					Widget::TouchEventType type) {
				if (type == cocos2d::ui::Widget::TouchEventType::ENDED)
				{
					if (backgroundLayer != nullptr && backgroundLayer->isVisible()) {
						this->removeChild(backgroundLayer, false);
						backgroundLayer = nullptr;
					} else
					{
                        if(isSound){
                            CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(s_click);
                        }
						auto *newScene = StickerScene::scene();
						auto transition = TransitionFade::create(1.0, newScene);
						Director *pDirector = Director::getInstance();
						pDirector->replaceScene(transition);
						instance = nullptr;
					}
				}});
	this->addChild(btnStickerScene);

	//Add btn album
	Button* btnAlbumScene = Button::create(s_homescene_btn_album);
	btnAlbumScene->setPosition(
			Vec2(
					winSize.width - btnAlbumScene->getContentSize().width / 2
							- btnStickerScene->getContentSize().width - 5 - 20,
					winSize.height - btnAlbumScene->getContentSize().height / 2
							- 10));
	btnAlbumScene->setTouchEnabled(true);
	btnAlbumScene->setPressedActionEnabled(true);
	btnAlbumScene->addTouchEventListener(
			[this](Ref *pSender,
					Widget::TouchEventType type) {
				if (type == cocos2d::ui::Widget::TouchEventType::ENDED)
				{
					if (backgroundLayer != nullptr && backgroundLayer->isVisible()) {
						this->removeChild(backgroundLayer, false);
						backgroundLayer = nullptr;
					} else
					{
                        if(isSound){
                            CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(s_click);
                        }
						auto *newScene = AlbumScene::scene();
						auto transition = TransitionFade::create(1.0, newScene);
						Director *pDirector = Director::getInstance();
						pDirector->replaceScene(transition);
						instance = nullptr;
					}
				}});
	this->addChild(btnAlbumScene);

	//Add btn home
	Button* btnHomeScene = Button::create(s_homescene_btn_home);
	btnHomeScene->setPosition(
			Vec2(
					winSize.width - btnHomeScene->getContentSize().width / 2
							- btnAlbumScene->getContentSize().width
							- btnStickerScene->getContentSize().width - 5 - 40,
					winSize.height - btnHomeScene->getContentSize().height / 2
							- 10));
	btnHomeScene->setZoomScale(0);
	btnHomeScene->addTouchEventListener(
			[this](Ref *pSender,
					Widget::TouchEventType type) {
				if (type == cocos2d::ui::Widget::TouchEventType::ENDED)
				{
					if (backgroundLayer != nullptr && backgroundLayer->isVisible()) {
						this->removeChild(backgroundLayer, false);
						backgroundLayer = nullptr;
					}
				}});
	this->addChild(btnHomeScene);

}

bool HomeScene::isAPacketAvailableFunc() {
	return StickerHelper::getCurrentPacketsFromSharePreferences().size() > 0;
}

//---------------------------------------------------------------------End of init methods
//---------------------------------------------------------------------Game loop methods
void HomeScene::update(float dt) {

}

void HomeScene::timer(float interval) {
	if (btnRewardedAds != nullptr) {
		bool isAdsAvailable = BaseScene::isRewardedAdsAvailable();
		btnRewardedAds->setEnabled(isAdsAvailable);
		btnRewardedAds->setOpacity(isAdsAvailable ? 255 : 150);
	}

	int currentTimeInSecond = time(nullptr);
	int secondLeft = timeToGetFreeStickerInSecond - currentTimeInSecond;
	int minuteLeft = secondLeft / 60;
	secondLeft = secondLeft % 60;
	if (secondLeft >= 0) {
		labelTimeToGetFreeSticker->setString(
				String::createWithFormat("FREE in\n%d:%d", minuteLeft,
						secondLeft)->getCString());
	}

	isAPacketAvailable = isAPacketAvailableFunc();
	if (isAPacketAvailable) {
		setVisibilityPacket();
		this->unschedule(schedule_selector(HomeScene::timer));
	} else {
		if (time(nullptr) >= timeToGetFreeStickerInSecond) {
			StickerHelper::appendAPacketToSharePreferences(
					STICKER_RARITY::UNKNOWN);

			timeToGetFreeStickerInSecond = time(
					nullptr) + TIME_TO_GET_FREE_PACKET_IN_SECOND;
			UserDefault::getInstance()->setIntegerForKey(
			TIME_TO_GET_FREE_STICKER_IN_SECOND, timeToGetFreeStickerInSecond);
		}
	}
}
//---------------------------------------------------------------------End of game loop methods
//---------------------------------------------------------------------Invalidate views methods
void HomeScene::setVisibilityPacket() {
	btnRewardedAds->setVisible(!isAPacketAvailable);
	btnIAP->setVisible(!isAPacketAvailable);
	spriteTimeFreeSticker->setVisible(!isAPacketAvailable);
}

void HomeScene::invalidateProgressBar() {
	currentStickers = StickerHelper::getCurrentExistStickerNumber(true);
	labelSticker->setString(
			String::createWithFormat("%d/%d stickers", currentStickers,
			vt_stickers.size())->getCString());
	progressBar->setPercent(currentStickers * 100 / vt_stickers.size());
}

void HomeScene::invalidateMenuBarPosition() {
	if (menuBar->numberOfRunningActions() == 0) {
		menuBar->runAction(
				MoveTo::create(0.15f,
						isMenuBarShowing ?
								menuBarVisiblePosition :
								menuBarInvisiblePosition));
	}
}

void HomeScene::closeFriendLayer() {
	friendLayer->setVisible(false);
}

bool isClosingTradeLayer;
void HomeScene::closeTradeLayer() {
	if (tradeLayer != nullptr && !isClosingTradeLayer) {
		isClosingTradeLayer = true;
		auto func = CallFunc::create([=]() {
			isClosingTradeLayer = false;
		});
		this->runAction(Sequence::create(DelayTime::create(1), func, nullptr));

		CCLog("bambi HomeScene -> closeTradeLayer");
		Vector<Node*> layerChildren = tradeLayer->getChildren();
		for (Node* child : layerChildren) {
			if (child && child->getTag() == kTagTradeLayerElements) {
				CCLog("bambi HomeScene -> closeTradeLayer: child: %d",
						child->getTag());
				tradeLayer->removeChild(child, false);
			}
		}
		CCLog(
				"bambi HomeScene -> closeTradeLayer -> set layer visible = false");
		tradeLayer->setVisible(false);
		CCLog(
				"bambi HomeScene -> closeTradeLayer, vtGivenSticker size: %d, vtPendingRequest size: %d",
				vtGivenSticker.size(), vtPendingRequest.size());
		btnTrade->setEnabled(
				vtGivenSticker.size() + vtPendingRequest.size() > 0);
		isTradeLayerHasContent = false;
	}
}

void HomeScene::closeBlurLayer() {
	if (backgroundLayer == nullptr || !backgroundLayer->isVisible()) {
		Vector<Node*> layerChildren = blurLayer->getChildren();
		for (const auto child : layerChildren) {
			if (child && child->getTag() == kTagNewSticker) {
				blurLayer->removeChild(child, false);
			}
		}
		blurLayer->setVisible(false);
	}
}

//---------------------------------------------------------------------End of invalidate views methods
//---------------------------------------------------------------------Game logic methods
void HomeScene::earn3RandomStickers() {
	earn3Stickers(STICKER_RARITY::UNKNOWN);
}

Sticker* HomeScene::getARandomSticker(STICKER_RARITY packetRarity) {
	STICKER_RARITY rarity;
	int random = CppUtils::randomBetween(1, 100);
	string userdefaultKeyCommon = StickerHelper::getRarityString(packetRarity)
			+ "common";
	string userdefaultKeyUncommon = StickerHelper::getRarityString(packetRarity)
			+ "uncommon";
	string userdefaultKeyRare = StickerHelper::getRarityString(packetRarity)
			+ "rare";
	string userdefaultKeyRarest = StickerHelper::getRarityString(packetRarity)
			+ "rarest";
	string userdefaultKeyVeryRare = StickerHelper::getRarityString(packetRarity)
			+ "veryrare";

	CCLog("bambi HomeScene -> probability key: %s",
			userdefaultKeyCommon.c_str());
	CCLog("bambi HomeScene -> probability key: %s",
			userdefaultKeyUncommon.c_str());
	CCLog("bambi HomeScene -> probability key: %s", userdefaultKeyRare.c_str());
	CCLog("bambi HomeScene -> probability key: %s",
			userdefaultKeyRarest.c_str());
	CCLog("bambi HomeScene -> probability key: %s",
			userdefaultKeyVeryRare.c_str());

	int commonProbability = UserDefault::getInstance()->getIntegerForKey(
			userdefaultKeyCommon.c_str(), 0);
	int uncommonProbability = UserDefault::getInstance()->getIntegerForKey(
			userdefaultKeyUncommon.c_str(), 0);
	int rareProbability = UserDefault::getInstance()->getIntegerForKey(
			userdefaultKeyRare.c_str(), 0);
	int veryRareProbability = UserDefault::getInstance()->getIntegerForKey(
			userdefaultKeyVeryRare.c_str(), 0);
	int rarestProbability = UserDefault::getInstance()->getIntegerForKey(
			userdefaultKeyRarest.c_str(), 0);

	CCLog(
			"bambi HomeScene -> getARandomSticker: commonProb: %d, uncommonProb: %d, rareProb: %d, veryrareProb: %d, rarestProb: %d",
			commonProbability, uncommonProbability, rareProbability,
			veryRareProbability, rarestProbability);

	if (random < commonProbability) {
		rarity = STICKER_RARITY::COMMON;
	} else if (random < commonProbability + uncommonProbability) {
		rarity = STICKER_RARITY::UNCOMMON;
	} else if (random
			< commonProbability + uncommonProbability + rareProbability) {
		rarity = STICKER_RARITY::RARE;
	} else if (random
			< commonProbability + uncommonProbability + rareProbability
					+ veryRareProbability) {
		rarity = STICKER_RARITY::VERYRARE;
	} else if (random
			< commonProbability + uncommonProbability + rareProbability
					+ veryRareProbability + rarestProbability) {
		rarity = STICKER_RARITY::RAREST;
	} else {
		rarity = STICKER_RARITY::UNKNOWN;
	}

	while (true) {
		Sticker* sticker = vt_stickers.at(
				CppUtils::randomBetween(0, vt_stickers.size() - 1));
		if (sticker->rarity == rarity || rarity == STICKER_RARITY::UNKNOWN) {
			return sticker;
		}

	}
	return nullptr;
}

void HomeScene::earn3Stickers(STICKER_RARITY rarity) {
	StickerHelper::removeAPacketFromSharePerferences(rarity);

	isAPacketAvailable = isAPacketAvailableFunc();
	blurLayer->setVisible(true);
	setVisibilityPacket();
	if (!isAPacketAvailable) {
		schedule(schedule_selector(HomeScene::timer), 1);
	}

	string stickerIdString = "";
	for (int i = 0; i < 3; i++) {
		//Determine position of the sticker
		Vec2 position;
		if (i == 0) {
			position = Vec2(winSize.width / 3 - 40, winSize.height * 0.65);
		} else if (i == 1) {
			position = Vec2(winSize.width * 2 / 3 + 40, winSize.height * 0.65);
		} else {
			position = Vec2(winSize.width / 2, winSize.height * 0.3);
		}

		//Add sticker sprite
		Sticker * sticker = getARandomSticker(rarity);

		//Add btn sticker
		Button* stickerBtn = Button::create(sticker->sticker_image);
		stickerBtn->setTouchEnabled(true);
		stickerBtn->setZoomScale(0);
		stickerBtn->setPressedActionEnabled(true);
		stickerBtn->addTouchEventListener(
				[this, sticker](Ref *pSender,
						Widget::TouchEventType type) {
					if (type == cocos2d::ui::Widget::TouchEventType::ENDED)
					{
						if (backgroundLayer != nullptr && backgroundLayer->isVisible()) {
							this->removeChild(backgroundLayer, false);
							backgroundLayer = nullptr;
						} else
						{
                            if(isSound){
                                CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(s_click);
                            }
							openStickerDetailLayer(sticker);
						}}});
		stickerBtn->setTag(kTagNewSticker);

		//Add sprite_new if needed
		if (!StickerHelper::isStickerHasAlreadyExisted(sticker->sticker_id)) {
			Sprite* newSprite = Sprite::create(s_homescene_new);
			newSprite->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
			newSprite->setPosition(
					Vec2(
							stickerBtn->getContentSize().width
									- newSprite->getContentSize().width / 2,
							stickerBtn->getContentSize().height
									- newSprite->getContentSize().height / 2));
			stickerBtn->addChild(newSprite);
		}

		//Add a light below the sticker if type != common
		if (sticker->rarity != STICKER_RARITY::COMMON) {
			Sprite* lightSprite = Sprite::create(s_homescene_uncommon);
			lightSprite->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
			lightSprite->setPosition(position);
			lightSprite->setTag(kTagNewSticker); //For being removed from blurLayer later
			blurLayer->addChild(lightSprite);

			lightSprite->runAction(
					RepeatForever::create(RotateBy::create(5, 180)));
			lightSprite->runAction(
					RepeatForever::create(
							Sequence::create(ScaleTo::create(0.5, 0.9),
									ScaleTo::create(0.5, 1), nullptr)));
		}

		stickerBtn->setPosition(position);
		blurLayer->addChild(stickerBtn);

		stickerIdString += CppUtils::doubleToString(sticker->sticker_id);
		if (i < 2) {
			stickerIdString += ",";
		}
	}
	StickerHelper::saveToMyStickerList(stickerIdString);

	invalidateProgressBar();

	timeToGetFreeStickerInSecond = time(
			nullptr) + TIME_TO_GET_FREE_PACKET_IN_SECOND;
	UserDefault::getInstance()->setIntegerForKey(
	TIME_TO_GET_FREE_STICKER_IN_SECOND, timeToGetFreeStickerInSecond);
}

void HomeScene::openStickerDetailLayer(Sticker* sticker) {
	if (backgroundLayer != nullptr && backgroundLayer->isVisible()) {
		return;
	}

	CCLog("bambi HomeScene -> openStickerDetailLayer");
	TTFConfig configStickerDetailLabel(s_font, 100 * s_font_ratio);

	//Add blur layer
	backgroundLayer = LayerColor::create(Color4B(0, 0, 0, 255));
	backgroundLayer->setContentSize(winSize);
	backgroundLayer->setPosition(Vec2::ZERO);
	backgroundLayer->setAnchorPoint(Vec2(0.0f, 0.0f));
	this->addChild(backgroundLayer);

	//Add sticker sprite at center of the screen
	Sprite* stickerSprite = Sprite::create(sticker->sticker_image);
	stickerSprite->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	stickerSprite->setPosition(winSize.width / 2, winSize.height / 2);
	stickerSprite->setScale(1.5f);
	backgroundLayer->addChild(stickerSprite);

	//Add sticker id label
	BLabel* labelStickerId = BLabel::createWithTTF(configStickerDetailLabel,
			String::createWithFormat("#%d", sticker->sticker_id)->getCString(),
			TextHAlignment::CENTER);
	labelStickerId->setPosition(
			Vec2(winSize.width / 2,
					winSize.height - labelStickerId->getContentSize().height));
	labelStickerId->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	labelStickerId->setColor(Color3B::WHITE);
	backgroundLayer->addChild(labelStickerId);

	//Add sticker name label
	BLabel* labelStickerName = BLabel::createWithTTF(configStickerDetailLabel,
			sticker->sticker_name, TextHAlignment::CENTER);
	labelStickerName->setPosition(
			Vec2(winSize.width / 2,
					labelStickerId->getPositionY()
							- labelStickerName->getContentSize().height));
	labelStickerName->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	labelStickerName->setColor(Color3B::YELLOW);
	backgroundLayer->addChild(labelStickerName);

	//Add sticker glued info label
	int stickerTotalNumber = StickerHelper::getStickerQuantityInMyList(
			sticker->sticker_id);
	int gluedNumber =
			StickerHelper::isStickerHasNotSticked(sticker->sticker_id) ? 0 : 1;
	int leftNumber = stickerTotalNumber - gluedNumber;
	BLabel* labelStickerGluedInfo = BLabel::createWithTTF(
			configStickerDetailLabel,
			String::createWithFormat("(%d glued, %d left)", gluedNumber,
					leftNumber)->getCString(), TextHAlignment::CENTER);
	labelStickerGluedInfo->setPosition(
			Vec2(winSize.width / 2,
					labelStickerName->getPositionY()
							- labelStickerGluedInfo->getContentSize().height));
	labelStickerGluedInfo->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	labelStickerGluedInfo->setColor(Color3B::GRAY);
	backgroundLayer->addChild(labelStickerGluedInfo);

	//Add sticker page name label
	BLabel* labelStickerPageName =
			BLabel::createWithTTF(configStickerDetailLabel,
					StickerHelper::getStickerPageFromId(
							sticker->sticker_page_id)->sticker_page_name,
					TextHAlignment::CENTER);
	labelStickerPageName->setPosition(
			Vec2(winSize.width / 2, winSize.height * 0.2));
	labelStickerPageName->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	labelStickerPageName->setColor(Color3B::WHITE);
	backgroundLayer->addChild(labelStickerPageName);

	//Add sticker rarity label
	BLabel* labelStickerRarity = BLabel::createWithTTF(configStickerDetailLabel,
			StickerHelper::getRarityString(sticker->rarity),
			TextHAlignment::CENTER);
	labelStickerRarity->setPosition(
			Vec2(winSize.width / 2,
					labelStickerPageName->getPositionY()
							- labelStickerRarity->getContentSize().height));
	labelStickerRarity->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	labelStickerRarity->setColor(Color3B::WHITE);
	backgroundLayer->addChild(labelStickerRarity);
}

void HomeScene::addElementsToTradeLayer() {
	if (isTradeLayerHasContent) {
		return;
	}

	CCLog("bambi HomeScene -> addElementsToTradeLayer");
	TTFConfig labelConfig(s_font, 100 * s_font_ratio);
	int numberOfPendingRequest = vtPendingRequest.size();
	int numberOfGivenStickers = vtGivenSticker.size();
	//Add pending number label
	BLabel* labelPendingNumber =
			BLabel::createWithTTF(labelConfig,
					String::createWithFormat("%d",
							(numberOfPendingRequest + numberOfGivenStickers))->getCString(),
					TextHAlignment::CENTER);
	labelPendingNumber->setPosition(
			Vec2(80, tradeLayer->getContentSize().height - 240));
	labelPendingNumber->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	labelPendingNumber->setColor(Color3B::BLACK);
	labelPendingNumber->setTag(kTagTradeLayerElements);
	tradeLayer->addChild(labelPendingNumber, 2);

	if (vtPendingRequest.size() > 0) {
		isTradeLayerHasContent = true;
		PendingRequest* pendingRequest = vtPendingRequest.at(0);
		CCLog(
				"bambi HomeScene -> addElementsToTradeLayer -> adding pending request: stickerID: %s, %d",
				pendingRequest->getStickerId().c_str(),
				CppUtils::stringToDouble(pendingRequest->getStickerId()));
		Sticker* sticker = StickerHelper::getStickerFromId(
				CppUtils::stringToDouble(pendingRequest->getStickerId()));
		CCLog(
				"bambi HomeScene -> addElementsToTradeLayer -> adding pending request -> 2");
		//Add btn friend name
		Sprite*backgroundName = Sprite::create(
				s_homescene_btn_friend_name_background);
		backgroundName->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
		backgroundName->setPosition(tradeLayer->getContentSize().width / 2,
				tradeLayer->getContentSize().height * 0.8 - 150);
		backgroundName->setTag(kTagTradeLayerElements);
		tradeLayer->addChild(backgroundName);

		CCLog(
				"bambi HomeScene -> addElementsToTradeLayer -> adding pending request -> 3");
		//Add friend name label
		BLabel* labelName = BLabel::createWithTTF(labelConfig,
				String::createWithFormat("%s",
						pendingRequest->getName().c_str())->getCString(),
				TextHAlignment::CENTER);
		labelName->setPosition(
				Vec2(backgroundName->getContentSize().width / 2,
						backgroundName->getContentSize().height / 2));
		labelName->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
		labelName->setColor(Color3B::BLACK);
		labelName->setTag(kTagTradeLayerElements);
		backgroundName->addChild(labelName);

		CCLog(
				"bambi HomeScene -> addElementsToTradeLayer -> adding pending request -> 4");
		//Add btn my name
		Sprite*backgroundMyName = Sprite::create(
				s_homescene_btn_friend_name_background);
		backgroundMyName->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
		backgroundMyName->setPosition(tradeLayer->getContentSize().width / 2,
				tradeLayer->getContentSize().height * 0.3 + 30);
		backgroundMyName->setTag(kTagTradeLayerElements);
		tradeLayer->addChild(backgroundMyName);

		CCLog(
				"bambi HomeScene -> addElementsToTradeLayer -> adding pending request -> 5");
		//Add friend my label
		BLabel* labelMyName = BLabel::createWithTTF(labelConfig, "Me!",
				TextHAlignment::CENTER);
		labelMyName->setPosition(
				Vec2(backgroundMyName->getContentSize().width / 2,
						backgroundMyName->getContentSize().height / 2));
		labelMyName->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
		labelMyName->setColor(Color3B::BLACK);
		labelMyName->setTag(kTagTradeLayerElements);
		backgroundMyName->addChild(labelMyName);

		CCLog(
				"bambi HomeScene -> addElementsToTradeLayer -> adding pending request -> 6");
		//Add sprite arrow down
		Sprite* arrowDown = Sprite::create(s_homescene_arrow_down);
		arrowDown->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
		arrowDown->setPosition(170,
				tradeLayer->getContentSize().height / 2 + 20);
		arrowDown->setTag(kTagTradeLayerElements);
		tradeLayer->addChild(arrowDown);

		CCLog(
				"bambi HomeScene -> addElementsToTradeLayer -> adding pending request -> 7");
		//Add sticker sprite
		Sprite* stickerSprite = Sprite::create(sticker->sticker_image);
		stickerSprite->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
		stickerSprite->setPosition(tradeLayer->getContentSize().width / 2,
				tradeLayer->getContentSize().height / 2 - 20);
		stickerSprite->setScale(0.8f);
		stickerSprite->setTag(kTagTradeLayerElements);
		tradeLayer->addChild(stickerSprite);

		CCLog(
				"bambi HomeScene -> addElementsToTradeLayer -> adding pending request -> 8");
		//Add asking label
		BLabel* labelAsking = BLabel::createWithTTF(labelConfig,
				String::createWithFormat("Ask you for the sticker \"%s\"",
						sticker->sticker_name.c_str())->getCString(),
				TextHAlignment::CENTER);
		labelAsking->setPosition(
				Vec2(tradeLayer->getContentSize().width / 2,
						tradeLayer->getContentSize().height / 2 + 200));
		labelAsking->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
		labelAsking->setColor(Color3B::BLACK);
		labelAsking->setTag(kTagTradeLayerElements);
		tradeLayer->addChild(labelAsking);

		CCLog(
				"bambi HomeScene -> addElementsToTradeLayer -> adding pending request -> 9");
		//Add btn accept request
		Button* btnAcceptRequest = Button::create(s_homescene_btn_ok);
		btnAcceptRequest->setPosition(
				Vec2(
						winSize.width / 2
								- btnAcceptRequest->getContentSize().width / 2
								- 20, 350));
		btnAcceptRequest->setTouchEnabled(true);
		btnAcceptRequest->setPressedActionEnabled(true);
		btnAcceptRequest->addTouchEventListener(
				[this, pendingRequest](Ref *pSender,
						Widget::TouchEventType type) {
					if (type == cocos2d::ui::Widget::TouchEventType::ENDED && !isAcceptingRequest)
					{
                        if(isSound){
                            CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(s_click);
                        }
						isAcceptingRequest = true;
						auto func = CallFunc::create([=]() {
									isAcceptingRequest = false;
								});
						this->runAction(Sequence::create(DelayTime::create(1), func, nullptr));

						FirebaseHandler::getInstance()->acceptSendingSticker(vtPendingRequest, pendingRequest);
					}});
		btnAcceptRequest->setTag(kTagTradeLayerElements);
		tradeLayer->addChild(btnAcceptRequest);

		CCLog(
				"bambi HomeScene -> addElementsToTradeLayer -> adding pending request -> 10");
		//Add btn deny request
		Button* btnDenyRequest = Button::create(s_homescene_btn_deny);
		btnDenyRequest->setPosition(
				Vec2(
						winSize.width / 2
								+ btnDenyRequest->getContentSize().width / 2
								+ 20, 350));
		btnDenyRequest->setTouchEnabled(true);
		btnDenyRequest->setPressedActionEnabled(true);
		btnDenyRequest->addTouchEventListener(
				[this,pendingRequest](Ref *pSender,
						Widget::TouchEventType type) {
					if (type == cocos2d::ui::Widget::TouchEventType::ENDED && !isDenyingRequest)
					{
                        if(isSound){
                            CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(s_click);
                        }
						isDenyingRequest = true;
						auto func = CallFunc::create([=]() {
									isDenyingRequest = false;
								});
						this->runAction(Sequence::create(DelayTime::create(1), func, nullptr));

						FirebaseHandler::getInstance()->denySendingSticker(vtPendingRequest, pendingRequest);
					}});
		btnDenyRequest->setTag(kTagTradeLayerElements);
		tradeLayer->addChild(btnDenyRequest);

		CCLog(
				"bambi HomeScene -> addElementsToTradeLayer -> added pending request -> going to erase it from vector");

		vtPendingRequest.erase(
				std::remove(vtPendingRequest.begin(), vtPendingRequest.end(),
						pendingRequest), vtPendingRequest.end());
	} else if (vtGivenSticker.size() > 0) {
		isTradeLayerHasContent = true;
		CCLog(
				"bambi HomeScene -> addElementsToTradeLayer -> adding given sticker request");
		PendingRequest* pendingRequest = vtGivenSticker.at(0);
		Sticker* sticker = StickerHelper::getStickerFromId(
				CppUtils::stringToDouble(pendingRequest->getStickerId()));

		//Add btn friend name
		Sprite*backgroundName = Sprite::create(
				s_homescene_btn_friend_name_background);
		backgroundName->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
		backgroundName->setPosition(tradeLayer->getContentSize().width / 2,
				tradeLayer->getContentSize().height * 0.8 - 150);
		backgroundName->setTag(kTagTradeLayerElements);
		tradeLayer->addChild(backgroundName);

		//Add friend name label
		BLabel* labelName = BLabel::createWithTTF(labelConfig,
				String::createWithFormat("%s",
						pendingRequest->getName().c_str())->getCString(),
				TextHAlignment::CENTER);
		labelName->setPosition(
				Vec2(backgroundName->getContentSize().width / 2,
						backgroundName->getContentSize().height / 2));
		labelName->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
		labelName->setColor(Color3B::BLACK);
		labelName->setTag(kTagTradeLayerElements);
		backgroundName->addChild(labelName);

		//Add btn my name
		Sprite*backgroundMyName = Sprite::create(
				s_homescene_btn_friend_name_background);
		backgroundMyName->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
		backgroundMyName->setPosition(tradeLayer->getContentSize().width / 2,
				tradeLayer->getContentSize().height * 0.3 + 30);
		backgroundMyName->setTag(kTagTradeLayerElements);
		tradeLayer->addChild(backgroundMyName);

		//Add friend my label
		BLabel* labelMyName = BLabel::createWithTTF(labelConfig, "Me!",
				TextHAlignment::CENTER);
		labelMyName->setPosition(
				Vec2(backgroundMyName->getContentSize().width / 2,
						backgroundMyName->getContentSize().height / 2));
		labelMyName->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
		labelMyName->setColor(Color3B::BLACK);
		labelMyName->setTag(kTagTradeLayerElements);
		backgroundMyName->addChild(labelMyName);

		//Add sprite arrow down
		Sprite* arrowDown = Sprite::create(s_homescene_arrow_down);
		arrowDown->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
		arrowDown->setPosition(170,
				tradeLayer->getContentSize().height / 2 + 20);
		arrowDown->setTag(kTagTradeLayerElements);
		tradeLayer->addChild(arrowDown);

		//Add sticker sprite
		Sprite* stickerSprite = Sprite::create(sticker->sticker_image);
		stickerSprite->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
		stickerSprite->setPosition(tradeLayer->getContentSize().width / 2,
				tradeLayer->getContentSize().height / 2 - 20);
		stickerSprite->setScale(0.8f);
		stickerSprite->setTag(kTagTradeLayerElements);
		tradeLayer->addChild(stickerSprite);

		//Add asking label
		BLabel* labelAsking = BLabel::createWithTTF(labelConfig,
				String::createWithFormat("Sent you the sticker \"%s\"",
						sticker->sticker_name.c_str())->getCString(),
				TextHAlignment::CENTER);
		labelAsking->setPosition(
				Vec2(tradeLayer->getContentSize().width / 2,
						tradeLayer->getContentSize().height / 2 + 200));
		labelAsking->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
		labelAsking->setColor(Color3B::BLACK);
		labelAsking->setTag(kTagTradeLayerElements);
		tradeLayer->addChild(labelAsking);

		//Add btn accept request
		Button* btnAcceptRequest = Button::create(s_homescene_btn_ok);
		btnAcceptRequest->setPosition(Vec2(winSize.width / 2, 350));
		btnAcceptRequest->setTouchEnabled(true);
		btnAcceptRequest->setPressedActionEnabled(true);
		btnAcceptRequest->addTouchEventListener(
				[this, pendingRequest](Ref *pSender,
						Widget::TouchEventType type) {
					if (type == cocos2d::ui::Widget::TouchEventType::ENDED && !isAcceptingRequest)
					{
                        if(isSound){
                            CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(s_click);
                        }
						isAcceptingRequest = true;
						auto func = CallFunc::create([=]() {
									isAcceptingRequest = false;
								});
						this->runAction(Sequence::create(DelayTime::create(1), func, nullptr));

						FirebaseHandler::getInstance()->acceptReceivingSticker(vtGivenSticker, pendingRequest);
					}});
		btnAcceptRequest->setTag(kTagTradeLayerElements);
		tradeLayer->addChild(btnAcceptRequest);
		vtGivenSticker.erase(
				std::remove(vtGivenSticker.begin(), vtGivenSticker.end(),
						pendingRequest), vtGivenSticker.end());
	}
}

//---------------------------------------------------------------------End of game logic methods
//---------------------------------------------------------------------Callback methods

void HomeScene::responseAfterDenyingRequest(bool isSuccess) {
	CCLog(
			isSuccess ?
					"HomeScene -> responseAfterDenyingRequest -> success" :
					"HomeScene -> responseAfterDenyingRequest -> failed");
	if (isSuccess) {
		closeTradeLayer();
	}
}

void HomeScene::responseAfterAcceptingRequest(bool isSuccess) {
	CCLog(
			isSuccess ?
					"HomeScene -> responseAfterAcceptingRequest -> success" :
					"HomeScene -> responseAfterDenyingRequest -> failed");

	if (isSuccess) {
		closeTradeLayer();
	}
}

void HomeScene::packetButtonsCallback(Ref* pSender,
		ui::Widget::TouchEventType eEventType) {
	if (eEventType == ui::Widget::TouchEventType::BEGAN
			&& !blurLayer->isVisible() && !friendLayer->isVisible()
			&& !tradeLayer->isVisible() && !iapLayer->isVisible()
			&& isPacketRunningTransactionDone
			&& cut->numberOfRunningActions() == 0) {
        if(isSound){
            CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(s_click);
        }
        
		int animationDuration = 3;
		Button* btnPacketBottom = dynamic_cast<Button*>(pSender);
		int tag = (int) btnPacketBottom->getTag();
		Button* btnPacketTop = nullptr;
		for (Button* record : vtButtonTopPackets) {
			if (record->getTag() == tag
					&& record->getPosition()
							== btnPacketBottom->getPosition()) {
				btnPacketTop = record;
			}
		}

		cut->setPositionY(btnPacketBottom->getPositionY() + 200);
		cut->runAction(
				Sequence::create(
						Spawn::createWithTwoActions(
								Repeat::create(cut_animate, 20),
								MoveBy::create(2,
										Vec2(
												winSize.width
														+ cut->getContentSize().width,
												0))),
						MoveBy::create(0,
								Vec2(
										-winSize.width
												- cut->getContentSize().width,
										0)), nullptr));
		CallFunc* funcResetScheduleGetSticker;
		if (btnPacketTop != nullptr) {
			btnPacketTop->runAction(
					Sequence::create(
							DelayTime::create(
									1.7f * btnPacketTop->getPositionX()
											/ winSize.width),
							MoveBy::create(0.2, Vec2(0, 100)),
							DelayTime::create(2),
							MoveBy::create(0, Vec2(0, -100)), nullptr));
		}

		CallFunc* funcSetVisiblePacket = CallFunc::create([=]() {
			this->removeChild(btnPacketTop, false);
			this->removeChild(btnPacketBottom, false);

			vtButtonBottomPackets.erase(
					std::remove(vtButtonBottomPackets.begin(),
							vtButtonBottomPackets.end(), btnPacketBottom),
					vtButtonBottomPackets.end());

			vtButtonTopPackets.erase(
					std::remove(vtButtonTopPackets.begin(),
							vtButtonTopPackets.end(), btnPacketTop),
					vtButtonTopPackets.end());
		});

		switch (tag) {
		case kTagFreePacket: {
			funcResetScheduleGetSticker = CallFunc::create([=]() {
				earn3RandomStickers();
			});
		}
			break;
		case kTagCommonPacket: {
			funcResetScheduleGetSticker = CallFunc::create([=]() {
				earn3Stickers(STICKER_RARITY::COMMON);
			});
		}
			break;
		case kTagUncommonPacket: {
			funcResetScheduleGetSticker = CallFunc::create([=]() {
				earn3Stickers(STICKER_RARITY::UNCOMMON);
			});
		}
			break;
		case kTagRarePacket: {
			funcResetScheduleGetSticker = CallFunc::create([=]() {
				earn3Stickers(STICKER_RARITY::RARE);
			});
		}
			break;

		case kTagVeryRarePacket: {
			funcResetScheduleGetSticker = CallFunc::create([=]() {
				earn3Stickers(STICKER_RARITY::VERYRARE);
			});
		}
			break;

		case kTagRarestPacket: {
			funcResetScheduleGetSticker = CallFunc::create([=]() {
				earn3Stickers(STICKER_RARITY::RAREST);
			});
		}
			break;
		}
		btnPacketBottom->runAction(
				Sequence::create(DelayTime::create(animationDuration),
						funcResetScheduleGetSticker, funcSetVisiblePacket,
						nullptr));
	}
}

void HomeScene::iapButtonsCallback(Ref* pSender,
		ui::Widget::TouchEventType eEventType) {
	if (eEventType == ui::Widget::TouchEventType::ENDED
			&& !blurLayer->isVisible() && !friendLayer->isVisible()
			&& !tradeLayer->isVisible() && !iapLayer->isVisible()) {
      	if (backgroundLayer != nullptr && backgroundLayer->isVisible()) {
			this->removeChild(backgroundLayer, false);
			backgroundLayer = nullptr;
		} else {
            if(isSound){
                CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(s_click);
            }
            
			iapLayer->setVisible(true);
		}
	}
}
void HomeScene::rewardedButtonsCallback(Ref* pSender,
		ui::Widget::TouchEventType eEventType) {
	if (eEventType == ui::Widget::TouchEventType::ENDED
			&& !blurLayer->isVisible() && !friendLayer->isVisible()
			&& !tradeLayer->isVisible() && !iapLayer->isVisible()) {
		if (backgroundLayer != nullptr && backgroundLayer->isVisible()) {
			this->removeChild(backgroundLayer, false);
			backgroundLayer = nullptr;
		} else {
			showRewardedAds();
		}
	}
}

void HomeScene::tradeButtonCallback(Ref* pSender,
		ui::Widget::TouchEventType eEventType) {
	if (eEventType == ui::Widget::TouchEventType::ENDED
			&& !blurLayer->isVisible() && !friendLayer->isVisible()
			&& !tradeLayer->isVisible() && !iapLayer->isVisible()) {
  		int numberOfPendingRequest = vtPendingRequest.size();
		int numberOfGivenStickers = vtGivenSticker.size();
		if (backgroundLayer != nullptr && backgroundLayer->isVisible()) {
			this->removeChild(backgroundLayer, false);
			backgroundLayer = nullptr;
		} else
        {
            if(isSound){
                CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(s_click);
            }
            
            if (numberOfGivenStickers > 0 || numberOfPendingRequest > 0) {
			CCLog(
					"bambi HomeScene -> tradeButtonCallback -> going to addElementsToTradeLayer, numberOfGivenStickers: %d, numberOfPendingRequest: %d",
					numberOfGivenStickers, numberOfPendingRequest);
			addElementsToTradeLayer();
			if (tradeLayer != nullptr) {
				tradeLayer->setVisible(true);
			}
			isMenuBarShowing = false;
			invalidateMenuBarPosition();
		} else {
			SocialPlugin::showToast(
					"Your inbox is empty right now. Ask your friends for more stickers!");
		}
        }
	}
}

void HomeScene::friendButtonCallback(Ref* pSender,
		ui::Widget::TouchEventType eEventType) {
	if (eEventType == ui::Widget::TouchEventType::ENDED
			&& !blurLayer->isVisible() && !friendLayer->isVisible()
			&& !tradeLayer->isVisible() && !iapLayer->isVisible()) {
    	if (backgroundLayer != nullptr && backgroundLayer->isVisible()) {
			this->removeChild(backgroundLayer, false);
			backgroundLayer = nullptr;
		} else {
            if(isSound){
                CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(s_click);
            }
			friendLayer->setVisible(true);
			isMenuBarShowing = false;
			invalidateMenuBarPosition();
		}
	}
}

bool isLoggingInFacebook = false;
void HomeScene::facebookConnectButtonCallback(Ref* pSender,
		ui::Widget::TouchEventType eEventType) {
	if (eEventType == ui::Widget::TouchEventType::ENDED
			&& !blurLayer->isVisible() && !friendLayer->isVisible()
			&& !tradeLayer->isVisible() && !iapLayer->isVisible()
			&& !isLoggingInFacebook) {
		if (backgroundLayer != nullptr && backgroundLayer->isVisible()) {
			this->removeChild(backgroundLayer, false);
			backgroundLayer = nullptr;
		} else {
            if(isSound){
                CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(s_click);
            }
			CCLog("bambi HomeScene -> logging in");
			isLoggingInFacebook = true;
			auto func = CallFunc::create([=]() {
				isLoggingInFacebook = false;
			});
			this->runAction(
					Sequence::create(DelayTime::create(1), func, nullptr));

			FacebookHandler::getInstance()->loginFacebook();
		}
	}
}

void HomeScene::responseWhenLoginOrLogoutFacebook() {
	if (FacebookHandler::getInstance()->isFacebookLoggedIn()) {
		CCLog(
				"bambi HomeScene -> responseWhenLoginOrLogoutFacebook: logged in");
		FirebaseHandler::getInstance()->checkFacebookIdExistOnFirebase();
	} else {
		CCLog(
				"bambi HomeScene -> responseWhenLoginOrLogoutFacebook: not logged in");
		//							loadingSprite->setVisible(false);
		//							loadingSprite_child->setVisible(false);
		isRequestDone = true;
	}
	setVisibilityViewsOfTradingFeature();
}

void HomeScene::responseForQuerryTopFriend(vector<BUserInfor*> friendList) {
	FirebaseHandler::getInstance()->getStickersDataFromFirebase();

	CCLog("bambi HomeScene -> responseForQuerryTopFriend");
	if (friendLayer == nullptr) {
		return;
	}
	TTFConfig labelConfig(s_font, 100 * s_font_ratio);
	vt_Friends = friendList;
	friendLayer->removeChildByTag(kTagFriendList, false);

	btnFriend->setEnabled(vt_Friends.size() > 0);

//Scrollview configuration
	int numberOfItems = friendList.size();
	float itemMargin = 170;
	Size scrollFrameSize = Size(winSize.width, 880);

//Create scrollview
	BScrollView* scrollview = BScrollView::createVertical(numberOfItems,
			itemMargin, scrollFrameSize);
	scrollview->setPosition(Vec2(winSize.width / 2, winSize.height / 2 - 70));
	scrollview->setBounceEnabled(false);
	scrollview->setTag(kTagFriendList);
	scrollview->setScrollBarEnabled(false);
	friendLayer->addChild(scrollview);

//Add sth to scroll view
	float positionX = scrollview->leftPosition;
	float positionY = scrollview->topPosition;
	for (int i = 1; i <= numberOfItems; i++) {
		//Add btn friend name
		Button*stickerBtn = Button::create(
				s_homescene_btn_friend_name_background);
		stickerBtn->setTouchEnabled(true);
		stickerBtn->setPressedActionEnabled(true);
		stickerBtn->setTag(i);
		stickerBtn->addTouchEventListener(
				[this](Ref *pSender,
						Widget::TouchEventType type) {
					if (type == cocos2d::ui::Widget::TouchEventType::ENDED && !isOpeningAnotherScene)
					{
                        if(isSound){
                            CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(s_click);
                        }
						isOpeningAnotherScene = true;
						auto func = CallFunc::create([=]() {
									isOpeningAnotherScene = false;
								});
						this->runAction(
								Sequence::create(DelayTime::create(1), func, nullptr));

						int index = (int) dynamic_cast<Button*>(pSender)->getTag();
						if(vt_Friends.at(index - 1)->getObjectId().compare(UserDefault::getInstance()->getStringForKey(KEY_WORLD_OJECTID)) == 0)
						{
							CCLog("bambi HomeScene -> go to setting scene - objectId: %s",vt_Friends.at(index - 1)->getObjectId().c_str());
							auto *newScene = SettingScene::scene();
							auto transition = TransitionFade::create(1.0, newScene);
							Director *pDirector = Director::getInstance();
							pDirector->replaceScene(transition);
						} else
						{
							CCLog("bambi HomeScene -> go to trade scene - objectId: %s",vt_Friends.at(index - 1)->getObjectId().c_str());
							auto *newScene = TradingScene::scene(vt_Friends.at(index - 1));
							auto transition = TransitionFade::create(1.0, newScene);
							Director *pDirector = Director::getInstance();
							pDirector->replaceScene(transition);
						}
						instance = nullptr;
					}});
		stickerBtn->setPosition(
				Vec2(positionX + scrollFrameSize.width / 2 - itemMargin / 2,
						positionY - stickerBtn->getContentSize().height / 2));
		scrollview->addChild(stickerBtn);

		//Add user name label
		BLabel* labelName = BLabel::createWithTTF(labelConfig,
				String::createWithFormat("%d --- %s", i,
						vt_Friends.at(i - 1)->getName().c_str())->getCString(),
				TextHAlignment::CENTER);
		labelName->setPosition(
				Vec2(stickerBtn->getContentSize().width / 2,
						stickerBtn->getContentSize().height / 2));
		labelName->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
		labelName->setColor(Color3B::BLACK);
		labelName->setTag(kTagFriendList);
		stickerBtn->addChild(labelName);

		positionY -= itemMargin;
	}
}

void HomeScene::responseAfterGetStickersDataFromFirebase(string facebookId,
		string stickerData, string stickedStickerData) {
	CCLog(
			"bambi HomeScene -> responseAfterGetStickersDataFromFirebase - facebookID: %s, stickerData: %s",
			facebookId.c_str(), stickerData.c_str());
	if (facebookId == sdkbox::PluginFacebook::getUserID()) {
		CCLog(
				"bambi HomeScene -> responseAfterGetStickersDataFromFirebase - this is my facebook id");
		if (FacebookHandler::getInstance()->isFacebookFirstTimeLoggingIn()) {
			CCLog(
					"bambi HomeScene -> responseAfterGetStickersDataFromFirebase - is first time logging in");
			if (stickerData.length() > 0) {
				CCLog(
						"bambi HomeScene -> responseAfterGetStickersDataFromFirebase - has data from server -> save stickers to local");

				UserDefault::getInstance()->setStringForKey(CURRENT_STICKER,
						stickerData);
				UserDefault::getInstance()->setStringForKey(STICKED_STICKER,
						stickedStickerData);
				UserDefault::getInstance()->setBoolForKey(
				KEY_FACEBOOK_FIRST_TIME_LOGGING_IN, false);

				//Update UI
				invalidateProgressBar();
			} else {
				CCLog(
						"bambi HomeScene -> responseAfterGetStickersDataFromFirebase - has no data from server -> save stickers from local to server");
				FirebaseHandler::getInstance()->saveToMyStickedStickerList(
						UserDefault::getInstance()->getStringForKey(
						STICKED_STICKER));
				FirebaseHandler::getInstance()->saveToMyStickerList(
						UserDefault::getInstance()->getStringForKey(
						CURRENT_STICKER));
			}
		}
	}

	//TODO get pending request from server here
	FirebaseHandler::getInstance()->checkPendingRequest();//responseAfterCheckingPendingRequest will be called
	FirebaseHandler::getInstance()->checkGivenStickers();//responseAfterCheckingGivenSticker will be called

}

void HomeScene::responseAfterAskingSticker(int stickerId, bool isSuccess) {
//Required null
}

void HomeScene::responseAfterCheckingGivenSticker(
		vector<PendingRequest*> _vtGivenStickers) {
	vtGivenSticker = _vtGivenStickers;
	btnTrade->setEnabled(vtGivenSticker.size() + vtPendingRequest.size() > 0);

//	for (PendingRequest* request : vtGivenStickers) {
//		CCLog(
//				"bambi HomeScene -> responseAfterCheckingGivenSticker, object id: %s - name: %s - stickerId: %s",
//				request->getObjectId().c_str(), request->getName().c_str(),
//				request->getStickerId().c_str());
//	}
}
void HomeScene::responseAfterCheckingPendingRequest(
		vector<PendingRequest*> _vtPendingRequest) {
	vtPendingRequest = _vtPendingRequest;
	btnTrade->setEnabled(vtGivenSticker.size() + vtPendingRequest.size() > 0);

//	for (PendingRequest* request : vtPendingRequest) {
//		CCLog(
//				"bambi HomeScene -> responseAfterCheckingPendingRequest, object id: %s - name: %s - stickerId: %s",
//				request->getObjectId().c_str(), request->getName().c_str(),
//				request->getStickerId().c_str());
//	}
}

void HomeScene::responseAfterCheckFacebookIdExistOnFirebase() {
	CCLog("bambi HomeScene -> responseAfterCheckFacebookIdExistOnFirebase");
	if (FacebookHandler::getInstance()->isFacebookLoggedIn()) {
//			isGettingData = true;
//			labelError->setVisible(false);
//			loadingSprite->setVisible(true);
		//		if(isWorldMode)
		//			FirebaseHandler::getInstance()->fetchTopWorld();
		//		else
		FirebaseHandler::getInstance()->fetchFriendsFromFacebook();
		//After fetch data responseForQuerryTopWorld or responseForQuerryTopFriend will be called.
	} else {
		CCLog(
				"bambi HomeScene -> responseAfterCheckFacebookIdExistOnFirebase, facebook is not logged in");
//			labelError->setVisible(true);
//			isGettingData = false;
	}
}

void HomeScene::settingButtonsCallback(Ref* pSender,
		ui::Widget::TouchEventType eEventType) {
	if (eEventType == ui::Widget::TouchEventType::ENDED
			&& !isOpeningAnotherScene) {
		isOpeningAnotherScene = true;
		auto func = CallFunc::create([=]() {
			isOpeningAnotherScene = false;
		});
		this->runAction(Sequence::create(DelayTime::create(1), func, nullptr));

		if (backgroundLayer != nullptr && backgroundLayer->isVisible()) {
			this->removeChild(backgroundLayer, false);
			backgroundLayer = nullptr;
		} else {
            if(isSound){
                CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(s_click);
            }
			int tag = (int) dynamic_cast<Button*>(pSender)->getTag();
			switch (tag) {
			case kTagFacebookPage:
				Application::getInstance()->openURL(s_linkToFacebookPage);
				break;
			case kTagMoreGame:
#if(CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
				Application::getInstance()->openURL(s_linkToAppStoreMoreGame);
#endif
#if(CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
				Application::getInstance()->openURL(s_linkToGooglePlayMoreGame);
#endif
				break;
			case kTagRating:
#if(CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
				Application::getInstance()->openURL(s_linkToAppStoreRating);
#endif
#if(CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
				Application::getInstance()->openURL(s_linkToGooglePlayRating);
#endif
				break;
			case kTagSetting: {
				auto *newScene = SettingScene::scene();
				auto transition = TransitionFade::create(1.0, newScene);
				Director *pDirector = Director::getInstance();
				pDirector->replaceScene(transition);
				instance = nullptr;
			}
				break;
			default:
				CCLOG("Error in Menu inflatten");
				break;
			}
		}
	}
}

void HomeScene::onVideoAdsPlayed() {
	StickerHelper::appendAPacketToSharePreferences(STICKER_RARITY::UNKNOWN);
	addPacketFromUserDefault();
}

bool HomeScene::onTouchBegan(Touch* touch, Event* event) {
	if (friendLayer != nullptr && friendLayer->isVisible()) {
		friendLayer->setVisible(false);
	}

	Rect rect = menuBar->getBoundingBox();
	rect.setRect(rect.origin.x, rect.origin.y, rect.size.width, 200);
	if (rect.containsPoint(touch->getLocation())) {
        if(isSound){
            CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(s_click);
        }
        
		isMenuBarShowing = !isMenuBarShowing;
		invalidateMenuBarPosition();
	} else if (backgroundLayer != nullptr && backgroundLayer->isVisible()) {
		this->removeChild(backgroundLayer, false);
		backgroundLayer = nullptr;
	}
	return true;
}

void HomeScene::onInitialized(bool success) {
	CCLog("bambi HomeScene -> IAP onInitialized, success: %s",
			success ? "true" : "false");
}
void HomeScene::onSuccess(const sdkbox::Product& p) {
	CCLog("bambi HomeScene -> IAP onSuccess: %s", p.name.c_str());
	if (p.name == IAP_ANDROID_PACK1_KEY || p.name == IAP_IOS_PACK1_KEY) {
		StickerHelper::appendAPacketToSharePreferences(
				STICKER_RARITY::UNCOMMON);
		iapLayer->setVisible(false);

		addPacketFromUserDefault();
	} else if (p.name == IAP_ANDROID_PACK2_KEY || p.name == IAP_IOS_PACK2_KEY) {
		StickerHelper::appendAPacketToSharePreferences(STICKER_RARITY::COMMON);
		StickerHelper::appendAPacketToSharePreferences(
				STICKER_RARITY::UNCOMMON);
		iapLayer->setVisible(false);

		addPacketFromUserDefault();
	} else if (p.name == IAP_ANDROID_PACK3_KEY || p.name == IAP_IOS_PACK3_KEY) {
		StickerHelper::appendAPacketToSharePreferences(STICKER_RARITY::COMMON);
		StickerHelper::appendAPacketToSharePreferences(
				STICKER_RARITY::UNCOMMON);
		StickerHelper::appendAPacketToSharePreferences(STICKER_RARITY::RARE);
		iapLayer->setVisible(false);

		addPacketFromUserDefault();
    }
}
void HomeScene::onFailure(const sdkbox::Product& p, const std::string& msg) {
	CCLog("bambi HomeScene -> IAP onFailure: %s", p.name.c_str());
}
void HomeScene::onCanceled(const sdkbox::Product& p) {
	CCLog("bambi HomeScene -> IAP onCanceled: %s", p.name.c_str());
}
void HomeScene::onRestored(const sdkbox::Product& p) {
	CCLog("bambi HomeScene -> IAP onRestored: %s", p.name.c_str());
}
void HomeScene::onProductRequestSuccess(
		const std::vector<sdkbox::Product>& products) {
	CCLog("bambi HomeScene -> IAP onProductRequestSuccess");
}
void HomeScene::onProductRequestFailure(const std::string& msg) {
	CCLog("bambi HomeScene -> IAP onProductRequestFailure: %s", msg.c_str());
}
void HomeScene::onRestoreComplete(bool ok, const std::string &msg) {
	CCLog("bambi HomeScene -> IAP onRestoreComplete, ok: %s, msg: %s",
			ok ? "true" : "false", msg.c_str());
}

//TODO exist game if press back twice in 2 seconds
bool firstClickInHomeScene = true;
void HomeScene::onKeyReleased(EventKeyboard::KeyCode keycode, Event* event) {
	if (keycode == EventKeyboard::KeyCode::KEY_ESCAPE) {
		if (firstClickInHomeScene) {
			firstClickInHomeScene = false;
			SocialPlugin::showToast("Press back again to Exit!");

			auto func = CallFunc::create([=]() {
				firstClickInHomeScene = true;
			});
			this->runAction(
					Sequence::create(DelayTime::create(2), func, nullptr));
		} else {
			CCDirector::sharedDirector()->end();
		}
	}
}
//---------------------------------------------------------------------End of callback methods
