#include "CommandParser.hpp"
#include "Player.hpp"
#include "Item.hpp"
#include "Room.hpp"
#include "FileManager.hpp"
#include <unordered_map>
#include <iostream>
#include "Renderer.h"
#include <chrono>
#include <sstream>
#include <vector>
#include <functional>
#include <cmath>
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/error/en.h"
#include <fstream>
#include "InputHandler.h"
#include "raylib.h"

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif

// Capture cout to lines for Raylib display
std::vector<std::string> captureOutput(std::function<void()> func) {
    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    func();
    std::cout.rdbuf(old);
    std::istringstream iss(oss.str());
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty()) lines.push_back(line);
    }
    return lines;
}

// ── Global game context ───────────────────────────────────────────────────────

struct GameContext {
    Renderer* renderer = nullptr;
    Renderer::GameState state = Renderer::GameState::MENU;
    int menuIndex = 0;
    bool needsInit = false;

    FileManager* fm = nullptr;
    std::unordered_map<std::string, Item>* allItems = nullptr;
    std::unordered_map<std::string, Room>* allRooms = nullptr;
    Player* player = nullptr;
    CommandParser* parser = nullptr;
    InputHandler* inputHandler = nullptr;
    std::vector<std::string>* displayLines = nullptr;
    bool* running = nullptr;
    std::string userInput;
    bool wasTyping = false;

    void resetGame() {
        delete parser;       parser = nullptr;
        delete inputHandler; inputHandler = nullptr;
        delete player;       player = nullptr;
        delete allRooms;     allRooms = nullptr;
        delete allItems;     allItems = nullptr;
        delete fm;           fm = nullptr;
        delete displayLines; displayLines = nullptr;
        delete running;      running = nullptr;
        userInput.clear();
        wasTyping = false;
    }

    bool initGame() {
        resetGame();

        fm = new FileManager();
        allItems = new std::unordered_map<std::string, Item>(fm->loadItems("items.json"));
        allRooms = new std::unordered_map<std::string, Room>(fm->loadRooms("rooms.json", *allItems));

        if (allRooms->empty()) return false;

        auto startIt = allRooms->find("cell_1");
        if (startIt == allRooms->end()) return false;

        player = new Player("Ferengate");
        player->setCurrentRoom(&startIt->second);
        running = new bool(true);
        displayLines = new std::vector<std::string>();
        parser = new CommandParser(player, *running, fm, renderer, allRooms);
        inputHandler = new InputHandler();

        renderer->SetDayProgress(0.0f);

        std::vector<std::string> startingLines = captureOutput([&]() {
            parser->writeMessage(allRooms->at("cell_1").describeSelf());
            });
        startingLines.erase(std::remove_if(startingLines.begin(), startingLines.end(),
            [](const std::string& s) { return s.empty(); }), startingLines.end());
        for (const auto& line : startingLines) {
            if (!line.empty()) displayLines->push_back(line);
        }
        renderer->SnapToBottom();

        needsInit = false;
        return true;
    }
};

static GameContext gCtx;

// ── Per-frame callback ────────────────────────────────────────────────────────
// Takes void* to match emscripten_set_main_loop_arg's expected signature.
// The arg is unused — all state lives in gCtx.

