#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/Null.hpp>
#include <hyprtoolkit/element/Combobox.hpp>

#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/memory/UniquePtr.hpp>
#include <hyprutils/string/VarList.hpp>
#include <hyprutils/string/String.hpp>
#include <hyprutils/os/Process.hpp>

#include <print>
#include <ranges>
#include <algorithm>
#include <fstream>
#include <filesystem>

using namespace Hyprutils::Memory;
using namespace Hyprutils::Math;
using namespace Hyprutils::String;
using namespace Hyprutils::OS;
using namespace Hyprtoolkit;

#define SP  CSharedPointer
#define ASP CAtomicSharedPointer
#define WP  CWeakPointer
#define UP  CUniquePointer

constexpr const size_t                         TABS_NUMBER       = 5;
constexpr const size_t                         INNER_NULL_MARGIN = 5;

constexpr std::array<const char*, TABS_NUMBER> TITLES = {
    "Welcome to Hyprland!", "Getting started", "Basic configuration", "Default apps", "That's it!",
};

constexpr std::array<const char*, 6> TERMINALS = {
    "kitty", "alacritty", "wezterm", "foot", "konsole", "gnome-terminal",
};

constexpr std::array<const char*, 5> FILE_MANAGERS = {"dolphin", "thunar", "pcmanfm", "nautilus", "nemo"};

constexpr const char*                TAB1_CONTENT =
    R"#(We hope you enjoy your stay. In order to help you get accomodated to Hyprland in an easier manner, we prepared a little basic setup tutorial, just for you.

If you feel adventurous, or are an advanced user, you can click the "Thanks, but I don't need help" button on the bottom. It will close this window and never show it again.

If you want to manually launch this welcome app, just execute hyprland-welcome in your terminal.

Click the "next" button to proceed to the next step of your setup :)
)#";

constexpr const char* TAB2_CONTENT =
    R"#(The first thing we'll need to do is get some packages installed that you absolutely need in order for your system to be working properly.
Apps with a <span foreground="#cc2222">*</span> are <span foreground="red"><i>absolutely necessary</i></span> for a working system. All other are <span foreground="red"><i>highly</i></span> recommended, as they provide core parts of a working environment.
You can proceed without any of those, but it's not advised.

There is a possibility that this app is unable to detect some of your installed binaries. In that case, it's okay to ignore them. Nix users might have the issue.

Use the <i>launch terminal</i> button to launch a terminal.
Use SUPER+M to exit hyprland.
Supported terminals: kitty, alacritty, foot, wezterm, konsole, gnome-terminal, xterm.

<i>Hint: Hover on the different components to see what options are accepted. <span foreground="#22cc22">Green</span> means the component is found to be installed, <span foreground="#22cccc">blue</span> means it's running.
This list refreshes automatically.</i>)#";

constexpr const char* TAB3_CONTENT =
    R"#(Now that you've installed the basic apps, make sure to add them to autostart. Hyprland doesn't automatically start anything for you, you need to tell it to.
Go to ~/.config/hypr/hyprland.conf, and add "exec-once = appname" to launch your apps, for example:
exec-once = hyprpaper
exec-once = waybar

In general, configuring apps is something for you to do. Each app you install may come with its own config file and options.

A great point to start is the Hyprland wiki at https://wiki.hypr.land. There, the master tutorial will teach you everything and link to further docs.

If you prefer pre-configured settings, or "dotfiles", you can see the "preconfigured configs" section on the wiki, or search online. <span foreground="#cc2222">Important note:</span> dotfiles can run <i>anything</i> on your computer. Make sure you trust the source.)#";

constexpr const char* TAB4_PREAMBLE =
    R"#(We know that not everyone uses kitty and dolphin. That's why we let you choose.
If you wish to change the defaults, use the dropdowns below.)#";

constexpr const char* TAB5_CONTENT =
    R"#(That's it for this small introduction! Explore the wiki, and various apps, and enjoy your journey!

