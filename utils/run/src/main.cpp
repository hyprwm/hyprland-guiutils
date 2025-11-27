#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/element/Textbox.hpp>
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/Null.hpp>

#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/memory/UniquePtr.hpp>
#include <hyprutils/string/ConstVarList.hpp>
#include <hyprutils/string/String.hpp>
#include <hyprutils/string/VarList2.hpp>
#include <hyprutils/os/Process.hpp>

#include <filesystem>
#include <xkbcommon/xkbcommon-keysyms.h>

using namespace Hyprutils::Memory;
using namespace Hyprutils::Math;
using namespace Hyprutils::String;
using namespace Hyprutils::OS;
using namespace Hyprtoolkit;

#define SP CSharedPointer
#define WP CWeakPointer
#define UP CUniquePointer

static SP<IBackend> backend;

static struct {
    SP<CTextboxElement>      textbox;
    SP<CTextElement>         errorText;
    SP<CTextElement>         content;
    SP<CColumnLayoutElement> layoutInner;
} state;

//

// TODO: move to utils
static bool executableExistsInPath(const std::string_view& exe) {
    const char* PATHENV = std::getenv("PATH");
    if (!PATHENV)
        return false;

    CVarList2       paths(PATHENV, 0, ':', true);
    std::error_code ec;

    for (const auto& PATH : paths) {
        std::filesystem::path candidate = std::filesystem::path(PATH) / exe;
        if (!std::filesystem::exists(candidate, ec) || ec)
            continue;
        if (!std::filesystem::is_regular_file(candidate, ec) || ec)
            continue;
        auto perms = std::filesystem::status(candidate, ec).permissions();
        if (ec)
            continue;
        if ((perms & std::filesystem::perms::others_exec) != std::filesystem::perms::none)
            return true;
    }

    return false;
}

static std::expected<void, std::string> commenceRun(const std::string_view& sv) {
    if (!executableExistsInPath(sv))
        return std::unexpected("Executable doesn't exist");

    CProcess proc(std::string{sv}, {});
    if (!proc.runAsync())
        return std::unexpected("Couldn't execute process.");

    if (!proc.pid())
        return std::unexpected("Process couldn't start");

    return {};
}

static bool tryRunApp() {
    if (const auto RES = commenceRun(state.textbox->currentText()); !RES) {
        if (!state.errorText) {
            state.errorText = CTextBuilder::begin()
                                  ->text(std::format("<i>{}</i>", RES.error()))
                                  ->fontSize(CFontSize{CFontSize::HT_FONT_TEXT})
                                  ->color([] { return CHyprColor{0xFFEE2222}; })
                                  ->commence();
            state.layoutInner->clearChildren();
            state.layoutInner->addChild(state.content);
            state.layoutInner->addChild(state.textbox);
            state.layoutInner->addChild(state.errorText);
        } else
            state.errorText->rebuild()->text(std::format("<i>{}</i>", RES.error()))->commence();

        return false;
    }

    return true;
}

int main(int argc, char** argv, char** envp) {
    backend = IBackend::create();

    //
    const Vector2D WINDOW_SIZE = {350, 100};
    auto           window = CWindowBuilder::begin()->preferredSize(WINDOW_SIZE)->minSize(WINDOW_SIZE)->maxSize(WINDOW_SIZE)->appTitle("Run")->appClass("hyprland-run")->commence();

    window->m_rootElement->addChild(CRectangleBuilder::begin()->color([] { return backend->getPalette()->m_colors.background; })->commence());

    auto layout = CColumnLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})->commence();
    layout->setMargin(3);

    state.layoutInner = CColumnLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {0.85F, 1.F}})->gap(10)->commence();

    window->m_rootElement->addChild(layout);

    layout->addChild(state.layoutInner);
    state.layoutInner->setGrow(true);

    state.content =
        CTextBuilder::begin()->text("Run an application")->fontSize(CFontSize{CFontSize::HT_FONT_TEXT})->color([] { return backend->getPalette()->m_colors.text; })->commence();

    state.textbox = CTextboxBuilder::begin()
                        ->placeholder("Input the app name...")
                        ->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {250, 25}})
                        ->multiline(false)
                        ->commence();

    std::vector<SP<CButtonElement>> buttons;

    buttons.emplace_back(CButtonBuilder::begin()
                             ->label("Cancel")
                             ->onMainClick([w = WP<IWindow>{window}](auto) {
                                 if (w)
                                     w->close();
                                 backend->destroy();
                             })
                             ->size({CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}})
                             ->commence());

    buttons.emplace_back(CButtonBuilder::begin()
                             ->label("Run")
                             ->onMainClick([w = WP<IWindow>{window}](auto) {
                                 if (!tryRunApp())
                                     return;

                                 if (state.textbox->currentText().empty())
                                     return;

                                 if (w)
                                     w->close();
                                 backend->destroy();
                             })
                             ->size({CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}})
                             ->commence());

    auto null2 = CNullBuilder::begin()->commence();

    auto layout2 = CRowLayoutBuilder::begin()->gap(3)->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->commence();

    null2->setGrow(true);

    window->m_events.keyboardKey.listenStatic([w = WP<IWindow>{window}](Input::SKeyboardKeyEvent ev) {
        if (ev.xkbKeysym == XKB_KEY_Return && tryRunApp()) {
            if (w)
                w->close();
            backend->destroy();
        }
    });

    state.layoutInner->addChild(state.content);
    state.layoutInner->addChild(state.textbox);

    layout2->addChild(null2);
    for (const auto& b : buttons) {
        layout2->addChild(b);
    }

    layout->addChild(layout2);

    window->m_events.closeRequest.listenStatic([w = WP<IWindow>{window}] {
        w->close();
        backend->destroy();
    });

    window->open();

    backend->enterLoop();

    return 0;
}