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
#include <hyprutils/os/Process.hpp>

#include <print>

using namespace Hyprutils::Memory;
using namespace Hyprutils::Math;
using namespace Hyprutils::String;
using namespace Hyprutils::OS;
using namespace Hyprtoolkit;

#define SP CSharedPointer
#define WP CWeakPointer
#define UP CUniquePointer

static SP<IBackend>       backend;
static SP<CButtonElement> donateButton, openNewsButton;

constexpr const char*     TEXT_CONTENT = R"#(Hyprland has been updated! 😄

Please check the release notes, which can be found on GitHub, as well as the news page on hypr.land.

Every release may come with breaking changes, so if you get any config errors, try checking the latest release notes.

If you are using plugins, you will need to rebuild them.

<i>You can disable this screen in your Hyprland config.</i>)#";

//
int main(int argc, char** argv, char** envp) {
    setenv("HT_QUIET", "1", true);
    backend = IBackend::create();

    std::string              versionStr = "";
    std::vector<std::string> buttonsStrs;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        if (arg == "--new-version") {
            if (i + 1 >= argc) {
                std::print(stderr, "--new-version requires a parameter\n");
                return 1;
            }

            versionStr = argv[i + 1];

            i++;
            continue;
        }

        std::print(stderr, "invalid arg {}\n", argv[i]);
        return 1;
    }

    //
    const auto FONT_SIZE   = CFontSize{CFontSize::HT_FONT_TEXT}.ptSize();
    const auto WINDOW_SIZE = Vector2D{FONT_SIZE * 90.F, FONT_SIZE * 30.F};

    auto       window = CWindowBuilder::begin()
                      ->preferredSize(WINDOW_SIZE)
                      ->minSize(WINDOW_SIZE)
                      ->maxSize(WINDOW_SIZE)
                      ->appTitle("Hyprland Updated")
                      ->appClass("hyprland-donate-screen")
                      ->commence();

    window->m_rootElement->addChild(CRectangleBuilder::begin()->color([] { return backend->getPalette()->m_colors.background; })->commence());

    auto layout = CColumnLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})->commence();
    layout->setMargin(3);

    auto layoutInner = CColumnLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {0.85F, 1.F}})->commence();

    window->m_rootElement->addChild(layout);

    layout->addChild(layoutInner);
    layoutInner->setGrow(true);
    layoutInner->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_ABSOLUTE);
    layoutInner->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_HCENTER, true);

    auto title = CTextBuilder::begin()
                     ->text(std::move(std::format("Hyprland updated to {}!", versionStr)))
                     ->fontSize({CFontSize::HT_FONT_H2})
                     ->color([] { return backend->getPalette()->m_colors.text; })
                     ->commence();

    auto hr = CRectangleBuilder::begin() //
                  ->color([] { return CHyprColor{backend->getPalette()->m_colors.text.darken(0.65)}; })
                  ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {0.5F, 9.F}})
                  ->commence();

    hr->setMargin(4);

    auto content = CTextBuilder::begin()
                       ->text(TEXT_CONTENT)
                       ->fontSize(CFontSize{CFontSize::HT_FONT_TEXT})
                       ->align(Hyprtoolkit::HT_FONT_ALIGN_CENTER)
                       ->color([] { return backend->getPalette()->m_colors.text; })
                       ->commence();

    auto null2 = CNullBuilder::begin()->commence();

    auto layout2 = CRowLayoutBuilder::begin()->gap(3)->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->commence();

    donateButton = CButtonBuilder::begin()
                       ->label("💝 Support")
                       ->onMainClick([w = WP<IWindow>{window}](auto) {
                           donateButton->rebuild()->label("💝 Thank you!")->commence();

                           CProcess proc("xdg-open", {"https://hypr.land/support"});
                           proc.runAsync();
                       })
                       ->size({CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}})
                       ->commence();

    openNewsButton = CButtonBuilder::begin()
                         ->label("🔗 Open news")
                         ->onMainClick([w = WP<IWindow>{window}](auto) {
                             openNewsButton->rebuild()->label("🔗 Right away!")->commence();

                             CProcess proc("xdg-open", {"https://hypr.land/news"});
                             proc.runAsync();
                         })
                         ->size({CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}})
                         ->commence();

    auto closeButton = CButtonBuilder::begin()
                           ->label("Thanks")
                           ->onMainClick([w = WP<IWindow>{window}](auto) {
                               if (w)
                                   w->close();
                               backend->destroy();
                           })
                           ->size({CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}})
                           ->commence();

    null2->setGrow(true);

    layoutInner->addChild(title);
    layoutInner->addChild(hr);
    layoutInner->addChild(content);

    layout2->addChild(null2);
    layout2->addChild(donateButton);
    layout2->addChild(openNewsButton);
    layout2->addChild(closeButton);

    layout->addChild(layout2);

    window->m_events.closeRequest.listenStatic([w = WP<IWindow>{window}] {
        w->close();
        backend->destroy();
    });

    window->open();

    backend->enterLoop();

    return 0;
}