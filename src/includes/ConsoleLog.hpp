#pragma once
#include <string>
#include <vector>
#include <mutex>
#include "imgui/imgui.h"

enum class LogLevel {
    Info,
    Warning,
    Error,
    Performance
};

struct LogEntry {
    LogLevel level;
    std::string timestamp;
    std::string message;
};

class ConsoleLog {
private:
    std::vector<LogEntry> entries;
    std::mutex logMutex;
    bool autoScroll;
    char filterBuffer[256];
    bool showInfo;
    bool showWarning;
    bool showError;
    bool showPerformance;
    bool isCollapsed;
    float height;

    int infoCount;
    int warningCount;
    int errorCount;
    int performanceCount;

public:
    ConsoleLog();
    ~ConsoleLog() = default;

    static ConsoleLog& Get();

    void AddLog(LogLevel level, const char* fmt, ...) IM_FMTARGS(3);
    void Clear();
    void Draw(const char* title = "Console Log");

    bool IsCollapsed() const { return isCollapsed; }
    void SetCollapsed(bool collapsed) { isCollapsed = collapsed; }
    float GetHeight() const { return isCollapsed ? 28.0f : height; }
    void SetHeight(float h) { height = h; }
};

void RaylibTraceLogCallback(int logLevel, const char *text, va_list args);