Here are some important default shortcuts:
• SUPER + Q <span foreground="#666666">=</span> Terminal
• SUPER + E <span foreground="#666666">=</span> File Manager
• SUPER + C <span foreground="#666666">=</span> Close window
• SUPER + V <span foreground="#666666">=</span> Toggle floating
• SUPER + M <span foreground="#666666">=</span> Exit Hyprland
• SUPER + [1 - 9] <span foreground="#666666">=</span> Workspaces 1 - 9
• SUPER + SHIFT + [1 - 9] <span foreground="#666666">=</span> Move window to workspace 1 - 9
• SUPER + Arrows <span foreground="#666666">=</span> Move focus around

<i>You can easily change these in your hyprland.conf.</i>
    
Thank you for choosing Hyprland! ❤️)#";

struct SAppState {
    std::string              name;
    std::vector<std::string> binaryNames;
    bool                     mandatory = false;
    SP<CTextElement>         labelEl;
};

static struct {
    SP<IBackend>                              backend;
    SP<CRectangleElement>                     tabContainer;
    std::array<SP<CNullElement>, TABS_NUMBER> tabs;
    SP<CTextElement>                          topText;
    SP<CRowLayoutElement>                     buttonLayout;
    SP<CNullElement>                          buttonSpacer;
    SP<CButtonElement>                        buttonBack, buttonNext, buttonQuit, buttonFinish, buttonOpenWiki, buttonLaunchTerm;
    size_t                                    tab = 0;
    std::vector<SP<SAppState>>                appStates;
    ASP<CTimer>                               appRefreshTimer, wikiOpenTimer;
} state;

static bool appExists(std::string binName) {
    static auto PATH = getenv("PATH");

    if (!PATH)
        return false;

    static CVarList paths(PATH, 0, ':', true);

    for (const auto& p : paths) {
        std::error_code ec;
        if (!std::filesystem::exists(std::filesystem::path(p) / binName, ec) || ec)
            continue;
        return true;
    }

    return false;
}

static bool appIsRunning(std::string binName) {
    // loop over /proc/ entries, check exe
    std::error_code ec_it;
    for (const std::filesystem::path& procEntry : std::filesystem::directory_iterator("/proc", ec_it)) {
        if (ec_it)
            continue;

        std::error_code ec;

        if (!std::filesystem::exists(procEntry / "exe", ec) || ec)
            continue;

        const auto CANONICAL = std::filesystem::canonical(procEntry / "exe", ec);

        if (ec)
            continue;

        if (!CANONICAL.has_filename())
            continue;

        if (CANONICAL.filename() == binName)
            return true;
    }

    return false;
}

static void updateApps() {
    if (state.tab != 1)
        return;

    state.appRefreshTimer = state.backend->addTimer(std::chrono::seconds(1), [](ASP<CTimer> t, void* d) { updateApps(); }, nullptr);

    for (const auto& a : state.appStates) {

        bool found = false;

        for (const auto& bn : a->binaryNames) {
            if (!appIsRunning(bn))
                continue;

            found = true;

            a->labelEl->rebuild()
                ->text(std::format("{}{}: <span foreground=\"#22cccc\">Running: </span>{}", a->name, (a->mandatory ? "<span foreground=\"#cc2222\">*</span>" : ""), bn))
                ->commence();
            break;
        }
        if (!found) {
            for (const auto& bn : a->binaryNames) {
                if (!appExists(bn))
                    continue;

                found = true;

                a->labelEl->rebuild()
                    ->text(std::format("{}{}: <span foreground=\"#22cc22\">Installed: </span>{}", a->name, (a->mandatory ? "<span foreground=\"#cc2222\">*</span>" : ""), bn))
                    ->commence();
                break;
            }
        }

        if (!found)
            a->labelEl->rebuild()
                ->text(std::format("{}{}: <span foreground=\"#cc2222\">Missing</span>", a->name, (a->mandatory ? "<span foreground=\"#cc2222\">*</span>" : "")))
                ->commence();
    }
}

