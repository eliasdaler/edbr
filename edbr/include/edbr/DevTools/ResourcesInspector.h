#pragma once

class ImageCache;
class MaterialCache;

class ResourcesInspector {
public:
    void update(float dt, const ImageCache& imageCache, const MaterialCache& materialCache);
};
