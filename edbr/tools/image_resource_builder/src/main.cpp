#include <filesystem>

#include <CLI/CLI.hpp>

#define CUTE_ASEPRITE_IMPLEMENTATION
#include <cute_aseprite.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <nlohmann/json.hpp>

namespace
{
struct RGBColor {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    std::uint8_t a;
};

void setPixel(std::vector<std::uint8_t>& pixels, int imgWidth, int x, int y, const RGBColor& color)
{
    const auto idx = static_cast<std::size_t>((x + y * imgWidth) * 4);
    pixels[idx + 0] = color.r;
    pixels[idx + 1] = color.g;
    pixels[idx + 2] = color.b;
    pixels[idx + 3] = color.a;
}

struct Rect {
    int x;
    int y;
    int w;
    int h;
};

struct Animation {
    std::string name;
    int startFrame;
    int endFrame;
    int frameDuration; // ms
};

struct SpriteSheet {
    int width;
    int height;
    std::vector<Rect> rects;
};

bool isAsepriteFile(const std::filesystem::path& p)
{
    static const auto asepriteExts = std::array<std::filesystem::path, 2>{".ase", ".aseprite"};
    for (const auto& ext : asepriteExts) {
        if (p.extension() == ext) {
            return true;
        }
    }
    return false;
}

bool isImageFile(const std::filesystem::path& p)
{
    return p.extension() == ".png";
}

void writeAnimationsFile(
    const std::filesystem::path& outAnimPath,
    const std::vector<Animation>& animations,
    const SpriteSheet& spriteSheet);

void processFile(
    const std::filesystem::path& inDir,
    const std::filesystem::path& outAnimDir,
    const std::filesystem::path& outImgDir,
    const std::filesystem::path& path)
{
    const auto relPath = path.lexically_relative(inDir);

    auto outImgPath = outImgDir / relPath;
    outImgPath.replace_extension(".png");

    if (!std::filesystem::exists(outImgPath.parent_path())) {
        std::filesystem::create_directories(outImgPath.parent_path());
    }

    if (!isAsepriteFile(path)) { // just copy as-is, no need for processing
        std::filesystem::copy(path, outImgPath);
        return;
    }

    auto outAnimPath = outAnimDir / relPath;
    outAnimPath.replace_extension(".json");

    if (!std::filesystem::exists(outAnimPath.parent_path())) {
        std::filesystem::create_directories(outAnimPath.parent_path());
    }

    ase_t* ase = cute_aseprite_load_from_file(path.c_str(), NULL);

    // make an image close to square in dimensions
    // e.g. if 16 frames, 4x4 spritesheet will be made
    const auto spriteSheetDim = std::ceil(std::sqrt(ase->frame_count));

    SpriteSheet spriteSheet;
    spriteSheet.width = ase->w * spriteSheetDim;
    spriteSheet.height = ase->w * spriteSheetDim;

    const auto frameWidth = ase->w;
    const auto frameHeight = ase->h;
    const auto numChannels = 4;
    std::vector<std::uint8_t> pixelData(spriteSheet.width * spriteSheet.height * numChannels, 0);

    spriteSheet.rects.reserve(ase->frame_count);

    int startX = 0;
    int startY = 0;
    for (int frameIdx = 0; frameIdx < ase->frame_count; ++frameIdx) {
        // write frame
        const auto& frame = ase->frames[frameIdx];
        for (int y = 0; y < frameHeight; y++) {
            for (int x = 0; x < frameWidth; x++) {
                const auto& pixel = frame.pixels[x + y * ase->w];
                setPixel(
                    pixelData,
                    spriteSheet.width,
                    startX + x,
                    startY + y,
                    RGBColor{pixel.r, pixel.g, pixel.b, pixel.a});
            }
        }

        spriteSheet.rects.push_back(Rect{startX, startY, frameWidth, frameHeight});

        // go to next pixels
        startX += frameWidth;
        if (startX >= spriteSheet.width) { // go to next line
            startX = 0;
            startY += frameHeight;
        }
    }

    const auto res = stbi_write_png(
        outImgPath.string().c_str(),
        spriteSheet.width,
        spriteSheet.height,
        numChannels,
        pixelData.data(),
        spriteSheet.width * numChannels);
    if (res == 0) {
        std::cout << "failed to write image " << outImgPath << std::endl;
    }

    std::vector<Animation> animations;
    animations.reserve(ase->tag_count);
    for (int i = 0; i < ase->tag_count; ++i) {
        const auto& tag = ase->tags[i];

        // TODO: support animations with different frame rates?
        const auto frameDuration = ase->frames[tag.from_frame].duration_milliseconds;

        animations.push_back(Animation{
            .name = tag.name,
            .startFrame = tag.from_frame,
            .endFrame = tag.to_frame,
            .frameDuration = frameDuration,
        });
    }

    writeAnimationsFile(outAnimPath, animations, spriteSheet);

    cute_aseprite_free(ase);
}

void writeAnimationsFile(
    const std::filesystem::path& outAnimPath,
    const std::vector<Animation>& animations,
    const SpriteSheet& spriteSheet)
{
    nlohmann::json root;
    auto& animationsObj = root["animations"];
    for (const auto& animation : animations) {
        auto& animObj = animationsObj[animation.name];
        animObj["startFrame"] = animation.startFrame;
        animObj["endFrame"] = animation.endFrame;
        animObj["frameDuration"] = animation.frameDuration / 1000.f;
    }

    auto spriteSheetArr = nlohmann::json::array();
    for (const auto& r : spriteSheet.rects) {
        spriteSheetArr.push_back(nlohmann::json::array({r.x, r.y, r.w, r.h}));
    }
    root["spriteSheet"]["frames"] = std::move(spriteSheetArr);

    std::ofstream jsonFile(outAnimPath);
    assert(jsonFile.good());
    jsonFile << root << std::endl;
}

}

int main(int argc, char** argv)
{
    CLI::App app{
        "image_resource_builder - a tool for converting .aseprite files to internal engine "
        ".json format and .png spritesheet (.png files are just copied as-is)"};
    argv = app.ensure_utf8(argv);

    std::string inDir;
    std::string outImgDir;
    std::string outAnimDir;

    app.add_option("in", inDir, "Input directory");
    app.add_option("out_anim_dir", outAnimDir, "Output animations directory");
    app.add_option("out_img_dir", outImgDir, "Output images directory");
    app.validate_positionals();

    CLI11_PARSE(app, argc, argv);

    if (inDir.empty() || outAnimDir.empty() || outImgDir.empty()) {
        std::cout << "usage: image_resource_builder IN_DIR OUT_ANIM_DIR OUT_IMAGES_DIR\n";
        std::exit(1);
    }

    if (!std::filesystem::exists(outAnimDir)) {
        std::filesystem::create_directory(outAnimDir);
    } else {
        std::filesystem::remove_all(outAnimDir);
        std::filesystem::create_directory(outAnimDir);
    }

    if (!std::filesystem::exists(outImgDir)) {
        std::filesystem::create_directory(outImgDir);
    } else {
        std::filesystem::remove_all(outImgDir);
        std::filesystem::create_directory(outImgDir);
    }

    for (const auto& p : std::filesystem::recursive_directory_iterator(inDir)) {
        if (std::filesystem::is_regular_file(p)) {
            if (isAsepriteFile(p.path()) || isImageFile(p.path())) {
                processFile(inDir, outAnimDir, outImgDir, p.path());
            }
        }
    }

    return 0;
}
