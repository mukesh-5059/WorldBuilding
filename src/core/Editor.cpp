#include "Application.hpp"
#include "ConsoleLog.hpp"
#include "rlimgui/rlImGui.h"
#include "imgui/imgui.h"
#include <cstdio>

int Application::AddTextureViewport(const std::string& initialPath) {
    TextureViewport vp;
    vp.id = nextViewportId++;
    vp.name = "Texture " + std::to_string(vp.id);
    vp.ownsTexture = true;
    if (!initialPath.empty()) {
        snprintf(vp.filePath, sizeof(vp.filePath), "%s", initialPath.c_str());
        if (FileExists(vp.filePath)) {
            vp.texture = LoadTexture(vp.filePath);
            if (vp.texture.id > 0 && vp.texture.width > 0) {
                SetTextureFilter(vp.texture, TEXTURE_FILTER_BILINEAR);
                vp.isLoaded = true;
                vp.loadedPath = vp.filePath;
            }
        }
    }
    textureViewports.push_back(vp);
    return vp.id;
}

int Application::AddTextureViewport(Texture2D texture, const std::string& name, std::function<Texture2D()> reloadCallback, bool ownsTexture, bool canClose) {
    TextureViewport vp;
    vp.id = nextViewportId++;
    vp.name = name.empty() ? ("Texture " + std::to_string(vp.id)) : name;
    vp.texture = texture;
    vp.isLoaded = (texture.id > 0);
    vp.ownsTexture = ownsTexture;
    vp.canClose = canClose;
    vp.reloadCallback = reloadCallback;
    if (vp.isLoaded) {
        SetTextureFilter(vp.texture, TEXTURE_FILTER_BILINEAR);
        vp.loadedPath = "[Raylib Texture2D]";
    }
    textureViewports.push_back(vp);
    return vp.id;
}

void Application::RemoveTextureViewport(int index) {
    if (index >= 0 && index < (int)textureViewports.size()) {
        if (textureViewports[index].isLoaded && textureViewports[index].ownsTexture && textureViewports[index].texture.id > 0) {
            UnloadTexture(textureViewports[index].texture);
        }
        textureViewports[index].isLoaded = false;
        textureViewports.erase(textureViewports.begin() + index);
    }
}

void Application::SetTextureViewportCallback(int viewportId, std::function<Texture2D()> reloadCallback) {
    for (auto& vp : textureViewports) {
        if (vp.id == viewportId) {
            vp.reloadCallback = reloadCallback;
            break;
        }
    }
}

void Application::SetTextureViewportTexture(int viewportId, Texture2D texture, bool ownsTexture) {
    for (auto& vp : textureViewports) {
        if (vp.id == viewportId) {
            if (vp.isLoaded && vp.ownsTexture && vp.texture.id > 0) {
                UnloadTexture(vp.texture);
            }
            vp.texture = texture;
            vp.isLoaded = (texture.id > 0);
            vp.ownsTexture = ownsTexture;
            if (vp.isLoaded) {
                SetTextureFilter(vp.texture, TEXTURE_FILTER_BILINEAR);
            }
            break;
        }
    }
}

void Application::ReloadTextureViewport(TextureViewport& vp) {
    if (vp.reloadCallback) {
        Texture2D newTex = vp.reloadCallback();
        if (newTex.id > 0) {
            if (vp.isLoaded && vp.ownsTexture && vp.texture.id > 0) {
                UnloadTexture(vp.texture);
            }
            vp.texture = newTex;
            vp.isLoaded = true;
            vp.loadedPath = "[Raylib Texture2D Callback]";
            SetTextureFilter(vp.texture, TEXTURE_FILTER_BILINEAR);
            ConsoleLog::Get().AddLog(LogLevel::Info, "Texture viewport '%s' reloaded via callback.", vp.name.c_str());
        }
    } else if (vp.filePath[0] != '\0') {
        if (FileExists(vp.filePath)) {
            if (vp.isLoaded && vp.ownsTexture && vp.texture.id > 0) {
                UnloadTexture(vp.texture);
            }
            vp.texture = LoadTexture(vp.filePath);
            if (vp.texture.id > 0 && vp.texture.width > 0) {
                SetTextureFilter(vp.texture, TEXTURE_FILTER_BILINEAR);
                vp.isLoaded = true;
                vp.loadedPath = vp.filePath;
                ConsoleLog::Get().AddLog(LogLevel::Info, "Texture viewport '%s' reloaded from file '%s'.", vp.name.c_str(), vp.filePath);
            }
        }
    }
}

