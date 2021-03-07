#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <Windows.h>
#include <cocos2d.h>

// Steam API include
#ifdef USE_STEAM
#include <steam_api.h>
#endif

#include "Resource.h"
#include "PTPAppDelegate.h"
#include "PTPConfig.h"
#include "PTPSettingsController.h"

#include "services/PTServices.h"

#include "models/PTModelController.h"
#include "models/PTModelGeneralSettings.h"

#define ARCHIVE_PASSWORD "HSv4c5qmsV4ae6sjn67nXEgv8XLNruFRFyird8mutl8ff/guzv28Wx56/S6UqrNbHX/8dZ+vvVwbK/lwmqy9CA=="

static PTPAppDelegate applicationInstance;

void setFullscreen(bool fullscreen, bool isFirstTime = false) {
    static bool isInFullscreen = false;
    if (!isFirstTime && fullscreen == isInFullscreen)
        return;

    isInFullscreen = fullscreen;

    cocos2d::GLViewImpl *view = static_cast<cocos2d::GLViewImpl*>(cocos2d::Director::getInstance()->getOpenGLView());

    MONITORINFO monitorInfo;
    monitorInfo.cbSize = sizeof(monitorInfo);
    GetMonitorInfo(MonitorFromWindow(view->getWin32Window(), MONITOR_DEFAULTTONEAREST), &monitorInfo);
    RECT windowRect(monitorInfo.rcMonitor);
    int width = windowRect.right - windowRect.left;
    int height = windowRect.bottom - windowRect.top;

    if (fullscreen) {
        view->setFullscreen();
        view->setFrameSize(width, height);
        view->setFrameZoomFactor(1.0f);
    }
    else {
        int windowWidth = width * 0.5;
        int windowHeight = height * 0.5;

        view->setWindowed(windowWidth, windowHeight);

        if (PTModelGeneralSettings::shared()->orientation() == PTModelGeneralSettings::LandscapeOrientation) {
            view->setFrameSize(windowWidth, windowHeight);
        }
        else {
            view->setFrameSize(windowHeight, windowWidth);
        }

        view->setFrameZoomFactor(1.0f);
    }

    if (PTModelGeneralSettings::shared()->orientation() == PTModelGeneralSettings::LandscapeOrientation) {
        view->setDesignResolutionSize(kResolutionWidth, kResolutionHeight, view->getResolutionPolicy());
    }
    else {
        view->setDesignResolutionSize(kResolutionHeight, kResolutionWidth, view->getResolutionPolicy());
    }
}

int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, PWSTR /*pCmdLine*/, int /*nCmdShow*/) {
    PTModelController *mc = PTModelController::shared();
    mc->clean();

    ssize_t dataSize = 0;
    unsigned char* dataBuffer = cocos2d::FileUtils::getInstance()->getFileData("data/data.pkg", "rb", &dataSize);
    if (dataBuffer && dataSize > 0) {
        mc->setUsingDataEncryption(true);

        free(dataBuffer);
    }

    mc->loadDataForSplashScreen("data/data.pkg", ARCHIVE_PASSWORD);

    applicationInstance.setDataArchiveProcessor(ARCHIVE_PASSWORD);
    applicationInstance.setupJsContext();

#ifdef USE_STEAM
    PTLog("Steam initialized: %d", SteamAPI_Init());
#endif

    cocos2d::GLView::setGLContextAttrs(GLContextAttrs({ 8, 8, 8, 8, 24, 8 }));
    cocos2d::GLViewImpl *view = cocos2d::GLViewImpl::createWithRect(PTModelGeneralSettings::shared()->applicationName(), cocos2d::Rect(0, 0, kResolutionWidth, kResolutionHeight));

    cocos2d::Director *director = cocos2d::Director::getInstance();
    director->setOpenGLView(view);
    director->setAnimationInterval(1.0 / 60);

    PTPSettingsController::shared()->load();

    if (PTPSettingsController::shared()->useModelFullscreen()) {
        setFullscreen(PTModelGeneralSettings::shared()->isFullscreen(), true);

        PTPSettingsController::shared()->setUseModelFullscreen(false);
        PTPSettingsController::shared()->setFullscreen(PTModelGeneralSettings::shared()->isFullscreen());
    }
    else {
        setFullscreen(PTPSettingsController::shared()->isFullscreen(), true);
    }

    PTServices::GlobalEventCallback fullscreenCallback = [](std::string type, std::string value) {
        if (type == "PT_Fullscreen") {
            setFullscreen(value == "true");
        }
    };
    PTServices::shared()->addGlobalEventListener(&fullscreenCallback);

    SetClassLong(view->getWin32Window(), GCL_HICON, (LONG)LoadImage(NULL, L"icon.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED));

    int result = applicationInstance.run();
    
    mc->clean();

    return result;
}
