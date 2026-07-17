#pragma once

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

    void performanceGui();

public:
    Application(int width = 1280, int height = 720, const char* title = "WorldBuilder");
    virtual ~Application();

    void Run();

protected:
    virtual void Init() {}
    virtual void Update(float deltaTime) {}
    virtual void Draw() {}
    virtual void DrawUI() {}
    virtual void Shutdown() {}
};
