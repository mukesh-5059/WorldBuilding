#pragma once
#include "raylib/raylib.h"
#include <vector>
#include <string>
#include <functional>

struct TextureViewport {
    int id = 0;
    std::string name;
    char filePath[512] = "";
    std::string loadedPath;
    Texture2D texture = { 0 };
    bool isLoaded = false;
    bool open = true;
    bool canClose = true;
    bool ownsTexture = true;
    std::function<Texture2D()> reloadCallback = nullptr;
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
    void renderResolutionGui();
    void SetRenderResolution(int newWidth, int newHeight);
    void editorGui();

    int AddTextureViewport(const std::string& initialPath = "");
    int AddTextureViewport(Texture2D texture, const std::string& name = "", std::function<Texture2D()> reloadCallback = nullptr, bool ownsTexture = false, bool canClose = true);
    void RemoveTextureViewport(int index);
    void SetTextureViewportCallback(int viewportId, std::function<Texture2D()> reloadCallback);
    void SetTextureViewportTexture(int viewportId, Texture2D texture, bool ownsTexture = false);
    void ReloadTextureViewport(int viewportId);
    void ReloadTextureViewport(TextureViewport& vp);
    TextureViewport* GetTextureViewport(int viewportId);

    virtual void Init() {}
    virtual void Update(float deltaTime) {}
    virtual void SceneDraw() {}
    virtual void DrawUI() {}
    virtual void On3DViewportClicked(Vector2 mouseNormInViewport) {}
    virtual void Shutdown() {}

public:
    Application(int width = 1280, int height = 720, const char* title = "WorldBuilder");
    virtual ~Application();

    void Run();
};
