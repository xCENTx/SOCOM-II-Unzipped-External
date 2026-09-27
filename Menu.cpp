#include "Menu.h"

Menu::Menu()
{
    //  establish overlay elements
    elements.bIsShown = bShowMenu;
    elements.Menu = std::bind(&Menu::Draw, this);
    elements.Shroud = std::bind(&Menu::SHROUD, this);
    elements.Hud = std::bind(&Menu::HUD, this);

    bRunning = true; // Keep the UI alive while waiting for the game/RDRAM.
}

Menu::~Menu()
{
    elements = DxWindow::SOverlay();
    bRunning = false;
}

ImRect Menu::GetImGuiMenuBounds()
{
    const ImVec2& posClone = g_dxWindow->GetCloneWindowPos();   //  get the position of the cloned application window
    const ImVec2& szClone = g_dxWindow->GetCloneWindowSize();   //  get the size of the cloned application window
    const ImVec2& halfClone = szClone * .5;                     //  half application window size

    //  Get Window Size
    ImVec2 szMenu(halfClone);           //  overlay imgui menu window size
    ImVec2 szMenuMax(800.f, 600.f);     //  max overlay imgui menu window size
    szMenu.x = (szMenu.x > szMenuMax.x) ? szMenuMax.x : szMenu.x;
    szMenu.y = (szMenu.y > szMenuMax.y) ? szMenuMax.y : szMenu.y;

    //  Get Window Position
    ImVec2 posMenu = posClone + halfClone - szMenu * .5;   //  overlay imgui menu window position
    return ImRect(posMenu, posMenu + szMenu);
}

ImRect Menu::GetCloneOverlayBounds()
{
    return ImRect(
        g_dxWindow->GetCloneWindowPos(),
        g_dxWindow->GetCloneWindowPos() + g_dxWindow->GetCloneWindowSize()
    );
}

ImRect Menu::GetClientScreenBounds()
{
    return ImRect(
        ImVec2(0, 0),
        ImVec2(g_dxWindow->GetScreenSize())
    );
}

void Menu::Draw()
{
    if (!bShowMenu)
        return;
    
    MainMenu();
}

void Menu::MainMenu()
{
    auto MenuRect = GetImGuiMenuBounds();
    ImGui::SetNextWindowPos(MenuRect.Min);
    ImGui::SetNextWindowSize(MenuRect.GetSize());
    if (!ImGui::Begin("SOCOM", &bShowMenu, 96))
    {
        ImGui::End();
        return;
    }
    auto width = ImGui::GetContentRegionAvail().x;
    auto height = ImGui::GetContentRegionAvail().y;

    //  ESP
    ImGui::Checkbox("ESP", &this->bESP);
    if (this->bESP)
    {
        ImGui::SameLine();
        ImGui::Checkbox("##names", &this->bESPName);
        GUI::Tooltip("NAMES");
        ImGui::SameLine();
        ImGui::Checkbox("##snap_lines", &this->bESPSnap);
        GUI::Tooltip("SNAP LINES");
        ImGui::SameLine();
        ImGui::Checkbox("##box_2D", &this->bESPBox2D);
        GUI::Tooltip("2D BOX");
        ImGui::SameLine();
        ImGui::Checkbox("##bones", &this->bESPBones);
        GUI::Tooltip("BONES");
        ImGui::SameLine();
        ImGui::Checkbox("##bounds", &this->bESPBounds);
        GUI::Tooltip("BOUNDS");
        ImGui::SameLine();
        ImGui::Checkbox("##chams", &this->bESPChams);
        GUI::Tooltip("CHAMS");
        ImGui::SameLine();
        ImGui::Checkbox("##box_health", &this->bESPHealth);
        GUI::Tooltip("HEALTH");

        ImGui::SameLine();
        // ImGui::SetCursorPosX(width * .5);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::SliderFloat("##ESP_DISTANCE", &mESPDist, 0.0f, 100.f, "%.0f");
    }

    ImGui::SetCursorPosY(height - ImGui::GetTextLineHeightWithSpacing() * 2);
    if (ImGui::Button("EXIT", ImGui::GetContentRegionAvail()))
    {
        //  shutdown
        this->bRunning = false;
    }

    ImGui::End();
}

