#include "ConsoleLog.hpp"
#include "raylib/raylib.h"
#include <cstdio>
#include <cstdarg>
#include <ctime>

ConsoleLog::ConsoleLog()
    : autoScroll(true), showInfo(true), showWarning(true), showError(true), showPerformance(true),
      isCollapsed(false), height(180.0f), infoCount(0), warningCount(0), errorCount(0), performanceCount(0) {
    filterBuffer[0] = '\0';
}

ConsoleLog& ConsoleLog::Get() {
    static ConsoleLog instance;
    return instance;
}

void ConsoleLog::AddLog(LogLevel level, const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    time_t rawTime = time(nullptr);
    struct tm* timeInfo = localtime(&rawTime);
    char timeBuf[16];
    if (timeInfo) {
        strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", timeInfo);
    } else {
        snprintf(timeBuf, sizeof(timeBuf), "00:00:00");
    }

    std::lock_guard<std::mutex> lock(logMutex);
    entries.push_back(LogEntry{ level, std::string(timeBuf), std::string(buf) });

    if (level == LogLevel::Info) infoCount++;
    else if (level == LogLevel::Warning) warningCount++;
    else if (level == LogLevel::Error) errorCount++;
    else if (level == LogLevel::Performance) performanceCount++;
}

void ConsoleLog::Clear() {
    std::lock_guard<std::mutex> lock(logMutex);
    entries.clear();
    infoCount = 0;
    warningCount = 0;
    errorCount = 0;
    performanceCount = 0;
}

void ConsoleLog::Draw(const char* title) {
    std::lock_guard<std::mutex> lock(logMutex);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 2.0f));

    // Collapse / Expand toggle button
    if (ImGui::Button(isCollapsed ? "^ Expand Console" : "v Collapse Console")) {
        isCollapsed = !isCollapsed;
    }
    ImGui::SameLine();

    ImGui::TextUnformatted(title);
    ImGui::SameLine();
    ImGui::TextDisabled("(%d logs | %d info, %d warn, %d err, %d perf)",
                        (int)entries.size(), infoCount, warningCount, errorCount, performanceCount);

    if (!isCollapsed) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(140.0f);
        ImGui::InputText("##ConsoleFilter", filterBuffer, sizeof(filterBuffer));

        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            entries.clear();
            infoCount = 0;
            warningCount = 0;
            errorCount = 0;
            performanceCount = 0;
        }

        ImGui::SameLine();
        ImGui::Checkbox("Auto-Scroll", &autoScroll);

        ImGui::SameLine();
        ImGui::Checkbox("Info", &showInfo);
        ImGui::SameLine();
        ImGui::Checkbox("Warn", &showWarning);
        ImGui::SameLine();
        ImGui::Checkbox("Err", &showError);
        ImGui::SameLine();
        ImGui::Checkbox("Perf", &showPerformance);

        ImGui::Separator();

        // Scrollable Log Area
        ImGui::BeginChild("ConsoleScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        for (const auto& item : entries) {
            if (item.level == LogLevel::Info && !showInfo) continue;
            if (item.level == LogLevel::Warning && !showWarning) continue;
            if (item.level == LogLevel::Error && !showError) continue;
            if (item.level == LogLevel::Performance && !showPerformance) continue;

            if (filterBuffer[0] != '\0') {
                if (item.message.find(filterBuffer) == std::string::npos &&
                    item.timestamp.find(filterBuffer) == std::string::npos) {
                    continue;
                }
            }

            ImVec4 color = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
            const char* levelTag = "[INFO]";
            if (item.level == LogLevel::Warning) {
                color = ImVec4(1.0f, 0.84f, 0.0f, 1.0f);
                levelTag = "[WARN]";
            } else if (item.level == LogLevel::Error) {
                color = ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
                levelTag = "[ERR ]";
            } else if (item.level == LogLevel::Performance) {
                color = ImVec4(0.2f, 0.9f, 0.9f, 1.0f);
                levelTag = "[PERF]";
            }

            ImGui::TextDisabled("[%s]", item.timestamp.c_str());
            ImGui::SameLine();
            ImGui::TextColored(color, "%s %s", levelTag, item.message.c_str());
        }

        if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndChild();
    }

    ImGui::PopStyleVar();
}

void RaylibTraceLogCallback(int logLevel, const char *text, va_list args) {
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), text, args);

    LogLevel level = LogLevel::Info;
    if (logLevel == LOG_WARNING) level = LogLevel::Warning;
    else if (logLevel == LOG_ERROR || logLevel == LOG_FATAL) level = LogLevel::Error;

    ConsoleLog::Get().AddLog(level, "%s", buffer);
}