static void updateTab() {
    state.tabContainer->clearChildren();
    state.tabContainer->addChild(state.tabs[state.tab]);
    state.topText->rebuild()->text(TITLES[state.tab])->commence();
    updateApps();

    state.buttonLayout->clearChildren();

    if (state.tab == 0) {
        state.buttonLayout->addChild(state.buttonSpacer);
        state.buttonLayout->addChild(state.buttonQuit);
        state.buttonLayout->addChild(state.buttonNext);
    } else if (state.tab == 1) {
        state.buttonLayout->addChild(state.buttonBack);
        state.buttonLayout->addChild(state.buttonSpacer);
        state.buttonLayout->addChild(state.buttonLaunchTerm);
        state.buttonLayout->addChild(state.buttonNext);
    } else if (state.tab == 2) {
        state.buttonLayout->addChild(state.buttonBack);
        state.buttonLayout->addChild(state.buttonSpacer);
        state.buttonLayout->addChild(state.buttonOpenWiki);
        state.buttonLayout->addChild(state.buttonNext);
    } else if (state.tab == 3) {
        state.buttonLayout->addChild(state.buttonBack);
        state.buttonLayout->addChild(state.buttonSpacer);
        state.buttonLayout->addChild(state.buttonNext);
    } else if (state.tab == 4) {
        state.buttonLayout->addChild(state.buttonBack);
        state.buttonLayout->addChild(state.buttonSpacer);
        state.buttonLayout->addChild(state.buttonOpenWiki);
        state.buttonLayout->addChild(state.buttonFinish);
    }
}

static void tabBack() {
    if (state.tab == 0)
        return;

    state.tab--;
    updateTab();
}

static void tabNext() {
    if (state.tab == TITLES.size() - 1)
        return;

    state.tab++;
    updateTab();
}

static void registerAppState(std::string&& name, std::vector<std::string>&& binaries, bool mandatory, const std::string& recommend = "", const std::string& note = "") {
    auto appState         = makeShared<SAppState>();
    appState->name        = std::move(name);
    appState->binaryNames = std::move(binaries);
    appState->labelEl     = CTextBuilder::begin()->color([] { return state.backend->getPalette()->m_colors.text; })->fontSize({CFontSize::HT_FONT_TEXT})->text("")->commence();
    appState->mandatory   = mandatory;

    std::string acceptedStr = "";
    for (const auto& b : appState->binaryNames) {
        acceptedStr += b + ", ";
    }
    if (!acceptedStr.empty())
        acceptedStr = acceptedStr.substr(0, acceptedStr.length() - 2);

    std::string tooltip = recommend.empty() ? std::format("Accepted: {}", acceptedStr) : std::format("Recommended: {}\nAccepted: {}", recommend, acceptedStr);
    if (!note.empty())
        tooltip += std::format("\n{}", note);

    appState->labelEl->setTooltip(std::move(tooltip));

    state.appStates.emplace_back(std::move(appState));
}

static SP<Hyprtoolkit::CRowLayoutElement> spaceOut(std::string&& label, SP<IElement> el) {
    auto text   = CTextBuilder::begin()->text(std::move(label))->commence();
    auto spacer = CNullBuilder::begin()->commence();
    spacer->setGrow(true, false);
    auto layout = CRowLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->commence();
    layout->addChild(text);
    layout->addChild(spacer);
    layout->addChild(el);
    return layout;
}

static std::optional<std::string> readFileAsString(const std::string& path) {
    std::error_code ec;

    if (!std::filesystem::exists(path, ec) || ec)
        return std::nullopt;

    std::ifstream file(path);
    if (!file.good())
        return std::nullopt;

    return trim(std::string((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>())));
}

