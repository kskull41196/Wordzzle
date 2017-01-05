#include "BaseScene.h"
#include "ShopScene.h"

Scene* BaseScene::scene() {
	// 'scene' is an autorelease object
	Scene *scene = Scene::create();

	// 'layer' is an autorelease object
	BaseScene *layer = BaseScene::create();
	// add layer as a child to scene
	scene->addChild(layer);

	// return the scene
	return scene;
}
void BaseScene::addBottomBanner() {
	Button* btnBottomBanner = Button::create(s_bottom_banner_close_button);
	btnBottomBanner->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	btnBottomBanner->setZoomScale(0);
	btnBottomBanner->addTouchEventListener([this](Ref *pSender,
			Widget::TouchEventType type) {
		if (type == cocos2d::ui::Widget::TouchEventType::ENDED)
		{
			if (isSound) {
				CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(
						s_click);
			}

			auto *newScene = ShopScene::scene(nullptr);
			auto transition = TransitionFade::create(1.0, newScene);
			Director *pDirector = Director::getInstance();
			pDirector->replaceScene(transition);
		}});
	btnBottomBanner->setPosition(
			Vec2(
					winSize.width - btnBottomBanner->getContentSize().width / 2
							- 10, 70));
	this->addChild(btnBottomBanner);
	btnBottomBanner->runAction(
			Sequence::create(DelayTime::create(1),
					MoveTo::create(0.3f,
							Vec2(
									winSize.width
											- btnBottomBanner->getContentSize().width
													/ 2 - 10, 110)), nullptr));

	Sprite* bottomBanner = Sprite::create(s_bottom_banner);
	bottomBanner->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	bottomBanner->setPosition(
			Vec2(winSize.width / 2, bottomBanner->getContentSize().height / 2));
	this->addChild(bottomBanner);
}

void BaseScene::showAdsBanner() {
#ifdef SDKBOX_ENABLED
	sdkbox::PluginAdMob::show(kAdmobBannerAds);
#endif
}
void BaseScene::hideAdsBanner() {
#ifdef SDKBOX_ENABLED
	sdkbox::PluginAdMob::hide(kAdmobBannerAds);
#endif
}

void BaseScene::showFullscreenAds() {
	CCLog("bambi BaseScene -> showFullscreenAds");
#ifdef SDKBOX_ENABLED
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	int random = CppUtils::randomBetween(1,3);
	if(random == 1)
	{
		sdkbox::PluginChartboost::show(kChartboostInstitialAds);
	} else if(random == 2) {
		sdkbox::PluginAdMob::show(kAdmobInstitialAds);
	} else {
		sdkbox::PluginVungle::show(kVungleInstitialAds);
	}
#else
	int random = CppUtils::randomBetween(1,2);
	if(random == 1)
	{
		sdkbox::PluginChartboost::show(kChartboostInstitialAds);
	} else {
		sdkbox::PluginAdMob::show(kAdmobInstitialAds);
	}
#endif
#endif
}

// on "init" you need to initialize your instance
bool BaseScene::init() {
	//////////////////////////////
	// 1. super init first
	if (!LayerColor::initWithColor(Color4B(255, 255, 255, 255))) {
		return false;
	}

	isSound = UserDefault::getInstance()->getBoolForKey(SOUND, true);
	UserDefault::getInstance()->setBoolForKey(SOUND, isSound);
	isNotificationShowing = false;
	origin = Director::getInstance()->getVisibleOrigin();
	winSize = Director::getInstance()->getVisibleSize();

	//Add label notification
	notificationLayer = LayerColor::create(Color4B(0, 0, 0, 255));
	notificationLayer->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	notificationLayer->setContentSize(Size(winSize.width, 150));
	notificationLayer->setPosition(0, winSize.height);
	this->addChild(notificationLayer);

	TTFConfig configLabelError(s_font, 40 * s_font_ratio);
	labelNotification = Label::createWithTTF(configLabelError, "",
			TextHAlignment::CENTER);
	labelNotification->setPosition(
			Vec2(notificationLayer->getContentSize().width / 2,
					notificationLayer->getContentSize().height / 2));
	labelNotification->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	labelNotification->setColor(Color3B::WHITE);
	notificationLayer->addChild(labelNotification);

	return true;
}
void BaseScene::showNotification(string message) {
	if (!isNotificationShowing) {
		labelNotification->setString(message);
		notificationLayer->setColor(Color3B(163, 213, 95));
		MoveBy* moveDown = MoveBy::create(0.3f,
				Vec2(0, -notificationLayer->getContentSize().height));
		MoveBy* moveUp = moveDown->reverse();
		isNotificationShowing = true;
		auto func = CallFunc::create([=]() {
			isNotificationShowing = false;
		});
		this->runAction(
				Sequence::create(moveDown, DelayTime::create(2), moveUp, func,
						nullptr));
	}
}
void BaseScene::showError(string error) {
	if (!isNotificationShowing) {
		labelNotification->setString(error);
		notificationLayer->setColor(Color3B(242, 109, 125));
		MoveBy* moveDown = MoveBy::create(0.3f,
				Vec2(0, -notificationLayer->getContentSize().height));
		MoveBy* moveUp = moveDown->reverse();
		isNotificationShowing = true;
		auto func = CallFunc::create([=]() {
			isNotificationShowing = false;
		});
		this->runAction(
				Sequence::create(moveDown, DelayTime::create(2), moveUp, func,
						nullptr));
	}
}