void Menu::SHROUD()
{
    const ImRect& wndw = GetClientScreenBounds();
    ImGui::SetNextWindowPos(wndw.Min);
    ImGui::SetNextWindowSize(wndw.GetSize());
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
    if (!ImGui::Begin("##SHROUDWINDOW", (bool*)true, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs))
    {
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::End();
        return;
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    ImGui::End();
}

void Menu::HUD()
{
    const ImRect& wndw = GetCloneOverlayBounds();
    ImGui::SetNextWindowPos(wndw.Min);
    ImGui::SetNextWindowSize(wndw.GetSize());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.00f, 0.00f, 0.00f, 0.00f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.00f, 0.00f, 0.00f, 0.00f));
    if (!ImGui::Begin("##HUDWINDOW", (bool*)true, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs))
    {
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::End();
        return;
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();


    ImGuiStyle style = ImGui::GetStyle();
    ImDrawList* pDraw = ImGui::GetWindowDrawList();
    auto center = wndw.GetCenter();
    auto top_center = ImVec2({ center.x, wndw.Min.y });

    if (this->bESP)
        RenderCache();
    
    ImGui::End();
}

DxWindow::SOverlay Menu::GetOverlay() { return elements; }

void Menu::UpdateOverlayViewState(bool bState) { elements.bIsShown = bState; }

