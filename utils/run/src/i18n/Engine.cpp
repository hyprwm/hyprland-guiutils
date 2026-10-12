#include "Engine.hpp"

#include <hyprutils/i18n/I18nEngine.hpp>
#include <hyprutils/memory/SharedPtr.hpp>
using namespace Hyprutils::Memory;

using namespace I18n;
using namespace Hyprutils::I18n;

static SP<Hyprutils::I18n::CI18nEngine> huEngine;
static std::string                      localeStr;

//
SP<I18n::CI18nEngine> I18n::i18nEngine() {
    static SP<I18n::CI18nEngine> engine = makeShared<I18n::CI18nEngine>();
    return engine;
}

// prevents CI18nEngine constructor from being bloated by std::string allocations/deallocations
[[gnu::noinline]] static void registerEntry(const char* locale, eI18nKeys key, const char* translation) {
    huEngine->registerEntry(locale, key, translation);
}

/* would be needed for plurals
[[gnu::noinline]] static void registerEntry(const char* locale, eI18nKeys key, const char* (*translationFunc)(const Hyprutils::I18n::translationVarMap&)) {
    huEngine->registerEntry(locale, key, translationFunc);
}
*/

I18n::CI18nEngine::CI18nEngine() {
    huEngine = makeShared<Hyprutils::I18n::CI18nEngine>();
    huEngine->setFallbackLocale("en_US");
    localeStr = huEngine->getSystemLocale().locale();

    // de_CH (Swiss German)
    registerEntry("de_CH", TXT_KEY_RUN_TITLE, "Applikation uusfüehre");
    registerEntry("de_CH", TXT_KEY_RUN_NOFOUND, "Uusfüehrbari Datei existiert ned");
    registerEntry("de_CH", TXT_KEY_RUN_NORUN, "Prozäss hed ned chönne uusgfüehrt wärde");
    registerEntry("de_CH", TXT_KEY_RUN_NOSTART, "Prozäss hed ned chönne gstartet wärde");
    registerEntry("de_CH", TXT_KEY_RUN_INPUT, "Programmname iigä...");
    registerEntry("de_CH", TXT_KEY_RUN_CANCEL, "Abbräche");
    registerEntry("de_CH", TXT_KEY_RUN_RUN, "Uusfüehre");

    // de_DE (German)
    registerEntry("de_DE", TXT_KEY_RUN_TITLE, "Applikation ausführen");
    registerEntry("de_DE", TXT_KEY_RUN_NOFOUND, "Ausführbare Datei existiert nicht");
    registerEntry("de_DE", TXT_KEY_RUN_NORUN, "Prozess konnte nicht ausgeführt werden");
    registerEntry("de_DE", TXT_KEY_RUN_NOSTART, "Prozess konnte nicht gestartet werden");
    registerEntry("de_DE", TXT_KEY_RUN_INPUT, "Programmname eingeben...");
    registerEntry("de_DE", TXT_KEY_RUN_CANCEL, "Abbrechen");
    registerEntry("de_DE", TXT_KEY_RUN_RUN, "Ausführen");

    // en_US (English)
    registerEntry("en_US", TXT_KEY_RUN_TITLE, "Run an appplication");
    registerEntry("en_US", TXT_KEY_RUN_NOFOUND, "Executable doesn't exist");
    registerEntry("en_US", TXT_KEY_RUN_NORUN, "Couldn't execute process");
    registerEntry("en_US", TXT_KEY_RUN_NOSTART, "Process couldn't start");
    registerEntry("en_US", TXT_KEY_RUN_INPUT, "Input the app name...");
    registerEntry("en_US", TXT_KEY_RUN_CANCEL, "Cancel");
    registerEntry("en_US", TXT_KEY_RUN_RUN, "Run");

    // it_IT (Italian)
    registerEntry("it_IT", TXT_KEY_RUN_TITLE, "Esegui un'applicazione");
    registerEntry("it_IT", TXT_KEY_RUN_NOFOUND, "L'eseguibile non esiste");
    registerEntry("it_IT", TXT_KEY_RUN_NORUN, "Impossibile eseguire il processo");
    registerEntry("it_IT", TXT_KEY_RUN_NOSTART, "Impossibile avviare il processo");
    registerEntry("it_IT", TXT_KEY_RUN_INPUT, "Inserisci il nome dell'appp...");
    registerEntry("it_IT", TXT_KEY_RUN_CANCEL, "Annulla");
    registerEntry("it_IT", TXT_KEY_RUN_RUN, "Esegui");
}

std::string I18n::CI18nEngine::localize(eI18nKeys key, const Hyprutils::I18n::translationVarMap& vars) {
    return huEngine->localizeEntry(localeStr, key, vars);
}
