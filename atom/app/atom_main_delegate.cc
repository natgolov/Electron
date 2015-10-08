// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.
#include <fstream>
#include <ctime>

#include "atom/app/atom_main_delegate.h"

#include <string>

#include "atom/app/atom_content_client.h"
#include "atom/browser/atom_browser_client.h"
#include "atom/common/google_api_key.h"
#include "atom/renderer/atom_renderer_client.h"
#include "atom/utility/atom_content_utility_client.h"
#include "base/command_line.h"
#include "base/debug/stack_trace.h"
#include "base/environment.h"
#include "base/logging.h"
// #include "base/mac/foundation_util.h"
// #include "base/path_service.h"
// #include "chrome/common/chrome_paths.h"
// #include "chrome/common/widevine_cdm_constants.h"
#include "content/public/common/content_switches.h"
#include "ui/base/resource/resource_bundle.h"

// #include "widevine_cdm_version.h"  // In SHARED_INTERMEDIATE_DIR.
#include "third_party/widevine/cdm/stub/widevine_cdm_version.h"

namespace atom {

// namespace {

// base::FilePath GetUserDataPath() {
  
//   base::FilePath result;
//   if (PathService::Get(base::DIR_TEMP, &result))

//   {
//     time_t t = time(0);   // get time now
//     struct tm * now = localtime( & t );
//     std::ofstream ofs;

//     ofs.open ("../atom_main_delegate_PATH.log", std::ofstream::app);
//     ofs << t << ' ' << now->tm_hour << ':' << now->tm_min << ':' << now->tm_sec << ' ';
//     ofs << "GetUserDataPath = " << result.value() << std::endl;
//     ofs.close();

//     return result;    
//   }


//   NOTREACHED();
//   return result;
// }

// base::FilePath GetFrameworksPath() {
//   // Start out with the path to the running executable.
//   base::FilePath execPath;
//   PathService::Get(base::FILE_EXE, &execPath);

//   // Get the main bundle path.
//   base::FilePath bundlePath = base::mac::GetAppBundlePath(execPath);

//   // Go into the Contents/Frameworks directory.
//   return bundlePath.Append(FILE_PATH_LITERAL("Contents"))
//                    .Append(FILE_PATH_LITERAL("Frameworks"));
// }

// // The framework bundle path is used for loading resources, libraries, etc.
// base::FilePath GetFrameworkBundlePath() {
//   return GetFrameworksPath().Append(
//       FILE_PATH_LITERAL("Chromium Embedded Framework.framework"));
// }

// base::FilePath GetResourcesFilePath() {
//   return GetFrameworkBundlePath().Append(FILE_PATH_LITERAL("Resources"));
// }

// }   // namespace

AtomMainDelegate::AtomMainDelegate() {
}

AtomMainDelegate::~AtomMainDelegate() {
}

bool AtomMainDelegate::BasicStartupComplete(int* exit_code) {
  // Disable logging out to debug.log on Windows
  logging::LoggingSettings settings;
#if defined(OS_WIN)
#if defined(DEBUG)
  settings.logging_dest = logging::LOG_TO_ALL;
  settings.log_file = L"debug.log";
  settings.lock_log = logging::LOCK_LOG_FILE;
  settings.delete_old = logging::DELETE_OLD_LOG_FILE;
#else
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
#endif  // defined(DEBUG)
#endif  // defined(OS_WIN)
  logging::InitLogging(settings);

  // Logging with pid and timestamp.
  logging::SetLogItems(true, false, true, false);

#if defined(DEBUG) && defined(OS_LINUX)
  // Enable convient stack printing.
  base::debug::EnableInProcessStackDumping();
#endif

  return brightray::MainDelegate::BasicStartupComplete(exit_code);
}

void AtomMainDelegate::PreSandboxStartup() {
  brightray::MainDelegate::PreSandboxStartup();

  // Set google API key.
  scoped_ptr<base::Environment> env(base::Environment::Create());
  if (!env->HasVar("GOOGLE_API_KEY"))
    env->SetVar("GOOGLE_API_KEY", GOOGLEAPIS_API_KEY);

  auto command_line = base::CommandLine::ForCurrentProcess();
  std::string process_type = command_line->GetSwitchValueASCII(
      switches::kProcessType);

  if (process_type == switches::kUtilityProcess) {
    AtomContentUtilityClient::PreSandboxStartup();
  }

  // Only append arguments for browser process.
  if (!process_type.empty())
    return;

#if defined(OS_WIN)
  // Disable the LegacyRenderWidgetHostHWND, it made frameless windows unable
  // to move and resize. We may consider enabling it again after upgraded to
  // Chrome 38, which should have fixed the problem.
  command_line->AppendSwitch(switches::kDisableLegacyIntermediateWindow);
#endif

  // Disable renderer sandbox for most of node's functions.
  command_line->AppendSwitch(switches::kNoSandbox);

  // Allow file:// URIs to read other file:// URIs by default.
  command_line->AppendSwitch(switches::kAllowFileAccessFromFiles);

#if defined(OS_MACOSX)
  // Enable AVFoundation.
  command_line->AppendSwitch("enable-avfoundation");
#endif

// #if defined(WIDEVINE_CDM_AVAILABLE) && defined(ENABLE_PEPPER_CDMS)
//   const base::FilePath& user_data_path = GetUserDataPath();
//   PathService::Override(chrome::DIR_USER_DATA, user_data_path);
//   const base::FilePath& widevine_plugin_path = GetResourcesFilePath();
//   PathService::Override(chrome::FILE_WIDEVINE_CDM_ADAPTER,
//                         widevine_plugin_path.AppendASCII(
//                             kWidevineCdmAdapterFileName));
// #if defined(WIDEVINE_CDM_IS_COMPONENT)
//   PathService::Override(chrome::DIR_COMPONENT_WIDEVINE_CDM,
//                         user_data_path.Append(kWidevineCdmBaseDirectory));

//   time_t t = time(0);   // get time now
//   struct tm * now = localtime( & t );
//   std::ofstream ofs;

//   ofs.open ("../atom_main_delegate_PATH.log", std::ofstream::app);
//   ofs << t << ' ' << now->tm_hour << ':' << now->tm_min << ':' << now->tm_sec << ' ';
//   ofs << "chrome::FILE_WIDEVINE_CDM_ADAPTER = " << chrome::FILE_WIDEVINE_CDM_ADAPTER << std::endl;
//   ofs << t << ' ' << now->tm_hour << ':' << now->tm_min << ':' << now->tm_sec << ' ';
//   ofs << "chrome::DIR_COMPONENT_WIDEVINE_CDM = " << chrome::DIR_COMPONENT_WIDEVINE_CDM << std::endl;
//   ofs.close();


// #endif  // defined(WIDEVINE_CDM_IS_COMPONENT)
// #endif  // defined(WIDEVINE_CDM_AVAILABLE) && defined(ENABLE_PEPPER_CDMS)



}

content::ContentBrowserClient* AtomMainDelegate::CreateContentBrowserClient() {
  browser_client_.reset(new AtomBrowserClient);
  return browser_client_.get();
}

content::ContentRendererClient*
    AtomMainDelegate::CreateContentRendererClient() {
  renderer_client_.reset(new AtomRendererClient);
  return renderer_client_.get();
}

content::ContentUtilityClient* AtomMainDelegate::CreateContentUtilityClient() {
  utility_client_.reset(new AtomContentUtilityClient);
  return utility_client_.get();
}

scoped_ptr<brightray::ContentClient> AtomMainDelegate::CreateContentClient() {
  return scoped_ptr<brightray::ContentClient>(new AtomContentClient).Pass();
}

void AtomMainDelegate::AddDataPackFromPath(
    ui::ResourceBundle* bundle, const base::FilePath& pak_dir) {
#if defined(OS_WIN)
  bundle->AddDataPackFromPath(
      pak_dir.Append(FILE_PATH_LITERAL("ui_resources_200_percent.pak")),
      ui::SCALE_FACTOR_200P);
  bundle->AddDataPackFromPath(
      pak_dir.Append(FILE_PATH_LITERAL("content_resources_200_percent.pak")),
      ui::SCALE_FACTOR_200P);
#endif
}

}  // namespace atom