void Menu::RenderCache()
{
    SOCOM::SGlobalSnapshot cache{};
    {
        std::lock_guard<std::mutex> lock(g_SOCOM->m_cacheMutex);
        cache = g_SOCOM->m_cache;
    } // release lock
    if (!cache.m_bValid) return;
    ImVec2 screen_pos = g_dxWindow->GetCloneWindowPos();
    ImVec2 screen_size = g_dxWindow->GetCloneWindowSize();
    ImRect screen_rect(screen_pos, screen_pos + screen_size);
    ImVec2 screen_center = screen_rect.GetCenter();
    ImDrawList* pDraw = ImGui::GetWindowDrawList();
    Engine::Vec2 szScreen = { screen_size.x , screen_size.y };

    /* PICKUPS */
    if (this->bESPPickups)
    {
        for (auto& obj : cache.m_pickups)
        {
            auto ent_origin = obj.m_pos;
            const auto distance = cache.m_camera.m_mtxModel.Translate().Distance(ent_origin);
            if (distance > this->mESPDist)
                continue;

            Engine::Vec2 screen;
            Engine::Vec2 screen_view;
            if (Engine::zdb::Tools::Transform::WorldToScreen(ent_origin, cache.m_camera, szScreen, &screen) == false)
                continue;

            const ImVec2 pos = screen_pos + ImVec2(screen.x, screen.y);

            //  SNAP
            if (this->bESPSnap)
                GUI::CleanLine(pos, screen_center, IM_COL32_WHITE);

            //  NAME
            if (this->bESPName)
            {
                char buf_dist[16];
                sprintf_s(buf_dist, "[%.0fm]", distance);
                std::string nameDist(buf_dist);
                std::string nameEnt = obj.m_name;
                ImVec2 szTextDist = ImGui::CalcTextSize(nameDist.c_str());
                ImVec2 szTextName = ImGui::CalcTextSize(nameEnt.c_str());
                ImVec2 posTextName = ImVec2(pos.x - (szTextName.x * .5f), pos.y - (szTextName.y * 0.5f));
                ImVec2 posTextDist = ImVec2(pos.x - (szTextDist.x * .5f), pos.y + (szTextName.y * 0.5f));
                GUI::DrawText_(posTextDist, IM_COL32_WHITE, nameDist);
                GUI::DrawBGText(posTextName, IM_COL32_WHITE, nameEnt, IM_COL32(52, 98, 235, 100));
            }
        }
        
    }

    /* PLAYERS */
    if (this->bESPPlayers)
    {
        bool bInit = false;
        for (auto& obj : cache.m_players)
        {
            if (!obj.m_bAlive)
                continue;

            auto ent_origin = obj.m_pos;
            const auto distance = cache.m_camera.m_mtxModel.Translate().Distance(ent_origin);
            const bool bIsProne = obj.m_stance == Engine::zdb::Enums::ESTANCE_PRONE;
            if (distance > this->mESPDist)
                continue;

            //  calc head pos
            auto ent_headOrigin = ent_origin;
            obj.m_stance == 0 ? ent_headOrigin.y += 20.f : obj.m_stance == 1 ? ent_headOrigin.y += 14.f : ent_headOrigin.y += 3.0f;

            Engine::Vec2 screen;
            Engine::Vec2 screenHead;
            if (Engine::zdb::Tools::Transform::WorldToScreen(ent_origin, cache.m_camera, szScreen, &screen) == false ||
                Engine::zdb::Tools::Transform::WorldToScreen(ent_headOrigin, cache.m_camera, szScreen, &screenHead) == false
                )
                continue;

            if (this->bESPBounds)
                GUI::DrawPlayerBounds(obj, cache.m_camera, IM_COL32_WHITE);

            if (this->bESPChams)
                GUI::DrawPlayerBoneBounds( obj, cache.m_camera, IM_COL32(255, 0, 0, 70), 1.5f);
            
            if (this->bESPBones)
                GUI::DrawPlayerSkeleton(obj, cache.m_camera, IM_COL32_WHITE);

            const ImVec2 pos = screen_pos + ImVec2(screen.x, screen.y);
            const ImVec2 head_pos = screen_pos + ImVec2(screenHead.x, screenHead.y);
            const float corner_height = abs(head_pos.y - pos.y);		                                    //	Width
            const float corner_width = corner_height * 0.65;			                                    //	Height
            const ImVec2 pos_box(head_pos.x - (corner_width / 2), head_pos.y);	                        //	Top Left Corner
            const ImRect bbox(pos_box, ImVec2(pos_box.x + corner_width, pos_box.y + corner_height));      //  2d bounding box

            //  SNAP
            if (this->bESPSnap)
                GUI::CleanLine(pos, screen_center, IM_COL32_WHITE);

            //  BOX
            if (!bIsProne && this->bESPBox2D) // draw 2d box but skip prone players
                pDraw->AddRect(bbox.Min, bbox.Max, IM_COL32_WHITE, 0.f, 0, 1.f);

            //	HEALTH
            if (!bIsProne && this->bESPHealth)
            {
                ImColor mColHealth(255 - obj.m_health * 2.55, obj.m_health * 2.55, 0);					//	health color
                float heightBarHP = bbox.GetHeight() - (bbox.GetHeight() * (obj.m_health / 100.f));	//	health bar height
                auto lBar = ImRect(bbox.Min, ImVec2(bbox.Min.x + 1.f, bbox.Max.y));					//	left healthbar
                pDraw->AddRect(lBar.Min, lBar.Max, IM_COL32_WHITE);
                pDraw->AddRect(ImVec2(lBar.Min.x, lBar.Min.y + heightBarHP), lBar.Max, mColHealth);
            }

            //  NAME
            if (this->bESPName)
            {
                char buf_dist[16];
                sprintf_s(buf_dist, "[%.0fm]", distance);
                std::string nameDist(buf_dist);
                std::string nameEnt = obj.m_name;
                ImVec2 szTextDist = ImGui::CalcTextSize(nameDist.c_str());
                ImVec2 szTextName = ImGui::CalcTextSize(nameEnt.c_str());
                ImVec2 posTextName = ImVec2(pos.x - (szTextName.x * .5f), pos.y);
                ImVec2 posTextDist = ImVec2(pos.x - (szTextDist.x * .5f), pos.y + (szTextName.y * 1.5f));
                GUI::DrawBGText(posTextName, IM_COL32_WHITE, nameEnt, IM_COL32(235, 98, 52, 100));
                GUI::DrawText_(posTextDist, IM_COL32_WHITE, nameDist);
            }
        }
    }
}