extern "C" void GameFrame(void* arg) {
    Renderer& renderer = *gCtx.renderer;
    const int MENU_ITEMS = 3;

    renderer.UpdateDayProgress();

    // ── MENU ─────────────────────────────────────────────────────────────────
    if (gCtx.state == Renderer::GameState::MENU) {
        if (IsKeyPressed(KEY_UP))   gCtx.menuIndex = (gCtx.menuIndex - 1 + MENU_ITEMS) % MENU_ITEMS;
        if (IsKeyPressed(KEY_DOWN)) gCtx.menuIndex = (gCtx.menuIndex + 1) % MENU_ITEMS;

        if (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (gCtx.menuIndex == 0) { gCtx.state = Renderer::GameState::PLAYING; gCtx.needsInit = true; }
            if (gCtx.menuIndex == 1) gCtx.state = Renderer::GameState::HOW_TO_PLAY;
        #ifndef PLATFORM_WEB
            if (gCtx.menuIndex == 2) { CloseWindow(); return; }
        #endif
        }

        int panelW = 500, panelH = 400;
        int panelX = GetScreenWidth() / 2 - panelW / 2;
        int panelY = GetScreenHeight() / 2 - panelH / 2;
        int optionStartY = panelY + 130;
        int optionSpacing = 70;
        int optionSize = 36;
        const char* options[] = { "New Game", "How to Play", "Quit" };
        for (int i = 0; i < MENU_ITEMS; i++) {
            float optY = (float)(optionStartY + i * optionSpacing);
            Rectangle optRect = { (float)(GetScreenWidth() / 2 - 200), optY, 400.0f, (float)optionSpacing };
            if (CheckCollisionPointRec(GetMousePosition(), optRect)) {
                gCtx.menuIndex = i;
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    if (i == 0) { gCtx.state = Renderer::GameState::PLAYING; gCtx.needsInit = true; }
                    if (i == 1) gCtx.state = Renderer::GameState::HOW_TO_PLAY;
#ifndef PLATFORM_WEB
                    if (i == 2) { CloseWindow(); return; }
#endif
                }
            }
        }

        BeginDrawing();
        renderer.DrawBackground();
        renderer.DrawMenuScreen(gCtx.menuIndex);
        EndDrawing();
        return;
    }

    // ── HOW TO PLAY ───────────────────────────────────────────────────────────
    if (gCtx.state == Renderer::GameState::HOW_TO_PLAY) {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) {
            gCtx.state = Renderer::GameState::MENU;
        }
        BeginDrawing();
        renderer.DrawBackground();
        renderer.DrawHowToPlayScreen();
        EndDrawing();
        return;
    }

    // ── END SCREENS ───────────────────────────────────────────────────────────
    if (gCtx.state == Renderer::GameState::DEAD || gCtx.state == Renderer::GameState::WIN) {
        bool won = (gCtx.state == Renderer::GameState::WIN);
        if (IsKeyPressed(KEY_ENTER)) {
            gCtx.menuIndex = 0;
            gCtx.state = Renderer::GameState::PLAYING;
            gCtx.needsInit = true;
        }
#ifndef PLATFORM_WEB
        if (IsKeyPressed(KEY_ESCAPE)) { CloseWindow(); return; }
#endif

        BeginDrawing();
        renderer.DrawBackground();
        renderer.DrawEndScreen(won);
        EndDrawing();
        return;
    }

    // ── PLAYING ───────────────────────────────────────────────────────────────
    if (gCtx.state == Renderer::GameState::PLAYING) {

        if (gCtx.needsInit) {
            if (!gCtx.initGame()) {
                std::cout << "ERROR: Failed to initialize game.\n";
                gCtx.state = Renderer::GameState::MENU;
                return;
            }
        }

        CommandParser& parser = *gCtx.parser;
        InputHandler& inputHandler = *gCtx.inputHandler;
        std::vector<std::string>& displayLines = *gCtx.displayLines;
        bool& running = *gCtx.running;

        renderer.UpdateScrollInput();
        renderer.UpdateTypingAnimation(GetFrameTime());
        renderer.UpdateMusic();

        bool currentlyTyping = renderer.IsTypingActive();
        if (gCtx.wasTyping && !currentlyTyping) {
            const auto& completedLines = renderer.GetLastTypedLines();
            for (const auto& line : completedLines) {
                if (!line.empty()) displayLines.push_back(line);
            }
            renderer.SnapToBottom();
        }
        gCtx.wasTyping = currentlyTyping;

        if (parser.gameOver) {
            if (!renderer.IsTypingActive()) {
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_SPACE)) {
                    gCtx.state = parser.gameWon
                        ? Renderer::GameState::WIN
                        : Renderer::GameState::DEAD;
                }
            }
        }
        else if (running) {
            if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

            // AUDIO MENU
            if (IsKeyPressed(KEY_F2)) {
                renderer.ToggleMusicPanel();
                renderer.StartMusic();
            }
            if (renderer.IsMusicPanelOpen() && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                int panelW = 380;
                int panelX = GetScreenWidth() - panelW - 20;
                int panelY = GetScreenHeight() / 2 - 520 / 2;
                int trackSpacing = 32;
                int listStartY = panelY + 75;

                for (int i = 0; i < 14; i++) {
                    Rectangle trackRect = {
                        (float)panelX + 16,
                        (float)(listStartY + i * trackSpacing),
                        (float)panelW - 32,
                        (float)trackSpacing
                    };
                    if (CheckCollisionPointRec(GetMousePosition(), trackRect)) {
                        renderer.SelectTrack(i);
                        break;
                    }
                }
            }
            if (renderer.IsMusicPanelOpen()) {
                if (IsKeyPressed(KEY_UP)) {
                    int idx = renderer.GetCurrentTrackIndex() - 1;
                    if (idx < 0) idx = 13;
                    renderer.SelectTrack(idx);
                }
                if (IsKeyPressed(KEY_DOWN)) {
                    int idx = renderer.GetCurrentTrackIndex() + 1;
                    if (idx >= 14) idx = 0;
                    renderer.SelectTrack(idx);
                }
                if (IsKeyPressed(KEY_ENTER)) {
                    renderer.StartMusic();
                }
            }

            if (inputHandler.UpdateInput(gCtx.userInput)) {
                std::string inputLine = "> " + gCtx.userInput;
                displayLines.push_back(inputLine);

                std::vector<std::string> newOutputLines;

                if (parser.pendingChoice.active) {
                    int choice = -1;
                    try { choice = std::stoi(gCtx.userInput) - 1; }
                    catch (...) {}

                    if (choice >= 0 && choice < (int)parser.pendingChoice.choices.size()) {
                        newOutputLines = captureOutput([&]() {
                            parser.pendingChoice.choices[choice].action();
                            });
                        parser.pendingChoice.active = false;
                    }
                    else {
                        newOutputLines.push_back("Invalid choice, enter a number.");
                    }
                }
                else {
                    newOutputLines = captureOutput([&]() {
                        parser.parse(gCtx.userInput);
                        });
                }

                if (!newOutputLines.empty()) {
                    renderer.StartTypingAnimation(newOutputLines);
                    renderer.SnapToBottom();
                }

                gCtx.userInput.clear();
            }
        }
        else {
            gCtx.state = Renderer::GameState::MENU;
        }

        const char* prompt = parser.gameOver
            ? "Press ENTER to continue..."
            : inputHandler.GetBuffer();

        BeginDrawing();
        renderer.DrawBackground();
        renderer.DrawTextOverlay(displayLines, prompt);
        renderer.DrawMusicPanel();
        EndDrawing();
    }
}

// Plain void() wrapper — required by emscripten_set_main_loop
extern "C" void GameLoop() {
    GameFrame(nullptr);
}

//////////////////////* MAIN */////////////////////
int main() {
    SetTraceLogLevel(LOG_ALL);
    SetTraceLogCallback([](int logType, const char* text, va_list args) {
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), text, args);
        printf("%s\n", buffer);
        });

    static Renderer renderer(1024, 768);
    gCtx.renderer = &renderer;
    gCtx.state = Renderer::GameState::MENU;
    gCtx.menuIndex = 0;
    gCtx.needsInit = false;

#ifdef PLATFORM_WEB
    emscripten_set_main_loop(GameLoop, 0, 1);
#else
    while (!renderer.WindowShouldClose()) {
        GameFrame(nullptr);
    }
#endif

    return 0;
}