void Application::ReloadTextureViewport(int viewportId) {
    for (auto& vp : textureViewports) {
        if (vp.id == viewportId) {
            ReloadTextureViewport(vp);
            break;
        }
    }
}

TextureViewport* Application::GetTextureViewport(int viewportId) {
    for (auto& vp : textureViewports) {
        if (vp.id == viewportId) {
            return &vp;
        }
    }
    return nullptr;
}

void Application::performanceGui(){
    ImGui::SetNextWindowPos(ImVec2(20.0f, 40.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(280.0f, 220.0f), ImGuiCond_FirstUseEver);

    ImGui::Begin("Performance");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Frame Time: %.3f ms/frame ", 1000.0f / ImGui::GetIO().Framerate);
    ImGui::SliderInt("Target FPS", &m_targetFps, 30, 300);
    ImGui::Separator();
    ImGui::PlotHistogram("Frame Times", m_frameTimeHistory, 100, m_frameTimeIndex, nullptr, 0.0f, 33.3f, ImVec2(0, 80));
    ImGui::End();
}

void Application::renderResolutionGui() {
    if (ImGui::CollapsingHeader("3D Render Resolution", ImGuiTreeNodeFlags_DefaultOpen)) {
        int curW = sceneRenderTexture.texture.width;
        int curH = sceneRenderTexture.texture.height;

        bool changed = false;
        if (ImGui::SliderInt("Render Width", &curW, 512, 3840, "%d px")) changed = true;
        if (ImGui::SliderInt("Render Height", &curH, 288, 2160, "%d px")) changed = true;

        if (changed) {
            SetRenderResolution(curW, curH);
        }

        ImGui::TextDisabled("Framebuffer: %d x %d px", sceneRenderTexture.texture.width, sceneRenderTexture.texture.height);
    }
}

void Application::editorGui() {
    // Set up Fullscreen Root Window (Strictly Non-Scrollable Container)
    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(mainViewport->WorkPos);
    ImGui::SetNextWindowSize(mainViewport->WorkSize);

    ImGuiWindowFlags rootFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                                 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("EditorRoot", nullptr, rootFlags);
    ImGui::PopStyleVar();

    float totalWidth = mainViewport->WorkSize.x;
    float totalHeight = mainViewport->WorkSize.y;
    float splitterThickness = 6.0f;

    // Enforce resizable inspector width limits
    if (inspectorWidth < 200.0f) inspectorWidth = 200.0f;
    if (inspectorWidth > totalWidth - 300.0f) inspectorWidth = totalWidth - 300.0f;

    float mainAreaWidth = totalWidth - inspectorWidth - splitterThickness;
    if (mainAreaWidth < 200.0f) mainAreaWidth = 200.0f;

    float currentConsoleH = ConsoleLog::Get().IsCollapsed() ? 28.0f : consoleHeight;
    float viewportHeight = totalHeight - currentConsoleH - (ConsoleLog::Get().IsCollapsed() ? 0.0f : splitterThickness);
    if (viewportHeight < 100.0f) viewportHeight = 100.0f;

    // Left Main Area Container (Non-Scrollable Container)
    ImGui::BeginChild("MainAreaPanel", ImVec2(mainAreaWidth, totalHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        // Top Viewport Panel with Tab Bar
        ImGui::BeginChild("ViewportPanel", ImVec2(mainAreaWidth, viewportHeight), false);
            if (ImGui::BeginTabBar("MainViewportTabs", ImGuiTabBarFlags_Reorderable)) {
                // Primary Hardcoded 3D Scene Viewport
                if (ImGui::BeginTabItem("3D Scene Viewport", nullptr, ImGuiTabItemFlags_NoCloseWithMiddleMouseButton)) {
                    ImVec2 availSize = ImGui::GetContentRegionAvail();
                    if (availSize.x > 0 && availSize.y > 0) {
                        rlImGuiImageRenderTextureFit(&sceneRenderTexture, true);
                        is3DViewportHovered = ImGui::IsItemHovered();

                        if (is3DViewportHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                            ImVec2 minP = ImGui::GetItemRectMin();
                            ImVec2 sizeP = ImGui::GetItemRectSize();
                            ImVec2 mouseP = ImGui::GetMousePos();

                            if (sizeP.x > 0.0f && sizeP.y > 0.0f) {
                                float normX = (mouseP.x - minP.x) / sizeP.x;
                                float normY = (mouseP.y - minP.y) / sizeP.y;
                                On3DViewportClicked(Vector2{ normX, normY });
                            }
                        }
                    } else {
                        is3DViewportHovered = false;
                    }
                    ImGui::EndTabItem();
                } else {
                    is3DViewportHovered = false;
                }

                // Dynamic Texture Viewports from Disk
                int removeIndex = -1;
                for (size_t i = 0; i < textureViewports.size(); ++i) {
                    auto& vp = textureViewports[i];
                    std::string tabLabel = vp.name + "###vp_" + std::to_string(vp.id);
                    if (ImGui::BeginTabItem(tabLabel.c_str(), vp.canClose ? &vp.open : nullptr)) {
                        ImGui::Spacing();
                        ImGui::AlignTextToFramePadding();
                        if (vp.reloadCallback != nullptr) {
                            ImGui::Text("Source: Raylib Texture2D (Callback Registered)");
                        } else {
                            ImGui::Text("File Path:");
                            ImGui::SameLine();
                            float buttonGroupWidth = 200.0f;
                            float availableInputWidth = ImGui::GetContentRegionAvail().x - buttonGroupWidth;
                            if (availableInputWidth < 100.0f) availableInputWidth = 100.0f;
                            ImGui::SetNextItemWidth(availableInputWidth);
                            ImGui::InputText(("##path_" + std::to_string(vp.id)).c_str(), vp.filePath, sizeof(vp.filePath));
                            
                            ImGui::SameLine();
                            if (ImGui::Button(("Load##load_" + std::to_string(vp.id)).c_str(), ImVec2(55.0f, 0.0f))) {
                                if (vp.filePath[0] != '\0') {
                                    if (vp.isLoaded && vp.ownsTexture && vp.texture.id > 0) {
                                        UnloadTexture(vp.texture);
                                        vp.isLoaded = false;
                                    }
                                    if (FileExists(vp.filePath)) {
                                        vp.texture = LoadTexture(vp.filePath);
                                        if (vp.texture.id > 0 && vp.texture.width > 0) {
                                            SetTextureFilter(vp.texture, TEXTURE_FILTER_BILINEAR);
                                            vp.isLoaded = true;
                                            vp.ownsTexture = true;
                                            vp.loadedPath = vp.filePath;
                                        }
                                    }
                                }
                            }
                        }

                        if (vp.reloadCallback != nullptr || vp.filePath[0] != '\0') {
                            ImGui::SameLine();
                            if (ImGui::Button(("Reload##reload_" + std::to_string(vp.id)).c_str(), ImVec2(65.0f, 0.0f))) {
                                ReloadTextureViewport(vp);
                            }
                        }

                        ImGui::SameLine();
                        if (ImGui::Button(("Clear##clear_" + std::to_string(vp.id)).c_str(), ImVec2(55.0f, 0.0f))) {
                            if (vp.isLoaded && vp.ownsTexture && vp.texture.id > 0) {
                                UnloadTexture(vp.texture);
                                vp.isLoaded = false;
                                vp.texture = { 0 };
                            } else {
                                vp.isLoaded = false;
                                vp.texture = { 0 };
                            }
                            vp.filePath[0] = '\0';
                            vp.loadedPath.clear();
                        }

                        ImGui::Separator();

                        if (vp.isLoaded && vp.texture.id > 0) {
                            ImGui::Text("Loaded Texture: %s (%d x %d px)", vp.loadedPath.c_str(), vp.texture.width, vp.texture.height);
                            ImVec2 avail = ImGui::GetContentRegionAvail();
                            if (avail.x > 0 && avail.y > 0 && vp.texture.width > 0 && vp.texture.height > 0) {
                                float aspect = (float)vp.texture.width / (float)vp.texture.height;
                                float availAspect = avail.x / avail.y;
                                float w = avail.x;
                                float h = avail.y;
                                if (availAspect > aspect) {
                                    w = avail.y * aspect;
                                } else {
                                    h = avail.x / aspect;
                                }
                                float offsetX = (avail.x - w) * 0.5f;
                                float offsetY = (avail.y - h) * 0.5f;
                                if (offsetX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
                                if (offsetY > 0) ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offsetY);
                                rlImGuiImageSize(&vp.texture, (int)w, (int)h);
                            }
                        } else {
                            ImGui::Spacing();
                            ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "No texture currently loaded.");
                            ImGui::Text("Enter the path to an image file on disk and click 'Load'.");
                            ImGui::Spacing();
                            if (FileExists("generated/polar_point_texture.png")) {
                                if (ImGui::Button("Load Default Generated Texture (generated/polar_point_texture.png)")) {
                                    snprintf(vp.filePath, sizeof(vp.filePath), "generated/polar_point_texture.png");
                                    if (vp.isLoaded) {
                                        UnloadTexture(vp.texture);
                                        vp.isLoaded = false;
                                    }
                                    vp.texture = LoadTexture(vp.filePath);
                                    if (vp.texture.id > 0 && vp.texture.width > 0) {
                                        vp.isLoaded = true;
                                        vp.loadedPath = vp.filePath;
                                    }
                                }
                            }
                        }

                        ImGui::EndTabItem();
                    }

                    if (!vp.open) {
                        removeIndex = (int)i;
                    }
                }

                if (removeIndex >= 0) {
                    RemoveTextureViewport(removeIndex);
                }

                // Plus '+' button tab at end of tab bar to add texture tabs easily
                if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip)) {
                    AddTextureViewport();
                }

                ImGui::EndTabBar();
            }
        ImGui::EndChild(); // ViewportPanel

        // Horizontal Splitter Handle between Viewport and Console Log
        if (!ConsoleLog::Get().IsCollapsed()) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
            ImGui::InvisibleButton("##HResizer", ImVec2(mainAreaWidth, splitterThickness));
            if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            }
            if (ImGui::IsItemActive()) {
                float deltaY = ImGui::GetIO().MouseDelta.y;
                consoleHeight -= deltaY;
                if (consoleHeight < 60.0f) consoleHeight = 60.0f;
                if (consoleHeight > totalHeight - 120.0f) consoleHeight = totalHeight - 120.0f;
            }
            ImU32 hCol = ImGui::GetColorU32(ImGui::IsItemActive() ? ImGuiCol_SeparatorActive : (ImGui::IsItemHovered() ? ImGuiCol_SeparatorHovered : ImGuiCol_Separator));
            ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), hCol);
            ImGui::PopStyleVar();
        }

        // Bottom Resizable Console Log Panel
        float finalConsoleH = totalHeight - viewportHeight - (ConsoleLog::Get().IsCollapsed() ? 0.0f : splitterThickness);
        ImGui::BeginChild("ConsolePanel", ImVec2(mainAreaWidth, finalConsoleH), true);
            ConsoleLog::Get().Draw("Console Log");
        ImGui::EndChild();

    ImGui::EndChild(); // MainAreaPanel

    ImGui::SameLine();

    // Vertical Splitter Handle between Main Area and Inspector Panel
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    ImGui::InvisibleButton("##VResizer", ImVec2(splitterThickness, totalHeight));
    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }
    if (ImGui::IsItemActive()) {
        float deltaX = ImGui::GetIO().MouseDelta.x;
        inspectorWidth -= deltaX;
        if (inspectorWidth < 200.0f) inspectorWidth = 200.0f;
        if (inspectorWidth > totalWidth - 300.0f) inspectorWidth = totalWidth - 300.0f;
    }
    ImU32 vCol = ImGui::GetColorU32(ImGui::IsItemActive() ? ImGuiCol_SeparatorActive : (ImGui::IsItemHovered() ? ImGuiCol_SeparatorHovered : ImGuiCol_Separator));
    ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), vCol);
    ImGui::PopStyleVar();

    ImGui::SameLine();

    // Right Inspector Panel
    ImGui::BeginChild("InspectorPanel", ImVec2(inspectorWidth, totalHeight), true);
        renderResolutionGui();
        DrawUI();
    ImGui::EndChild();

    ImGui::End(); // EditorRoot
}