void GUI::TextCentered(const char* pText)
{
    ImVec2 textSize = ImGui::CalcTextSize(pText);
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImVec2 textPos = ImVec2((windowSize.x - textSize.x) * 0.5f, (windowSize.y - textSize.y) * 0.5f);
    ImGui::SetCursorPos(textPos);
    ImGui::Text("%s", pText);
}

void GUI::Tooltip(const char* tip)
{
    if (!ImGui::IsItemHovered())
        return;

    ImGui::SetTooltip(tip);
}

void GUI::DrawText_(const ImVec2& pos, const ImColor& color, const std::string& text, const float& szFont)
{
    ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), szFont, pos, color, text.c_str(), text.c_str() + text.size(), 800.f, nullptr);
}

void GUI::DrawBGText(const ImVec2& pos, const ImColor& color, const std::string& text, const ImColor& background, const float& szFont)
{
    auto pFont = ImGui::GetFont();
    const ImVec2& textSize = ImGui::CalcTextSize(text.c_str());
    ImRect textRect = ImRect(pos, pos + textSize);
    if (szFont > 0.f)
    {
        const ImVec2& scaledTextSize = ImVec2(textSize.x * szFont / pFont->FontSize, szFont);
        ImVec2 scaledTextPos = ImVec2(pos.x - (scaledTextSize.x * .5f), pos.y);
    }
    ImGui::GetWindowDrawList()->AddRectFilled(textRect.Min, textRect.Max, background);
    DrawText_(textRect.Min, color, text, szFont);
}

void GUI::DrawBorderText(const ImVec2& pos, const ImColor& color, const std::string& text, const ImColor& border, const float& szFont)
{
    auto pFont = ImGui::GetFont();
    const ImVec2& textSize = ImGui::CalcTextSize(text.c_str());
    ImRect textRect = ImRect(pos, pos + textSize);
    if (szFont > 0.f)
    {
        const ImVec2& scaledTextSize = ImVec2(textSize.x * szFont / pFont->FontSize, szFont);
        ImVec2 scaledTextPos = ImVec2(pos.x - (scaledTextSize.x * .5f), pos.y);
		textRect = (ImRect(scaledTextPos, scaledTextPos + scaledTextSize));
    }
    ImGui::GetWindowDrawList()->AddRect(textRect.Min, textRect.Max, border);
    DrawText_(textRect.Min, color, text, szFont);
}

void GUI::DrawTextCentered(const ImVec2& pos, const ImColor& color, const std::string& text, const float& szFont)
{
    const ImVec2& textSize = ImGui::CalcTextSize(text.c_str());
    ImVec2 textPosition = ImVec2(pos.x - (textSize.x * 0.5f), pos.y);
    if (szFont <= 0.f)
    {
        DrawText_(textPosition, color, text, szFont);
        return;
    }

    auto pFont = ImGui::GetFont();
    ImVec2 scaledTextSize = ImVec2(textSize.x * szFont / pFont->FontSize, szFont);
    ImVec2 scaledTextPos = ImVec2(pos.x - (scaledTextSize.x * .5f), pos.y);
    DrawText_(scaledTextPos, color, text, szFont);
}

void GUI::DrawBGTextCentered(const ImVec2& pos, const ImColor& color, const std::string& text, const ImColor& background, const float& szFont)
{
    const ImVec2& textSize = ImGui::CalcTextSize(text.c_str());
    ImVec2 textPosition = ImVec2(pos.x - (textSize.x * 0.5f), pos.y);
    if (szFont <= 0.f)
    {
        DrawBGText(textPosition, color, text, background, szFont);
        return;
    }

    auto pFont = ImGui::GetFont();
    ImVec2 scaledTextSize = ImVec2(textSize.x * szFont / pFont->FontSize, szFont);
    ImVec2 scaledTextPos = ImVec2(pos.x - (scaledTextSize.x * .5f), pos.y);
    ImGui::GetWindowDrawList()->AddRectFilled(scaledTextPos, scaledTextPos + scaledTextSize, background);
    DrawText_(scaledTextPos, color, text, szFont);
}

