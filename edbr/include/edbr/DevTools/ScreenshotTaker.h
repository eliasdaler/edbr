#pragma once

#include <filesystem>

#include <edbr/Graphics/IdTypes.h>

class GfxDevice;

class ScreenshotTaker {
public:
    void setScreenshotDir(std::filesystem::path dir);

    void takeScreenshot(GfxDevice& gfxDevice, ImageId drawImageId);

private:
    std::filesystem::path screenshotDir;
};
