/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore BVBA and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "autobotmodule.h"

#include <QQmlEngine>

#include "modularity/ioc.h"
#include "ui/iinteractiveuriregister.h"
#include "ui/iuiactionsregister.h"

#include "internal/autobot.h"
#include "internal/autobotconfiguration.h"
#include "view/autobotscriptsmodel.h"
#include "view/testcaserunmodel.h"

#include "draw/painter.h"
#include "internal/draw/abpaintprovider.h"
#include "internal/autobotactionscontroller.h"
#include "internal/autobotactions.h"
#include "internal/autobotscriptsrepository.h"

#include "internal/api/apiregister.h"
#include "internal/api/logapi.h"
#include "internal/api/autobotapi.h"
#include "internal/api/dispatcherapi.h"
#include "internal/api/navigationapi.h"
#include "internal/api/contextapi.h"
#include "internal/api/shortcutsapi.h"
#include "internal/api/interactiveapi.h"
#include "internal/api/keyboardapi.h"
#include "internal/api/accessibilityapi.h"

#include "diagnostics/idiagnosticspathsregister.h"

using namespace mu::autobot;
using namespace mu::api;

static std::shared_ptr<AutobotConfiguration> s_configuration = std::make_shared<AutobotConfiguration>();
static std::shared_ptr<Autobot> s_autobot = std::make_shared<Autobot>();
static std::shared_ptr<AutobotActionsController> s_actionsController = std::make_shared<AutobotActionsController>();

std::string AutobotModule::moduleName() const
{
    return "autobot";
}

void AutobotModule::registerExports()
{
    modularity::ioc()->registerExport<IAutobot>(moduleName(), s_autobot);
    modularity::ioc()->registerExport<IAutobotConfiguration>(moduleName(), s_configuration);
    modularity::ioc()->registerExport<IAutobotScriptsRepository>(moduleName(), new AutobotScriptsRepository());

    modularity::ioc()->registerExport<IApiRegister>(moduleName(), new ApiRegister());

    // draw::Painter::extended = AbPaintProvider::instance();
}

void AutobotModule::resolveImports()
{
    auto ir = modularity::ioc()->resolve<ui::IInteractiveUriRegister>(moduleName());
    if (ir) {
        ir->registerQmlUri(Uri("musescore://autobot/batchtests"), "MuseScore/Autobot/BatchTestsDialog.qml");
        ir->registerQmlUri(Uri("musescore://autobot/scripts"), "MuseScore/Autobot/ScriptsDialog.qml");
        ir->registerQmlUri(Uri("musescore://autobot/selectfile"), "MuseScore/Autobot/AutobotSelectFileDialog.qml");
    }

    auto ar = modularity::ioc()->resolve<ui::IUiActionsRegister>(moduleName());
    if (ar) {
        ar->reg(std::make_shared<AutobotActions>());
    }

    auto api = modularity::ioc()->resolve<IApiRegister>(moduleName());
    if (api) {
        api->regApiCreator("global", "api.log", new ApiCreator<LogApi>());
        api->regApiCreator("autobot", "api.autobot", new ApiCreator<AutobotApi>());
        api->regApiCreator("autobot", "api.context", new ApiCreator<ContextApi>());
        api->regApiCreator("actions", "api.dispatcher", new ApiCreator<DispatcherApi>());
        api->regApiCreator("ui", "api.navigation", new ApiCreator<NavigationApi>());
        api->regApiCreator("shortcuts", "api.shortcuts", new ApiCreator<ShortcutsApi>());
        api->regApiCreator("global", "api.interactive", new ApiCreator<InteractiveApi>());
        api->regApiCreator("ui", "api.keyboard", new ApiCreator<KeyboardApi>());
        api->regApiCreator("accessibility", "api.accessibility", new ApiCreator<AccessibilityApi>());
    }
}

void AutobotModule::registerUiTypes()
{
    qmlRegisterType<AutobotScriptsModel>("MuseScore.Autobot", 1, 0, "AutobotScriptsModel");
    qmlRegisterType<TestCaseRunModel>("MuseScore.Autobot", 1, 0, "TestCaseRunModel");
}

void AutobotModule::onInit(const framework::IApplication::RunMode&)
{
    s_autobot->init();
    s_actionsController->init();

    //! --- Diagnostics ---
    auto pr = modularity::ioc()->resolve<diagnostics::IDiagnosticsPathsRegister>(moduleName());
    if (pr) {
        for (const io::path_t& p : s_configuration->scriptsDirPaths()) {
            pr->reg("autobotScriptsPath", p);
        }
        for (const io::path_t& p : s_configuration->testingFilesDirPaths()) {
            pr->reg("autobotTestingFilesPath", p);
        }
        pr->reg("autobotDataPath", s_configuration->dataPath());
        pr->reg("autobotSavingFilesPath", s_configuration->savingFilesPath());
        pr->reg("autobotReportsPath", s_configuration->reportsPath());
        pr->reg("autobotDrawDataPath", s_configuration->drawDataPath());
    }
}