void GUI::DrawBorderTextCentered(const ImVec2& pos, const ImColor& color, const std::string& text, const ImColor& border, const float& szFont)
{
    const ImVec2& textSize = ImGui::CalcTextSize(text.c_str());
    ImVec2 textPosition = ImVec2(pos.x - (textSize.x * 0.5f), pos.y);
    if (szFont <= 0.f)
    {
        DrawBorderText(textPosition, color, text, border, szFont);
        return;
    }

    auto pFont = ImGui::GetFont();
    ImVec2 scaledTextSize = ImVec2(textSize.x * szFont / pFont->FontSize, szFont);
    ImVec2 scaledTextPos = ImVec2(pos.x - (scaledTextSize.x * .5f), pos.y);
    ImGui::GetWindowDrawList()->AddRect(scaledTextPos, scaledTextPos + scaledTextSize, border);
    DrawText_(scaledTextPos, color, text, szFont);
}

void GUI::Line(const ImVec2& posA, const ImVec2& posB, const ImColor& color, const float& thickness)
{
    ImGui::GetWindowDrawList()->AddLine(posA, posB, color, thickness);
}

void GUI::Circle(const ImVec2& pos, const ImColor& color, const float& radius, const float& thickness, const float& segments)
{
    ImGui::GetWindowDrawList()->AddCircle(pos, radius, color, segments, thickness);
}

void GUI::CleanLine(const ImVec2& posA, const ImVec2& posB, const ImColor& color, const float& thickness)
{
    Line(posA, posB, ImColor(0.0f, 0.0f, 0.0f, color.Value.w), (thickness + 0.25));
    Line(posA, posB, ImColor(1.0f, 1.0f, 1.0f, color.Value.w), (thickness + 0.15));
    Line(posA, posB, color, thickness);
}

void GUI::CleanCircle(const ImVec2& pos, const ImColor& color, const float& radius, const float& thickness, const float& segments)
{
    Circle(pos, ImColor(0.0f, 0.0f, 0.0f, color.Value.w), radius, thickness, segments);
    Circle(pos, ImColor(1.0f, 1.0f, 1.0f, color.Value.w), radius, thickness, segments);
    Circle(pos, color, radius, thickness, segments);
}

void GUI::DrawPlayerSkeleton(const SOCOM::SImGuiPlayer& player, Engine::zdb::Classes::CZCamera camera, const ImColor& color)
{
    ImVec2 screen_pos = g_dxWindow->GetCloneWindowPos();
    ImVec2 screen_size = g_dxWindow->GetCloneWindowSize();
    Engine::Vec2 szScreen = { screen_size.x , screen_size.y };
    for (int limb = 0; limb < Engine::zdb::BONE_CHAIN_COUNT; limb++)
    {
        Engine::Vec2 previousScreen;
        bool previousVisible = false;
        bool havePrevious = false;

        for (int i = 0; i < 6; i++)
        {
            int boneIndex = Engine::zdb::cs_BoneChains[limb][i];

            if (boneIndex == BONE_INVALID)
                break;

            if (!player.m_bBoneValid[boneIndex])
            {
                havePrevious = false;
                continue;
            }

            const Engine::Vec3& world = player.m_bones[boneIndex];

            Engine::Vec2 screen;
            bool visible = Engine::zdb::Tools::Transform::WorldToScreen(
                    world,
                    camera,
                    szScreen,
                    &screen
                );

            if (havePrevious && previousVisible && visible)
            {
                CleanLine(
                    ImVec2(previousScreen.x, previousScreen.y) + screen_pos,
                    ImVec2(screen.x, screen.y) + screen_pos,
                    color
                );
            }

            previousScreen = screen;
            previousVisible = visible;
            havePrevious = true;
        }
    }
}

