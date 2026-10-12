#pragma once

#include <unordered_map>
#include <cstdint>
#include <string>

#include <hyprutils/memory/SharedPtr.hpp>

template <typename T>
using SP = Hyprutils::Memory::CSharedPointer<T>;

namespace I18n {

    enum eI18nKeys : uint8_t {
        TXT_KEY_RUN_TITLE = 0,
        TXT_KEY_RUN_NOFOUND,
        TXT_KEY_RUN_NORUN,
        TXT_KEY_RUN_NOSTART,
        TXT_KEY_RUN_INPUT,
        TXT_KEY_RUN_CANCEL,
        TXT_KEY_RUN_RUN,
    };

    class CI18nEngine {
      public:
        CI18nEngine();
        ~CI18nEngine() = default;

        std::string localize(eI18nKeys key, const std::unordered_map<std::string, std::string>& vars = {});
    };

    SP<CI18nEngine> i18nEngine();
};