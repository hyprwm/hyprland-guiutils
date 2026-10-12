#pragma once

#include <unordered_map>
#include <cstdint>
#include <string>

#include <hyprutils/memory/SharedPtr.hpp>

template <typename T>
using SP = Hyprutils::Memory::CSharedPointer<T>;

namespace I18n {

    enum eI18nKeys : uint8_t {
        TXT_KEY_DONATE_TITLE = 0,
        TXT_KEY_DONATE_CONTENT,
        TXT_KEY_DONATE_SUPPORT,
        TXT_KEY_DONATE_THANKYOU,
        TXT_KEY_DONATE_NOTHANKS,
    };

    class CI18nEngine {
      public:
        CI18nEngine();
        ~CI18nEngine() = default;

        std::string localize(eI18nKeys key, const std::unordered_map<std::string, std::string>& vars = {});
    };

    SP<CI18nEngine> i18nEngine();
};