static std::optional<std::string> updateDefaultConfigVar(const std::string& var, const char* newValue) {
    const auto HOME = getenv("HOME");
    if (!HOME)
        return "Can't save: no $HOME env";

    const auto PATH = std::string{HOME} + "/.config/hypr/hyprland.conf";

    const auto STR = readFileAsString(PATH);

    if (!STR)
        return "Can't save: failed to read config";

    std::string newConfig = *STR;

    size_t      varPos = newConfig.find("\n$" + var);
    if (varPos == std::string::npos)
        return "Can't save: config isn't default, doesn't have variable";

    varPos++;
    size_t varEnd = newConfig.find('\n', varPos + 1);

    if (varEnd == std::string::npos)
        newConfig = std::format("{}${} = {}", newConfig.substr(0, varPos), var, newValue);
    else
        newConfig = std::format("{}${} = {}{}", newConfig.substr(0, varPos), var, newValue, newConfig.substr(varEnd));

    std::ofstream ofs(PATH, std::ios::trunc);
    ofs << newConfig;
    ofs.close();

    return std::nullopt;
}

static void removeAutogen() {
    const auto HOME = getenv("HOME");
    if (!HOME)
        return;

    const auto PATH = std::string{HOME} + "/.config/hypr/hyprland.conf";

    const auto STR = readFileAsString(PATH);

    if (!STR)
        return;

    std::string newConfig = *STR;

    size_t      varPos = newConfig.find("\nautogenerated = 1");
    if (varPos == std::string::npos)
        return;

    varPos++;
    size_t varEnd = newConfig.find('\n', varPos);

    newConfig = newConfig.substr(0, varPos) + newConfig.substr(varEnd);

    std::ofstream ofs(PATH, std::ios::trunc);
    ofs << newConfig;
    ofs.close();
}

