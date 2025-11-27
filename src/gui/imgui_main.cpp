#include <windows.h>
#include <tchar.h>
#include <d3d11.h>
#include <dxgi.h>
#include <shellapi.h>

#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include "../../vendor/imgui/imgui.h"
#include "../../vendor/imgui/backends/imgui_impl_win32.h"
#include "../../vendor/imgui/backends/imgui_impl_dx11.h"

#include "../logger.hpp"
#include "../localization/mod.hpp"
#include "../app/settings/settings.hpp"
#include "../app/fetch/fetch.hpp"
#include "../parser/parser.hpp"
#include "../app/downloads.hpp"
#include "../tags/mod.hpp"
#include "../types.hpp"
#include "../ui_constants.hpp"
#include "../views/cards/render.hpp"
#include "../app/about_ui.hpp"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

// Forward declarations of helper functions
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Data
static ID3D11Device*           g_pd3dDevice = nullptr;
static ID3D11DeviceContext*    g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*         g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

static void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer)
    {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

static void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

static bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    // createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };

    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
        featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

static void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain)          { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext)   { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice)          { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

static void ResizeSwapChain(UINT width, UINT height)
{
    if (!g_pSwapChain) return;
    CleanupRenderTarget();
    g_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    CreateRenderTarget();
}

static std::wstring to_wide(const std::string& s) {
    if (s.empty()) return std::wstring();
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring ws(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &ws[0], len);
    return ws;
}

struct AppGuiState {
    std::string locale = "en";
    localization::Bundle bundle{};
    app::settings::Config cfg{};
    bool cfgLoaded = false;

    // Fetch/parse
    std::string threadUrl;
    std::string cookieHeader;
    parser::GameInfo game;
    bool fetchedOk = false;
    std::string fetchStatus;

    // Settings UI
    bool cfg_log_to_file = false;
    int lang_idx = 0; // 0 auto, 1 en, 2 ru

    // Downloads UI
    std::string downloads_target_dir;
    std::vector<std::pair<app::downloads::Manager::Id, app::downloads::Item>> downloads_list;
    std::string downloads_info;
    // Tags
    tags::Catalog catalog;
    bool tagsLoaded = false;

    // Filters state
    bool library_on = false;
    std::string filter_search;
    std::vector<int> include_tags;
    std::vector<int> exclude_tags;
    std::vector<int> include_prefixes;
    std::vector<int> exclude_prefixes;
    types::Sorting sort = types::Sorting::Date;
    types::DateLimit date_limit = types::DateLimit::Anytime;
    types::TagLogic include_logic = types::TagLogic::Or;
    types::SearchMode search_mode = types::SearchMode::Title;
    bool logs_autoscroll = true;
    std::size_t logs_prev_line_count = 0;
    bool about_open = false;
};

// Simple localization helpers (param replacement for { $max })
static std::string replace_all(std::string s, const std::string& from, const std::string& to) {
    if (from.empty()) return s;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}
static std::string l10n(localization::Bundle& bundle, const std::string& key) {
    return localization::get(bundle, key);
}
static std::string l10n_with_max(localization::Bundle& bundle, const std::string& key, int max) {
    auto base = localization::get(bundle, key);
    return replace_all(base, "{ $max }", std::to_string(max));
}
static std::string l10n_with_n(localization::Bundle& bundle, const std::string& key, std::size_t n) {
    auto base = localization::get(bundle, key);
    return replace_all(base, "{ $n }", std::to_string(n));
}

// Right-side Filters panel to mirror Rust layout
static void render_filters_panel(struct AppGuiState& st) {
    // Title
    std::string filtersTitle = l10n(st.bundle, "filters-title");
    if (!filtersTitle.empty()) {
        ImGui::TextUnformatted(filtersTitle.c_str());
    } else {
        ImGui::TextUnformatted("Filters");
    }
    ImGui::Separator();

    // SORTING (segmented)
    {
        std::string hdr = l10n(st.bundle, "filters-sorting");
        if (!hdr.empty()) ImGui::SeparatorText(hdr.c_str()); else ImGui::SeparatorText("SORTING");
        auto sort_button = [&](types::Sorting val, const char* key) {
            std::string lbl = l10n(st.bundle, key);
            if (lbl.empty()) lbl = key;
            bool selected = (st.sort == val);
            if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f,0.5f,0.8f,1.0f));
            if (ImGui::Button(lbl.c_str())) st.sort = val;
            if (selected) ImGui::PopStyleColor();
            ImGui::SameLine();
        };
        sort_button(types::Sorting::Date, "sorting-date");
        sort_button(types::Sorting::Likes, "sorting-likes");
        sort_button(types::Sorting::Views, "sorting-views");
        sort_button(types::Sorting::Title, "sorting-title");
        sort_button(types::Sorting::Rating, "sorting-rating");
        ImGui::NewLine();
    }

    ImGui::Separator();

    // DATE LIMIT (discrete)
    {
        std::string hdr = l10n(st.bundle, "filters-date-limit");
        if (!hdr.empty()) ImGui::SeparatorText(hdr.c_str()); else ImGui::SeparatorText("DATE LIMIT");

        struct Opt { types::DateLimit val; const char* key; };
        Opt opts[] = {
            {types::DateLimit::Anytime, "date-limit-anytime"},
            {types::DateLimit::Today, "date-limit-today"},
            {types::DateLimit::Days3, "date-limit-days3"},
            {types::DateLimit::Days7, "date-limit-days7"},
            {types::DateLimit::Days14, "date-limit-days14"},
            {types::DateLimit::Days30, "date-limit-days30"},
            {types::DateLimit::Days90, "date-limit-days90"},
            {types::DateLimit::Days180, "date-limit-days180"},
            {types::DateLimit::Days365, "date-limit-days365"},
        };
        int cur = 0;
        for (int i=0;i<(int)(sizeof(opts)/sizeof(opts[0]));++i) if (st.date_limit == opts[i].val) { cur = i; break; }
        std::string preview = l10n(st.bundle, opts[cur].key);
        if (ImGui::BeginCombo("##date_limit", preview.empty() ? "ANYTIME" : preview.c_str())) {
            for (int i=0;i<(int)(sizeof(opts)/sizeof(opts[0]));++i) {
                std::string name = l10n(st.bundle, opts[i].key);
                bool selected = (i == cur);
                if (ImGui::Selectable((name.empty()?opts[i].key:name.c_str()), selected)) {
                    st.date_limit = opts[i].val;
                    cur = i;
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

    ImGui::Separator();

    // SEARCH (mode + input)
    {
        std::string hdr = l10n(st.bundle, "filters-search");
        if (!hdr.empty()) ImGui::SeparatorText(hdr.c_str()); else ImGui::SeparatorText("SEARCH");

        // Mode switch (Creator/Title)
        auto mode_button = [&](types::SearchMode val, const char* key) {
            std::string lbl = l10n(st.bundle, key);
            if (lbl.empty()) lbl = key;
            bool selected = (st.search_mode == val);
            if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f,0.5f,0.8f,1.0f));
            if (ImGui::SmallButton(lbl.c_str())) st.search_mode = val;
            if (selected) ImGui::PopStyleColor();
            ImGui::SameLine();
        };
        mode_button(types::SearchMode::Creator, "search-mode-creator");
        mode_button(types::SearchMode::Title, "search-mode-title");
        ImGui::NewLine();

        // Input with placeholder
        std::string ph = l10n(st.bundle, "filters-search-placeholder");
        if (ph.empty()) ph = "Search...";
        char buf[256] = {0};
        if (st.filter_search.size() >= sizeof(buf)) st.filter_search.resize(sizeof(buf)-1);
        std::snprintf(buf, sizeof(buf), "%s", st.filter_search.c_str());
        ImGui::SetNextItemWidth(360.0f);
        if (ImGui::InputTextWithHint("##query", ph.c_str(), buf, sizeof(buf))) {
            st.filter_search = buf;
        }
    }

    ImGui::Separator();

    // TAGS include with OR/AND logic
    {
        std::string hdr = l10n_with_max(st.bundle, "filters-include-tags-header", ui_constants::kMaxFilterItems);
        if (hdr.empty()) hdr = "TAGS";
        ImGui::SeparatorText(hdr.c_str());

        // OR/AND logic
        auto logic_button = [&](types::TagLogic val, const char* key) {
            std::string lbl = l10n(st.bundle, key);
            if (lbl.empty()) lbl = key;
            bool selected = (st.include_logic == val);
            if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f,0.5f,0.8f,1.0f));
            if (ImGui::SmallButton(lbl.c_str())) st.include_logic = val;
            if (selected) ImGui::PopStyleColor();
            ImGui::SameLine();
        };
        logic_button(types::TagLogic::Or, "tag-logic-or");
        logic_button(types::TagLogic::And, "tag-logic-and");
        ImGui::NewLine();

        if (st.tagsLoaded) {
            std::vector<std::pair<int,std::string>> tagOptions;
            tagOptions.reserve(st.catalog.tags.size());
            for (auto& kv : st.catalog.tags) tagOptions.emplace_back(kv.first, kv.second);
            std::sort(tagOptions.begin(), tagOptions.end(), [](auto& a, auto& b){ return a.second < b.second; });

            std::string ph = l10n(st.bundle, "filters-select-tag-include");
            if (ph.empty()) ph = "Select a tag to filter...";
            if (ImGui::BeginCombo("##tag_inc", ph.c_str())) {
                for (auto& opt : tagOptions) {
                    bool selected = (std::find(st.include_tags.begin(), st.include_tags.end(), opt.first) != st.include_tags.end());
                    if (ImGui::Selectable(opt.second.c_str(), selected)) {
                        if (!selected && (int)st.include_tags.size() < ui_constants::kMaxFilterItems) {
                            st.include_tags.push_back(opt.first);
                            st.filter_search.clear();
                        }
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            // Render removable chips
            if (!st.include_tags.empty()) {
                ImGui::TextUnformatted("Included:");
                ImGui::SameLine();
                ImGui::BeginGroup();
                for (size_t i=0;i<st.include_tags.size();) {
                    int id = st.include_tags[i];
                    auto it = st.catalog.tags.find(id);
                    std::string name = (it==st.catalog.tags.end() ? std::to_string(id) : it->second);
                    std::string lab = name + " ×";
                    if (ImGui::SmallButton(lab.c_str())) {
                        st.include_tags.erase(st.include_tags.begin()+i);
                        continue;
                    }
                    ImGui::SameLine();
                    ++i;
                }
                ImGui::EndGroup();
                ImGui::NewLine();
            }
        }
    }

    ImGui::Separator();

    // EXCLUDE TAGS
    {
        std::string hdr = l10n_with_max(st.bundle, "filters-exclude-tags-header", ui_constants::kMaxFilterItems);
        if (hdr.empty()) hdr = "EXCLUDE TAGS";
        ImGui::SeparatorText(hdr.c_str());

        if (st.tagsLoaded) {
            std::vector<std::pair<int,std::string>> tagOptions;
            tagOptions.reserve(st.catalog.tags.size());
            for (auto& kv : st.catalog.tags) tagOptions.emplace_back(kv.first, kv.second);
            std::sort(tagOptions.begin(), tagOptions.end(), [](auto& a, auto& b){ return a.second < b.second; });

            std::string ph = l10n(st.bundle, "filters-select-tag-exclude");
            if (ph.empty()) ph = "Select a tag to exclude...";
            if (ImGui::BeginCombo("##tag_exc", ph.c_str())) {
                for (auto& opt : tagOptions) {
                    bool selected = (std::find(st.exclude_tags.begin(), st.exclude_tags.end(), opt.first) != st.exclude_tags.end());
                    if (ImGui::Selectable(opt.second.c_str(), selected)) {
                        if (!selected && (int)st.exclude_tags.size() < ui_constants::kMaxFilterItems) {
                            st.exclude_tags.push_back(opt.first);
                            st.filter_search.clear();
                        }
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            // Render removable chips
            if (!st.exclude_tags.empty()) {
                ImGui::TextUnformatted("Excluded:");
                ImGui::SameLine();
                ImGui::BeginGroup();
                for (size_t i=0;i<st.exclude_tags.size();) {
                    int id = st.exclude_tags[i];
                    auto it = st.catalog.tags.find(id);
                    std::string name = (it==st.catalog.tags.end() ? std::to_string(id) : it->second);
                    std::string lab = name + " ×";
                    if (ImGui::SmallButton(lab.c_str())) {
                        st.exclude_tags.erase(st.exclude_tags.begin()+i);
                        continue;
                    }
                    ImGui::SameLine();
                    ++i;
                }
                ImGui::EndGroup();
                ImGui::NewLine();
            }
        }
    }

    ImGui::Separator();

    // PREFIXES include/exclude
    auto build_prefix_options = [&](std::vector<std::pair<int,std::string>>& out) {
        auto add = [&](const std::vector<tags::Group>& gs) {
            for (const auto& g : gs) {
                for (const auto& p : g.prefixes) out.emplace_back(p.id, p.name);
            }
        };
        out.clear();
        add(st.catalog.games);
        add(st.catalog.comics);
        add(st.catalog.animations);
        add(st.catalog.assets);
        std::sort(out.begin(), out.end(), [](auto& a, auto& b){ return a.second < b.second; });
    };

    {
        std::string hdr = l10n_with_max(st.bundle, "filters-include-prefixes-header", ui_constants::kMaxFilterItems);
        if (hdr.empty()) hdr = "PREFIXES";
        ImGui::SeparatorText(hdr.c_str());

        std::vector<std::pair<int,std::string>> prefOptions;
        build_prefix_options(prefOptions);

        std::string ph = l10n(st.bundle, "filters-select-prefix-include");
        if (ph.empty()) ph = "Select a prefix to include...";
        if (ImGui::BeginCombo("##pref_inc", ph.c_str())) {
            for (auto& opt : prefOptions) {
                bool selected = (std::find(st.include_prefixes.begin(), st.include_prefixes.end(), opt.first) != st.include_prefixes.end());
                if (ImGui::Selectable(opt.second.c_str(), selected)) {
                    if (!selected && (int)st.include_prefixes.size() < ui_constants::kMaxFilterItems) {
                        st.include_prefixes.push_back(opt.first);
                    }
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        // Render removable chips
        if (!st.include_prefixes.empty()) {
            ImGui::TextUnformatted("Included:");
            ImGui::SameLine();
            ImGui::BeginGroup();
            for (size_t i=0;i<st.include_prefixes.size();) {
                int id = st.include_prefixes[i];
                // Find name
                std::string name;
                {
                    std::vector<std::pair<int,std::string>> opts; build_prefix_options(opts);
                    auto it = std::find_if(opts.begin(), opts.end(), [&](auto& p){ return p.first == id; });
                    name = (it==opts.end()? std::to_string(id) : it->second);
                }
                std::string lab = name + " ×";
                if (ImGui::SmallButton(lab.c_str())) { st.include_prefixes.erase(st.include_prefixes.begin()+i); continue; }
                ImGui::SameLine();
                ++i;
            }
            ImGui::EndGroup();
            ImGui::NewLine();
        }
    }

    {
        std::string hdr = l10n_with_max(st.bundle, "filters-exclude-prefixes-header", ui_constants::kMaxFilterItems);
        if (hdr.empty()) hdr = "EXCLUDE PREFIXES";
        ImGui::SeparatorText(hdr.c_str());

        std::vector<std::pair<int,std::string>> prefOptions;
        build_prefix_options(prefOptions);

        std::string ph = l10n(st.bundle, "filters-select-prefix-exclude");
        if (ph.empty()) ph = "Select a prefix to exclude...";
        if (ImGui::BeginCombo("##pref_exc", ph.c_str())) {
            for (auto& opt : prefOptions) {
                bool selected = (std::find(st.exclude_prefixes.begin(), st.exclude_prefixes.end(), opt.first) != st.exclude_prefixes.end());
                if (ImGui::Selectable(opt.second.c_str(), selected)) {
                    if (!selected && (int)st.exclude_prefixes.size() < ui_constants::kMaxFilterItems) {
                        st.exclude_prefixes.push_back(opt.first);
                    }
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        // Render removable chips
        if (!st.exclude_prefixes.empty()) {
            ImGui::TextUnformatted("Excluded:");
            ImGui::SameLine();
            ImGui::BeginGroup();
            for (size_t i=0;i<st.exclude_prefixes.size();) {
                int id = st.exclude_prefixes[i];
                // Find name
                std::string name;
                {
                    std::vector<std::pair<int,std::string>> opts; build_prefix_options(opts);
                    auto it = std::find_if(opts.begin(), opts.end(), [&](auto& p){ return p.first == id; });
                    name = (it==opts.end()? std::to_string(id) : it->second);
                }
                std::string lab = name + " ×";
                if (ImGui::SmallButton(lab.c_str())) { st.exclude_prefixes.erase(st.exclude_prefixes.begin()+i); continue; }
                ImGui::SameLine();
                ++i;
            }
            ImGui::EndGroup();
            ImGui::NewLine();
        }
    }

    ImGui::Separator();

    // Bottom buttons and Library toggle
    {
        if (ImGui::Button(l10n(st.bundle, "common-logs").c_str())) {
            // TODO: open logs view
        }
        ImGui::SameLine();
        if (ImGui::Button(l10n(st.bundle, "common-about").c_str())) {
            st.about_open = true;
        }
        ImGui::SameLine();
        if (ImGui::Button(l10n(st.bundle, "common-settings").c_str())) {
            // TODO: open settings view
        }
        std::string libLabel = st.library_on ? l10n(st.bundle, "filters-library-on") : l10n(st.bundle, "filters-library");
        if (libLabel.empty()) libLabel = st.library_on ? "Library (ON)" : "Library";
        if (ImGui::Button(libLabel.c_str())) {
            st.library_on = !st.library_on;
        }
    }
}


int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int)
{
    // Load settings and localization to get window title
    AppGuiState st{};
    st.cfgLoaded = app::settings::Store::load("config.json", st.cfg);
    if (!st.cfgLoaded) app::settings::Store::save("config.json", st.cfg);
    st.cfg_log_to_file = st.cfg.log_to_file;
    st.lang_idx = (st.cfg.language == "auto" ? 0 : (st.cfg.language == "ru" ? 2 : 1));
    st.locale = (st.cfg.language == "auto" ? "en" : st.cfg.language);

    // logger
    logger::set_level(0);
    if (st.cfg.log_to_file) {
        std::string logPath = "app.log";
        if (!st.cfg.cache_folder.empty()) {
            logPath = st.cfg.cache_folder;
            if (!logPath.empty() && logPath.back() != '/' && logPath.back() != '\\') logPath += '/';
            logPath += "app.log";
        }
        logger::set_log_file(logPath);
        logger::info(std::string("Logging to file: ") + logPath);
    }

    if (!localization::load_bundle("src/localization/resources", st.locale, st.bundle))
        localization::load_bundle("../src/localization/resources", st.locale, st.bundle);
    std::string titleUtf8 = localization::get(st.bundle, "app-window-title");
    if (titleUtf8.empty()) titleUtf8 = "F95 Manager";
    // Load tags catalog (for Filters and Cards)
    st.tagsLoaded = tags::load_from_json("src/tags/tags.json", st.catalog) ||
                    tags::load_from_json("../src/tags/tags.json", st.catalog);
    if (st.tagsLoaded) {
        logger::info("Tags loaded: " + std::to_string(st.catalog.tags.size()) + " tags");
    } else {
        logger::warn("Tags not loaded");
    }

    // Convert UTF-8 title to wide
    std::wstring wTitle = to_wide(titleUtf8);

    // Create application window
    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"F95ManagerGui", nullptr };
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowW(wc.lpszClassName, wTitle.c_str(), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // Style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Main loop
    bool done = false;
    while (!done)
    {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Basic UI skeleton: menu bar and panes placeholders
        if (ImGui::Begin(titleUtf8.c_str()))
        {
            // Fetch bar
            ImGui::Text("Thread URL:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(600.0f);
            static char urlBuf[1024] = {0};
            if (st.threadUrl.size() >= sizeof(urlBuf)) st.threadUrl.resize(sizeof(urlBuf) - 1);
            std::snprintf(urlBuf, sizeof(urlBuf), "%s", st.threadUrl.c_str());
            if (ImGui::InputText("##url", urlBuf, sizeof(urlBuf))) {
                st.threadUrl = urlBuf;
            }

            ImGui::SameLine();
            if (ImGui::Button("Fetch & Parse")) {
                std::map<std::string, std::string> hdrs;
                if (!st.cookieHeader.empty()) hdrs["Cookie"] = st.cookieHeader;
                int status = 0;
                auto body = app::fetch::get_body(st.threadUrl, status, hdrs);
                if (status >= 200 && status < 300 && !body.empty()) {
                    st.game = parser::parse_thread(body);
                    st.fetchedOk = true;
                    st.fetchStatus = "OK " + std::to_string(status);
                } else {
                    st.fetchedOk = false;
                    st.fetchStatus = "HTTP " + std::to_string(status);
                }
            }
            ImGui::SameLine();
            ImGui::TextUnformatted(st.fetchStatus.c_str());

            ImGui::Separator();

            if (ImGui::BeginTabBar("MainTabs"))
            {
                if (ImGui::BeginTabItem("Cards"))
                {
                    // Two-pane layout: left = cards list/grid, right = Filters panel (fixed width)
                    float right_w = 360.0f;
                    float spacing = ImGui::GetStyle().ItemSpacing.x;

                    // Left content
                    ImGui::BeginChild("CardsLeft", ImVec2(ImGui::GetContentRegionAvail().x - right_w - spacing, 0), false);
                    if (st.fetchedOk) {
                        std::vector<parser::GameInfo> items;
                        items.push_back(st.game);
                        views::cards::draw_cards_grid(items, (float)ui_constants::kCardWidth, &st.cfg, &st.catalog);
                    } else {
                        ImGui::Text("No data. Enter thread URL and press Fetch & Parse.");
                    }
                    ImGui::EndChild();

                    ImGui::SameLine();

                    // Right filters panel
                    ImGui::BeginChild("FiltersRight", ImVec2(right_w, 0), true);
                    render_filters_panel(st);
                    ImGui::EndChild();

                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Downloads"))
                {
                    // Target dir
                    ImGui::Text("Target dir:");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(600.0f);
                    static char tdirBuf[1024] = {0};
                    if (st.downloads_target_dir.size() >= sizeof(tdirBuf)) st.downloads_target_dir.resize(sizeof(tdirBuf)-1);
                    std::snprintf(tdirBuf, sizeof(tdirBuf), "%s", st.downloads_target_dir.c_str());
                    if (ImGui::InputText("##tdir", tdirBuf, sizeof(tdirBuf))) {
                        st.downloads_target_dir = tdirBuf;
                    }

                    // URLs (one per line)
                    ImGui::Text("URLs (one per line):");
                    static char urlsBuf[4096] = {0};
                    ImGui::InputTextMultiline("##urls", urlsBuf, sizeof(urlsBuf), ImVec2(800, 100));

                    // Enqueue
                    if (ImGui::Button("Enqueue")) {
                        // Parse lines
                        std::vector<std::string> urls;
                        {
                            std::string all = urlsBuf;
                            std::string cur;
                            for (char c : all) {
                                if (c == '\r') continue;
                                if (c == '\n') {
                                    if (!cur.empty()) { urls.push_back(cur); cur.clear(); }
                                } else {
                                    cur.push_back(c);
                                }
                            }
                            if (!cur.empty()) urls.push_back(cur);
                        }
                        if (!urls.empty() && !st.downloads_target_dir.empty()) {
                            app::downloads::Item it;
                            it.title = ""; // filename from URL
                            it.target_dir = st.downloads_target_dir;
                            it.urls = std::move(urls);
                            auto id = app::downloads::enqueue(it);
                            st.downloads_list.emplace_back(id, it);
                            st.downloads_info = "Enqueued " + std::to_string(id);
                        } else {
                            st.downloads_info = "Provide target dir and at least one URL";
                        }
                    }
                    ImGui::SameLine();
                    ImGui::TextUnformatted(st.downloads_info.c_str());

                    ImGui::Separator();

                    // List current downloads
                    if (st.downloads_list.empty()) {
                        ImGui::Text("No downloads enqueued.");
                    } else {
                        for (auto& p : st.downloads_list) {
                            auto id = p.first;
                            auto prog = app::downloads::query(id);
                            float frac = 0.0f;
                            if (prog.bytes_total > 0) frac = (float)((double)prog.bytes_done / (double)prog.bytes_total);
                            ImGui::Text("ID %llu: %s", (unsigned long long)id, prog.message.c_str());
                            ImGui::SameLine();
                            ImGui::Text(" %llu / %llu bytes", (unsigned long long)prog.bytes_done, (unsigned long long)prog.bytes_total);
                            ImGui::ProgressBar(frac, ImVec2(600.f, 0.f));
                            ImGui::SameLine();
                            if (ImGui::Button(std::string("Cancel##" + std::to_string(id)).c_str())) {
                                app::downloads::cancel(id);
                            }
                        }
                    }

                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Logs"))
                {
                    // Controls row
                    bool autoScroll = st.logs_autoscroll;
                    std::string autoscrollLbl = l10n(st.bundle, "logs-autoscroll");
                    if (autoscrollLbl.empty()) autoscrollLbl = "Autoscroll";
                    ImGui::Checkbox(autoscrollLbl.c_str(), &autoScroll);
                    st.logs_autoscroll = autoScroll;

                    ImGui::SameLine();
                    std::string clearLbl = l10n(st.bundle, "logs-clear");
                    if (clearLbl.empty()) clearLbl = "Clear";
                    if (ImGui::Button(clearLbl.c_str())) {
                        logger::clear();
                        st.logs_prev_line_count = 0;
                    }

                    ImGui::SameLine();
                    std::string copyLbl = l10n(st.bundle, "logs-copy");
                    if (copyLbl.empty()) copyLbl = "Copy";
                    if (ImGui::Button(copyLbl.c_str())) {
                        auto ls = logger::lines();
                        std::string all;
                        all.reserve(4096);
                        for (auto& s : ls) { all += s; if (!all.empty() && all.back() != '\n') all += "\n"; }
                        ImGui::SetClipboardText(all.c_str());
                    }

                    ImGui::SameLine();
                    auto count = logger::line_count();
                    std::string countLbl = l10n_with_n(st.bundle, "logs-lines", count);
                    if (countLbl.empty()) {
                        ImGui::Text("%zu lines", count);
                    } else {
                        ImGui::Text("%s", countLbl.c_str());
                    }

                    // Log content
                    ImGui::Separator();
                    ImGui::BeginChild("logs_scroll", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
                    auto lines = logger::lines();
                    for (auto& s : lines) {
                        ImGui::TextUnformatted(s.c_str());
                    }
                    if (st.logs_autoscroll && lines.size() > st.logs_prev_line_count) {
                        ImGui::SetScrollHereY(1.0f);
                    }
                    st.logs_prev_line_count = lines.size();
                    ImGui::EndChild();

                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Settings"))
                {
                    // Language
                    const char* langs[] = {"auto", "en", "ru"};
                    ImGui::Text("Language:");
                    ImGui::SameLine();
                    if (ImGui::BeginCombo("##lang", langs[st.lang_idx])) {
                        for (int i=0;i<3;i++) {
                            bool sel = (st.lang_idx == i);
                            if (ImGui::Selectable(langs[i], sel)) st.lang_idx = i;
                            if (sel) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    st.cfg.language = langs[st.lang_idx];

                    // Cookie header
                    static char cookieBuf[2048] = {0};
                    if (st.cookieHeader.size() >= sizeof(cookieBuf)) st.cookieHeader.resize(sizeof(cookieBuf) - 1);
                    std::snprintf(cookieBuf, sizeof(cookieBuf), "%s", st.cookieHeader.c_str());
                    ImGui::Text("Cookie header:");
                    ImGui::InputTextMultiline("##cookie", cookieBuf, sizeof(cookieBuf), ImVec2(800, 80));
                    st.cookieHeader = cookieBuf;

                    // Log to file
                    ImGui::Checkbox("Log to file", &st.cfg_log_to_file);
                    st.cfg.log_to_file = st.cfg_log_to_file;

                    if (ImGui::Button("Save Config")) {
                        if (app::settings::Store::save("config.json", st.cfg)) {
                            logger::info("Config saved.");
                        } else {
                            logger::error("Failed to save config.");
                        }
                    }
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();

        // Modals / overlays (must be called while frame is active)
        app::about_ui::show_about_dialog(st.about_open, st.bundle);

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.10f, 0.10f, 0.12f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

// Win32 message handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            UINT w = (UINT)LOWORD(lParam);
            UINT h = (UINT)HIWORD(lParam);
            ResizeSwapChain(w, h);
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
