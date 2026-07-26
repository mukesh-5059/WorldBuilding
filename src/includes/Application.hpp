#pragma once
#include "raylib/raylib.h"
#include "ConsoleLog.hpp"
#include <vector>
#include <functional>
#include <string>

struct TextureViewport {
    int id = 0;
    std::string name;
    char filePath[512] = "";
    std::string loadedPath;
    Texture2D texture = { 0 };
    bool isLoaded = false;
    bool open = true;
};

class Application {
protected:
    int width;
    int height;
    const char* title;
    bool running;
    
    int m_targetFps;
    float m_frameTimeHistory[100];
    int m_lastTargetFps;
    int m_frameTimeIndex;

    RenderTexture2D sceneRenderTexture;
    std::vector<TextureViewport> textureViewports;
    int nextViewportId;
    bool is3DViewportHovered;

    float inspectorWidth;
    float consoleHeight;

    void performanceGui();

public:
    Application(int width = 1280, int height = 720, const char* title = "WorldBuilder");
    virtual ~Application();

    void Run();
    void AddTextureViewport(const std::string& initialPath = "");
    void RemoveTextureViewport(int index);

protected:
    virtual void Init() {}
    virtual void Update(float deltaTime) {}
    virtual void SceneDraw() {}
    virtual void DrawUI() {}
    virtual void Shutdown() {}
};