static void initTabs() {
    {
        // Tab 1
        auto nullEl = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->commence();
        auto layout = CColumnLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->commence();
        auto text   = CTextBuilder::begin()->text(TAB1_CONTENT)->color([] { return state.backend->getPalette()->m_colors.text; })->commence();
        auto spacer = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {1, 1}})->commence();
        spacer->setGrow(true);

        layout->addChild(text);
        layout->addChild(spacer);
        nullEl->addChild(layout);
        nullEl->setGrow(true);
        nullEl->setMargin(INNER_NULL_MARGIN);
        state.tabs[0] = nullEl;
    }

    {
        // Tab 2
        auto nullEl = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->commence();
        auto layout = CColumnLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->gap(20)->commence();
        auto text   = CTextBuilder::begin()->text(TAB2_CONTENT)->color([] { return state.backend->getPalette()->m_colors.text; })->commence();
        auto spacer = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {1, 1}})->commence();
        spacer->setGrow(true);

        layout->addChild(text);

        nullEl->addChild(layout);
        nullEl->setGrow(true);
        nullEl->setMargin(INNER_NULL_MARGIN);

        auto appLayoutParent = CRowLayoutBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO, {1, 1}})->commence();
        SP<CColumnLayoutElement> appLayouts[2] = {
            CColumnLayoutBuilder::begin()->gap(4)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO, {0.5F, 1.F}})->commence(),
            CColumnLayoutBuilder::begin()->gap(4)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO, {0.5F, 1.F}})->commence(),
        };

        appLayouts[0]->setGrow(false, true);
        appLayouts[0]->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_ABSOLUTE);
        appLayouts[0]->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_LEFT, true);
        appLayouts[1]->setGrow(false, true);
        appLayouts[1]->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_ABSOLUTE);
        appLayouts[1]->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_LEFT, true);

        appLayoutParent->addChild(appLayouts[0]);
        appLayoutParent->addChild(appLayouts[1]);

        layout->addChild(appLayoutParent);

        // app states
        registerAppState("Authentication agent", {"hyprpolkitagent", "polkit-kde-agent"}, true, "hyprpolkitagent");
        registerAppState("File manager", {"dolphin", "ranger", "thunar", "pcmanfm", "nautilus", "nemo", "nnn", "yazi"}, true);
        registerAppState("Terminal", {"kitty", "alacritty", "wezterm", "foot", "konsole", "gnome-terminal"}, true, "kitty");
        registerAppState("Pipewire", {"pipewire", "wireplumber"}, true);
        registerAppState("Wallpaper", {"hyprpaper", "swww", "awww", "swaybg", "wpaperd"}, false, "hyprpaper");
        registerAppState("XDG Desktop Portal", {"xdg-desktop-portal-hyprland"}, true);
        registerAppState("Notification Daemon", {"dunst", "mako"}, true, "", "Please note you can have custom notification daemons with your shell, e.g. quickshell.");
        registerAppState("Status bar / shell", {"quickshell", "waybar", "eww", "ags"}, false, "", "For new users we recommend waybar, for advanced users quickshell.");
        registerAppState("Application launcher", {"hyprlauncher", "fuzzel", "wofi", "rofi", "anyrun", "walker", "tofi"}, false, "hyprlauncher");
        registerAppState("Clipboard", {"wl-copy"}, true, "", "wl-copy is provided by wl-clipboard in most distros.");

        // register them
        bool flip = false;
        for (const auto& e : state.appStates) {
            appLayouts[flip ? 1 : 0]->addChild(e->labelEl);
            flip = !flip;
        }

        state.tabs[1] = nullEl;
    }

    {
        // Tab 3
        auto nullEl = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->commence();
        auto layout = CColumnLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->commence();
        auto text   = CTextBuilder::begin()->text(TAB3_CONTENT)->color([] { return state.backend->getPalette()->m_colors.text; })->commence();
        auto spacer = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {1, 1}})->commence();
        spacer->setGrow(true);

        layout->addChild(text);
        layout->addChild(spacer);
        nullEl->addChild(layout);
        nullEl->setGrow(true);
        nullEl->setMargin(INNER_NULL_MARGIN);
        state.tabs[2] = nullEl;
    }

    {
        // Tab 4
        auto nullEl = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->commence();
        auto layout = CColumnLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->gap(4)->commence();
        auto text   = CTextBuilder::begin()->text(TAB4_PREAMBLE)->color([] { return state.backend->getPalette()->m_colors.text; })->commence();
        auto spacer = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {1, 1}})->commence();
        auto hr     = CRectangleBuilder::begin()
                      ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {0.5F, 11.F}})
                      ->color([] { return state.backend->getPalette()->m_colors.base; })
                      ->commence();
        auto hr2 = CRectangleBuilder::begin()
                       ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {0.5F, 11.F}})
                       ->color([] { return state.backend->getPalette()->m_colors.base; })
                       ->commence();
        auto terminalText     = CTextBuilder::begin()->text("")->color([] { return state.backend->getPalette()->m_colors.text; })->commence();
        auto terminalTextNull = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->commence();
        auto fileManagerText  = CTextBuilder::begin()->text("")->color([] { return state.backend->getPalette()->m_colors.text; })->commence();
        auto fileManagerNull  = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->commence();
        auto defaultContainer = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {0.6F, 1.F}})->commence();
        auto defaultLayout    = CColumnLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->gap(4)->commence();
        spacer->setGrow(true);
        hr->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_ABSOLUTE);
        hr->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_HCENTER, true);
        hr->setMargin(5);
        hr2->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_ABSOLUTE);
        hr2->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_HCENTER, true);
        hr2->setMargin(5);
        terminalText->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_ABSOLUTE);
        terminalText->setPositionFlag(sc<Hyprtoolkit::IElement::ePositionFlag>(Hyprtoolkit::IElement::HT_POSITION_FLAG_VCENTER | Hyprtoolkit::IElement::HT_POSITION_FLAG_RIGHT),
                                      true);
        fileManagerText->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_ABSOLUTE);
        fileManagerText->setPositionFlag(sc<Hyprtoolkit::IElement::ePositionFlag>(Hyprtoolkit::IElement::HT_POSITION_FLAG_VCENTER | Hyprtoolkit::IElement::HT_POSITION_FLAG_RIGHT),
                                         true);

        std::vector<std::string> terms, fms;
        terms.reserve(TERMINALS.size());
        fms.reserve(FILE_MANAGERS.size());
        for (const auto& t : TERMINALS) {
            terms.emplace_back(t);
        }
        for (const auto& f : FILE_MANAGERS) {
            fms.emplace_back(f);
        }

        auto updateText = [](SP<CTextElement> textEl, const char* app, std::string err = "") -> void {
            if (!err.empty()) {
                textEl->rebuild()->text(std::format("<span foreground=\"#cc2222\">⚠ Error: {}</span>", err))->commence();
                return;
            }

            if (appExists(app))
                textEl->rebuild()->text(std::format("<span foreground=\"#22cc22\">✓ {} is installed</span>", app))->commence();
            else
                textEl->rebuild()->text(std::format("<span foreground=\"#cc2222\">⚠ {} is not installed</span>", app))->commence();
        };

        defaultContainer->addChild(defaultLayout);
        defaultLayout->addChild(spaceOut("Terminal",
                                         CComboboxBuilder::begin()
                                             ->items(std::move(terms))
                                             ->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {200, 25}})
                                             ->onChanged([tt = terminalText, updateText](SP<CComboboxElement> el, size_t idx) {
                                                 const auto& TERM_NAME = TERMINALS[idx];
                                                 const auto  RESULT    = updateDefaultConfigVar("terminal", TERM_NAME);
                                                 if (RESULT)
                                                     updateText(tt, TERM_NAME, *RESULT);
                                                 else
                                                     updateText(tt, TERM_NAME);
                                             })
                                             ->commence()));
        terminalTextNull->addChild(terminalText);
        defaultLayout->addChild(terminalTextNull);

        defaultLayout->addChild(spaceOut("File Manager",
                                         CComboboxBuilder::begin()
                                             ->items(std::move(fms))
                                             ->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {200, 25}})
                                             ->onChanged([tt = fileManagerText, updateText](SP<CComboboxElement> el, size_t idx) {
                                                 const auto& FM_NAME = FILE_MANAGERS[idx];
                                                 const auto  RESULT  = updateDefaultConfigVar("fileManager", FM_NAME);
                                                 if (RESULT)
                                                     updateText(tt, FM_NAME, *RESULT);
                                                 else
                                                     updateText(tt, FM_NAME);
                                             })
                                             ->commence()));
        fileManagerNull->addChild(fileManagerText);
        defaultLayout->addChild(fileManagerNull);

        updateText(terminalText, TERMINALS[0]);
        updateText(fileManagerText, FILE_MANAGERS[0]);

        layout->addChild(text);
        layout->addChild(hr);
        layout->addChild(defaultContainer);
        layout->addChild(hr);
        layout->addChild(CTextBuilder::begin()
                             ->text("<i>You can always change these later in your hyprland.conf</i>")
                             ->color([] { return state.backend->getPalette()->m_colors.text; })
                             ->commence());
        layout->addChild(spacer);
        nullEl->addChild(layout);
        nullEl->setGrow(true);
        nullEl->setMargin(INNER_NULL_MARGIN);
        state.tabs[3] = nullEl;
    }

    {
        // Tab 5
        auto nullEl = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->commence();
        auto layout = CColumnLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->commence();
        auto text   = CTextBuilder::begin()->text(TAB5_CONTENT)->color([] { return state.backend->getPalette()->m_colors.text; })->commence();
        auto spacer = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {1, 1}})->commence();
        spacer->setGrow(true);

        layout->addChild(text);
        layout->addChild(spacer);
        nullEl->addChild(layout);
        nullEl->setGrow(true);
        nullEl->setMargin(INNER_NULL_MARGIN);
        state.tabs[4] = nullEl;
    }
}

