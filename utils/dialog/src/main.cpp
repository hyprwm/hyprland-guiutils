#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/Null.hpp>

#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/memory/UniquePtr.hpp>
#include <hyprutils/string/ConstVarList.hpp>
#include <hyprutils/string/String.hpp>

#include <print>
#include <algorithm>

using namespace Hyprutils::Memory;
using namespace Hyprutils::Math;
using namespace Hyprutils::String;
using namespace Hyprtoolkit;

#define SP CSharedPointer
#define WP CWeakPointer
#define UP CUniquePointer

static SP<IBackend> backend;
static SP<IWindow>  window;

//
int main(int argc, char** argv, char** envp) {
    setenv("HT_QUIET", "1", true);
    backend = IBackend::create();

    std::string              appTitle = "", titleStr = "", textStr = "";
    std::vector<std::string> buttonsStrs;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        if (arg == "--title") {
            if (i + 1 >= argc) {
                std::print(stderr, "--title requires a parameter\n");
                return 1;
            }

            titleStr = argv[i + 1];

            i++;
            continue;
        }

        if (arg == "--apptitle") {
            if (i + 1 >= argc) {
                std::print(stderr, "--apptitle requires a parameter\n");
                return 1;
            }

            appTitle = argv[i + 1];

            i++;
            continue;
        }

        if (arg == "--text") {
            if (i + 1 >= argc) {
                std::print(stderr, "--text requires a parameter\n");
                return 1;
            }

            textStr = argv[i + 1];

            i++;
            continue;
        }

        if (arg == "--buttons") {
            if (i + 1 >= argc) {
                std::print(stderr, "--buttons requires a parameter\n");
                return 1;
            }

            CConstVarList buttonz(argv[i + 1], 0, ';', true);

            for (auto& b : buttonz) {
                buttonsStrs.emplace_back(b);
            }

            i++;
            continue;
        }

        std::print(stderr, "invalid arg {}\n", argv[i]);
        return 1;
    }

    // for compatibility, let's fix newlines.
    replaceInString(textStr, "<br/>", "\n");
    replaceInString(textStr, "\\n", "\n");

    const auto  FONT_SIZE = CFontSize{CFontSize::HT_FONT_TEXT}.ptSize();
    const float WINDOW_X  = FONT_SIZE * 50.F;

    // for now do auto to estimate size
    auto layout = CColumnLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_AUTO, {WINDOW_X, 1.F}})->commence();
    layout->setMargin(3);

    auto layoutInner = CColumnLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {0.85F, 1.F}})->commence();

    layout->addChild(layoutInner);
    layoutInner->setGrow(true);
    layoutInner->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_ABSOLUTE);
    layoutInner->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_HCENTER, true);

    auto title =
        CTextBuilder::begin()->text(std::move(titleStr))->fontSize({CFontSize::HT_FONT_H2})->async(false)->color([] { return backend->getPalette()->m_colors.text; })->commence();

    auto hr = CRectangleBuilder::begin() //
                  ->color([] { return CHyprColor{backend->getPalette()->m_colors.text.darken(0.65)}; })
                  ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {0.5F, 9.F}})
                  ->commence();

    hr->setMargin(4);

    auto content = CTextBuilder::begin()
                       ->text(std::move(textStr))
                       ->fontSize(CFontSize{CFontSize::HT_FONT_TEXT})
                       ->async(false)
                       ->color([] { return backend->getPalette()->m_colors.text; })
                       ->commence();

    std::vector<SP<CButtonElement>> buttons;

    for (const auto& bstr : buttonsStrs) {
        buttons.emplace_back(CButtonBuilder::begin()
                                 ->label(std::string{bstr})
                                 ->onMainClick([str = bstr](auto) {
                                     std::println("{}", str);
                                     if (window)
                                         window->close();
                                     backend->destroy();
                                 })
                                 ->size({CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}})
                                 ->commence());
    }

    auto null2 = CNullBuilder::begin()->commence();

    auto layout2 = CRowLayoutBuilder::begin()->gap(3)->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->commence();

    null2->setGrow(true);

    layoutInner->addChild(title);
    layoutInner->addChild(hr);
    layoutInner->addChild(content);

    layout2->addChild(null2);
    for (const auto& b : buttons) {
        layout2->addChild(b);
    }

    layout->addChild(layout2);

    {
        // calculate the layout size before we open the window to size it properly
        SP<CNullElement> null = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {WINDOW_X, 99999999.F}})->commence();

        null->addChild(layout);
        layout->forceReposition();
    }

    const float WINDOW_Y = layout->size().y + 50 /* pad */;

    layout->rebuild()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})->commence();

    //
    window = CWindowBuilder::begin()
                 ->preferredSize({WINDOW_X, WINDOW_Y})
                 ->minSize({WINDOW_X, WINDOW_Y})
                 ->maxSize({WINDOW_X, WINDOW_Y})
                 ->appTitle(std::move(appTitle))
                 ->appClass("hyprland-dialog")
                 ->commence();

    window->m_rootElement->addChild(CRectangleBuilder::begin()->color([] { return backend->getPalette()->m_colors.background; })->commence());
    window->m_rootElement->addChild(layout);

    window->m_events.closeRequest.listenStatic([w = WP<IWindow>{window}] {
        w->close();
        backend->destroy();
    });

    window->open();

    backend->enterLoop();

    return 0;
}