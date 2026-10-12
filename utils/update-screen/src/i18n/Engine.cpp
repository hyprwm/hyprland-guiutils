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
    registerEntry("de_CH", TXT_KEY_UPDATE_TITLE, "Hyprland aktualisiert");
    registerEntry("de_CH", TXT_KEY_UPDATE_TITLEVER, "Hyprland zo {version} aktualisiert!");
    registerEntry("de_CH", TXT_KEY_UPDATE_CONTENT, R"#(Hyprland esch aktualisiert worde! 😄

Lueg d Versionshiwiis uf GitHub ond d Neuigkeite uf hypr.land aa, om z erfahre was neu esch.

Gwössnigi Versione beinhalted inkompatibli Änderige, wenn du also Konfigurationsfähler bechonsch, denn send die neuste Versionshiwiis en guete Startponkt.

Falls du Plugins bruuchsch, vergess ned sie neu z kompiliere.

<i>Du chasch die Aasecht i dinere Hyprland-Konfiguration deaktiviere.</i>)#");
    registerEntry("de_CH", TXT_KEY_UPDATE_SUPPORT, "💝 Onderstötze");
    registerEntry("de_CH", TXT_KEY_UPDATE_THANKYOU, "💝 Danke der!");
    registerEntry("de_CH", TXT_KEY_UPDATE_NEWS, "🔗 Neuigkeite öffne");
    registerEntry("de_CH", TXT_KEY_UPDATE_NEWSDONE, "🔗 Sofort!");
    registerEntry("de_CH", TXT_KEY_UPDATE_THANKS, "Danke");

    // de_DE (German)
    registerEntry("de_DE", TXT_KEY_UPDATE_TITLE, "Hyprland aktualisiert");
    registerEntry("de_DE", TXT_KEY_UPDATE_TITLEVER, "Hyprland zu {version} aktualisiert!");
    registerEntry("de_DE", TXT_KEY_UPDATE_CONTENT, R"#(Hyprland wurde aktualisiert! 😄

Schau dir die Versionshinweise auf GitHub und die Neuigkeiten auf hypr.land an, um zu erfahren, was neu ist.

Gewisse Versionen beinhalten inkompatible Änderungen. Wenn du also Konfigurationsfehler erhälst, dann sind die neusten Versionshinweise ein guter Startpunkt.

Falls du Plugins verwendest, vergiss nicht, sie neu zu kompilieren.

<i>Du kannst diese Ansicht in deiner Hyprland-Konfiguration deaktivieren.</i>)#");
    registerEntry("de_DE", TXT_KEY_UPDATE_SUPPORT, "💝 Unterstützen");
    registerEntry("de_DE", TXT_KEY_UPDATE_THANKYOU, "💝 Danke dir!");
    registerEntry("de_DE", TXT_KEY_UPDATE_NEWS, "🔗 Neuigkeiten öffnen");
    registerEntry("de_DE", TXT_KEY_UPDATE_NEWSDONE, "🔗 Sofort!");
    registerEntry("de_DE", TXT_KEY_UPDATE_THANKS, "Danke");

    // en_US (English)
    registerEntry("en_US", TXT_KEY_UPDATE_TITLE, "Hyprland Updated");
    registerEntry("en_US", TXT_KEY_UPDATE_TITLEVER, "Hyprland updated to {version}!");
    registerEntry("en_US", TXT_KEY_UPDATE_CONTENT, R"#(Hyprland has been updated! 😄

Check out the release notes on GitHub and the news page on hypr.land to see what's new.

Some releases include breaking changes, so if you run into any config errors, the latest release notes are a good place to start.

If you use plugins, make sure to rebuild them.

<i>You can turn off this screen in your Hyprland config.</i>)#");
    registerEntry("en_US", TXT_KEY_UPDATE_SUPPORT, "💝 Support");
    registerEntry("en_US", TXT_KEY_UPDATE_THANKYOU, "💝 Thank you!");
    registerEntry("en_US", TXT_KEY_UPDATE_NEWS, "🔗 Open news");
    registerEntry("en_US", TXT_KEY_UPDATE_NEWSDONE, "🔗 Right away!");
    registerEntry("en_US", TXT_KEY_UPDATE_THANKS, "Thanks");

    // it_IT (Italian)
    registerEntry("it_IT", TXT_KEY_UPDATE_TITLE, "Hyprland Aggiornato");
    registerEntry("it_IT", TXT_KEY_UPDATE_TITLEVER, "Hyprland aggiornato a {version}!");
    registerEntry("it_IT", TXT_KEY_UPDATE_CONTENT, R"#(Hyprland è stato aggiornato! 😄

Dai un'occhiata alle note di rilascio su GitHub e sulla pagina sulle novità su hypr.land per vedere cosa è cambiato.

Alcune versioni includono modifiche critiche. Se incontri errori di configurazione, le note di rilascio sono un buon posto dove iniziare.

Se usi dei plugin, assicurati di aggiornarli.

<i>Puoi disattivare questa schermata nel tuo config di Hyprland.</i>)#");
    registerEntry("it_IT", TXT_KEY_UPDATE_SUPPORT, "💝 Supporta");
    registerEntry("it_IT", TXT_KEY_UPDATE_THANKYOU, "💝 Grazie!");
    registerEntry("it_IT", TXT_KEY_UPDATE_NEWS, "🔗 Apri novità");
    registerEntry("it_IT", TXT_KEY_UPDATE_NEWSDONE, "🔗 Subito!");
    registerEntry("it_IT", TXT_KEY_UPDATE_THANKS, "Grazie");
}

std::string I18n::CI18nEngine::localize(eI18nKeys key, const Hyprutils::I18n::translationVarMap& vars) {
    return huEngine->localizeEntry(localeStr, key, vars);
}