int main(int argc, char** argv, char** envp) {
    state.backend = IBackend::create();

    const auto FONT_SIZE   = CFontSize{CFontSize::HT_FONT_TEXT}.ptSize();
    const auto WINDOW_SIZE = Vector2D{FONT_SIZE * 90.F, FONT_SIZE * 50.F};

    //
    auto window =
        CWindowBuilder::begin()->preferredSize(WINDOW_SIZE)->minSize(WINDOW_SIZE)->maxSize(WINDOW_SIZE)->appTitle("Welcome to Hyprland")->appClass("hyprland-welcome")->commence();

    initTabs();

    window->m_rootElement->addChild(CRectangleBuilder::begin()->color([] { return state.backend->getPalette()->m_colors.background; })->commence());

    auto rootLayout = CColumnLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})->gap(10)->commence();
    rootLayout->setMargin(3);

    window->m_rootElement->addChild(rootLayout);

    // top null: title

    auto topNull = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 10}})->commence();
    topNull->setMargin(4);

    state.topText = CTextBuilder::begin()->color([] { return state.backend->getPalette()->m_colors.text; })->text(TITLES[state.tab])->fontSize(CFontSize::HT_FONT_H2)->commence();
    state.topText->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_ABSOLUTE);
    state.topText->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_CENTER, true);

    topNull->addChild(state.topText);
    rootLayout->addChild(topNull);

    // // content
    state.tabContainer = CRectangleBuilder::begin()
                             ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})
                             ->color([] { return state.backend->getPalette()->m_colors.background; })
                             ->borderThickness(1)
                             ->borderColor([] { return state.backend->getPalette()->m_colors.background.brighten(0.2F); })
                             ->rounding(state.backend->getPalette()->m_vars.smallRounding)
                             ->commence();
    state.tabContainer->setGrow(false, true);

    rootLayout->addChild(state.tabContainer);

    state.buttonLayout = CRowLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->gap(5)->commence();
    state.buttonLayout->setMargin(2);

    state.buttonBack = CButtonBuilder::begin()->label("Back")->onMainClick([](SP<CButtonElement> self) { tabBack(); })->commence();
    state.buttonNext = CButtonBuilder::begin()->label("Next")->onMainClick([](SP<CButtonElement> self) { tabNext(); })->commence();
    state.buttonQuit = CButtonBuilder::begin()
                           ->label("Thanks, but I don't need help")
                           ->onMainClick([w = WP<IWindow>{window}](SP<CButtonElement> self) {
                               if (w)
                                   w->close();
                               state.backend->destroy();
                           })
                           ->commence();
    state.buttonLaunchTerm = CButtonBuilder::begin()
                                 ->label("Launch terminal")
                                 ->onMainClick([w = WP<IWindow>{window}](SP<CButtonElement> self) {
                                     for (const auto& t : TERMINALS) {
                                         if (!appExists(t))
                                             continue;

                                         CProcess proc(t, {""});
                                         proc.runAsync();
                                     }
                                 })
                                 ->commence();
    state.buttonFinish = CButtonBuilder::begin()
                             ->label("Finish")
                             ->onMainClick([w = WP<IWindow>{window}](SP<CButtonElement> self) {
                                 removeAutogen();
                                 if (w)
                                     w->close();
                                 state.backend->destroy();
                             })
                             ->commence();
    state.buttonOpenWiki = CButtonBuilder::begin()
                               ->label("🔗 Open wiki")
                               ->onMainClick([w = WP<IWindow>{window}](SP<CButtonElement> self) {
                                   CProcess proc("xdg-open", {"https://wiki.hypr.land/"});
                                   proc.runAsync();

                                   state.buttonOpenWiki->rebuild()->label("🔗 Opened in your browser")->commence();
                                   state.wikiOpenTimer = state.backend->addTimer(
                                       std::chrono::seconds(1), [](ASP<CTimer> t, void* d) { state.buttonOpenWiki->rebuild()->label("🔗 Open wiki")->commence(); }, nullptr);
                               })
                               ->commence();

    state.buttonSpacer = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {1, 1}})->commence();
    state.buttonSpacer->setGrow(true);

    rootLayout->addChild(state.buttonLayout);

    window->m_events.closeRequest.listenStatic([w = WP<IWindow>{window}] {
        w->close();
        state.backend->destroy();
    });

    updateTab();

    window->open();

    state.backend->enterLoop();

    return 0;
}