void GUI::DrawPlayerBounds(const SOCOM::SImGuiPlayer& player, Engine::zdb::Classes::CZCamera camera, const ImColor& color)
{
    bool visible[8]{};
    Engine::Vec2 screen[8];
    ImVec2 screen_pos = g_dxWindow->GetCloneWindowPos();
    ImVec2 screen_size = g_dxWindow->GetCloneWindowSize();
    Engine::Vec2 szScreen = { screen_size.x , screen_size.y };

    for (int i = 0; i < 8; i++)
    {
        if (!player.m_boundsValid[i])
            continue;

        visible[i] = Engine::zdb::Tools::Transform::WorldToScreen(
                player.m_bounds[i],
                camera,
                szScreen,
                &screen[i]
            );
    }

    for (int i = 0; i < 12; i++)
    {
        int a = Engine::zdb::cs_BoxVerts[i][0];
        int b = Engine::zdb::cs_BoxVerts[i][1];

        if (visible[a] && visible[b])
        {
            CleanLine(
                ImVec2(screen[a].x, screen[a].y) + screen_pos,
                ImVec2(screen[b].x, screen[b].y) + screen_pos,
                color
            );
        }
    }
}

void GUI::DrawPlayerBoneBounds(const SOCOM::SImGuiPlayer& player, Engine::zdb::Classes::CZCamera camera, const ImColor& color, float size)
{
    ImVec2 screen_pos = g_dxWindow->GetCloneWindowPos();
    ImVec2 screen_size = g_dxWindow->GetCloneWindowSize();
    Engine::Vec2 szScreen = { screen_size.x , screen_size.y };

    for (int bone = 0; bone < Engine::zdb::Enums::FT_BONE_MAX; bone++)
    {
        if (!player.m_bBoneBoundsValid[bone])
            continue;

        // Only draw bones used by our skeleton chains
        bool used = false;

        for (int chain = 0; chain < Engine::zdb::BONE_CHAIN_COUNT && !used; chain++)
        {
            for (int i = 0; i < 6; i++)
            {
                const int idx = Engine::zdb::cs_BoneChains[chain][i];

                if (idx == BONE_INVALID)
                    break;

                if (idx == bone)
                {
                    used = true;
                    break;
                }
            }
        }

        if (!used)
            continue;

        bool visible[8]{};
        Engine::Vec3 center{};
        Engine::Vec2 screen[8];
        for (int v = 0; v < 8; v++)
        {
            visible[v] = Engine::zdb::Tools::Transform::WorldToScreen( player.m_boneBounds[bone][v], camera, szScreen, &screen[v]);

            /* debug */
            center += player.m_boneBounds[bone][v];
        }

        // chams
        for (int face = 0; face < 6; face++)
        {
            int a = Engine::zdb::cs_BoxFaces[face][0];
            int b = Engine::zdb::cs_BoxFaces[face][1];
            int c = Engine::zdb::cs_BoxFaces[face][2];
            int d = Engine::zdb::cs_BoxFaces[face][3];

            if (!visible[a] || !visible[b] || !visible[c] || !visible[d])
                continue;

            ImGui::GetBackgroundDrawList()->AddQuadFilled(
                ImVec2(screen[a].x, screen[a].y) + screen_pos,
                ImVec2(screen[b].x, screen[b].y) + screen_pos,
                ImVec2(screen[c].x, screen[c].y) + screen_pos,
                ImVec2(screen[d].x, screen[d].y) + screen_pos,
                color
            );
        }

        // wireframe
        for (int edge = 0; edge < 12; edge++)
        {
            const int a = Engine::zdb::cs_BoxVerts[edge][0];
            const int b = Engine::zdb::cs_BoxVerts[edge][1];

            if (!visible[a] || !visible[b])
                continue;

            CleanLine(
                screen_pos + ImVec2(screen[a].x, screen[a].y),
                screen_pos + ImVec2(screen[b].x, screen[b].y),
                ImColor(1.0f, 1.0f, 1.0f, color.Value.w)
            );
        }
    }
}