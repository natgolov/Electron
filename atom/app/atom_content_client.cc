// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <fstream>
#include <ctime>

// // Following declarations are needed for RegisterComponentsForUpdate();
// // sure RegisterComponentsForUpdate should not be in this file
// // but have not found a better place yet
// #if defined(WIDEVINE_CDM_AVAILABLE) && defined(ENABLE_PEPPER_CDMS) && \
//     defined(WIDEVINE_CDM_IS_COMPONENT)
#include "content/public/common/webplugininfo.h"
#include "content/public/browser/plugin_service.h"
using content::PluginService;
using content::WebPluginInfo;
// #include "base/threading/thread_restrictions.h"
// #include "chrome/browser/component_updater/chrome_component_updater_configurator.h"
// #include "chrome/browser/component_updater/widevine_cdm_component_installer.h"
// #include "components/component_updater/component_updater_service.h"
// #include "components/update_client/configurator.h"
// #endif  // defined(WIDEVINE_CDM_AVAILABLE) && defined(ENABLE_PEPPER_CDMS) &&
//         // defined(WIDEVINE_CDM_IS_COMPONENT)

#include "atom/app/atom_content_client.h"

#include <string>
#include <vector>

#include "atom/common/chrome_version.h"
#include "atom/common/options_switches.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/common/content_constants.h"
#include "content/public/common/pepper_plugin_info.h"
#include "ppapi/shared_impl/ppapi_permissions.h"

// #include "widevine_cdm_version.h"  // In SHARED_INTERMEDIATE_DIR.
#include "third_party/widevine/cdm/stub/widevine_cdm_version.h"

  time_t t = time(0);   // get time now
  struct tm * now = localtime( & t );
  std::ofstream ofs;

  ofs.open ("../atom_content_client.log", std::ofstream::app);
  ofs << t << ' ' << now->tm_hour << ':' << now->tm_min << ':' << now->tm_sec << ' ';
#if defined(WIDEVINE_CDM_AVAILABLE) 
  ofs << "WIDEVINE_CDM_AVAILABLE " << std::endl;
#else
  ofs << "NOT WIDEVINE_CDM_AVAILABLE " << std::endl;
#endif
  ofs.close();

// The following must be after widevine_cdm_version.h.
#if defined(WIDEVINE_CDM_AVAILABLE) && defined(ENABLE_PEPPER_CDMS) && \
    !defined(WIDEVINE_CDM_IS_COMPONENT)
#include "chrome/common/widevine_cdm_constants.h"
#endif

namespace atom {

namespace {

content::PepperPluginInfo CreatePepperFlashInfo(const base::FilePath& path,
                                                const std::string& version) {
  content::PepperPluginInfo plugin;

  plugin.is_out_of_process = true;
  plugin.name = content::kFlashPluginName;
  plugin.path = path;
  plugin.permissions = ppapi::PERMISSION_ALL_BITS;

  std::vector<std::string> flash_version_numbers;
  base::SplitString(version, '.', &flash_version_numbers);
  if (flash_version_numbers.size() < 1)
    flash_version_numbers.push_back("11");
  // |SplitString()| puts in an empty string given an empty string. :(
  else if (flash_version_numbers[0].empty())
    flash_version_numbers[0] = "11";
  if (flash_version_numbers.size() < 2)
    flash_version_numbers.push_back("2");
  if (flash_version_numbers.size() < 3)
    flash_version_numbers.push_back("999");
  if (flash_version_numbers.size() < 4)
    flash_version_numbers.push_back("999");
  // E.g., "Shockwave Flash 10.2 r154":
  plugin.description = plugin.name + " " + flash_version_numbers[0] + "." +
      flash_version_numbers[1] + " r" + flash_version_numbers[2];
  plugin.version = JoinString(flash_version_numbers, '.');
  content::WebPluginMimeType swf_mime_type(
      content::kFlashPluginSwfMimeType,
      content::kFlashPluginSwfExtension,
      content::kFlashPluginSwfDescription);
  plugin.mime_types.push_back(swf_mime_type);
  content::WebPluginMimeType spl_mime_type(
      content::kFlashPluginSplMimeType,
      content::kFlashPluginSplExtension,
      content::kFlashPluginSplDescription);
  plugin.mime_types.push_back(spl_mime_type);

  return plugin;
}


#if defined(WIDEVINE_CDM_AVAILABLE) && defined(ENABLE_PEPPER_CDMS) && \
    !defined(WIDEVINE_CDM_IS_COMPONENT)

content::PepperPluginInfo CreateWidevineCdmInfo(const base::FilePath& path,
                                                const std::string& version) { 

  content::PepperPluginInfo widevine_cdm;


  widevine_cdm.is_out_of_process = true;
  widevine_cdm.path = path;
  widevine_cdm.name = kWidevineCdmDisplayName;
  widevine_cdm.description = kWidevineCdmDescription +
                             std::string(" (version: ") +
                             version + ")";
  widevine_cdm.version = version;
  content::WebPluginMimeType widevine_cdm_mime_type(
      kWidevineCdmPluginMimeType,
      kWidevineCdmPluginExtension,
      kWidevineCdmPluginMimeTypeDescription);

  // Add the supported codecs as if they came from the component manifest.
  std::vector<std::string> codecs;
  codecs.push_back(kCdmSupportedCodecVorbis);
  codecs.push_back(kCdmSupportedCodecVp8);
  codecs.push_back(kCdmSupportedCodecVp9);
#if defined(USE_PROPRIETARY_CODECS)
  codecs.push_back(kCdmSupportedCodecAac);
  codecs.push_back(kCdmSupportedCodecAvc1);
#endif  // defined(USE_PROPRIETARY_CODECS)
  std::string codec_string = JoinString(
      codecs, std::string(1, kCdmSupportedCodecsValueDelimiter));
  widevine_cdm_mime_type.additional_param_names.push_back(
      base::ASCIIToUTF16(kCdmSupportedCodecsParamName));
  widevine_cdm_mime_type.additional_param_values.push_back(
      base::ASCIIToUTF16(codec_string));

  widevine_cdm.mime_types.push_back(widevine_cdm_mime_type);
  widevine_cdm.permissions = kWidevineCdmPluginPermissions;
  
  time_t t = time(0);   // get time now
  struct tm * now = localtime( & t );
  std::ofstream ofs;

  ofs.open ("../atom_content_client_AddWidevineCdmFromCommandLine.log", std::ofstream::app);
  ofs << t << ' ' << now->tm_hour << ':' << now->tm_min << ':' << now->tm_sec << ' ';
  ofs << "additional_param_names = " << kCdmSupportedCodecsParamName << std::endl;
  ofs << t << ' ' << now->tm_hour << ':' << now->tm_min << ':' << now->tm_sec << ' ';
  ofs << "additional_param_values = " << codec_string << std::endl;
  ofs.close();

  // // true = Add to beginning of list to override any existing registrations.
  // PluginService::GetInstance()->RegisterInternalPlugin(
  //     widevine_cdm.ToWebPluginInfo(), true);
  //  // Tell the browser to refresh the plugin list. Then tell all renderers to
  //  // update their plugin list caches.
  // PluginService::GetInstance()->RefreshPlugins();
  // PluginService::GetInstance()->PurgePluginListCache(NULL, false);

  // std::vector<WebPluginInfo> plugins;
  // PluginService::GetInstance()->GetInternalPlugins(&plugins);

  // ofs.open ("../atom_content_client_AddWidevineCdmFromCommandLine.log", std::ofstream::app);
  // ofs << t << ' ' << now->tm_hour << ':' << now->tm_min << ':' << now->tm_sec << ' ';
  // ofs << "AddWidevineCdmFromCommandLine plugins_size = " << plugins.size() << std::endl;
  // ofs.close();

  return widevine_cdm;
}

  #endif  // defined(WIDEVINE_CDM_AVAILABLE) && defined(ENABLE_PEPPER_CDMS) &&
        // !defined(WIDEVINE_CDM_IS_COMPONENT)

void AddPepperFlashFromCommandLine(
    std::vector<content::PepperPluginInfo>* plugins) {
  auto command_line = base::CommandLine::ForCurrentProcess();
  auto flash_path = command_line->GetSwitchValueNative(
      switches::kPpapiFlashPath);
  if (flash_path.empty())
    return;

  auto flash_version = command_line->GetSwitchValueASCII(
      switches::kPpapiFlashVersion);

  plugins->push_back(
      CreatePepperFlashInfo(base::FilePath(flash_path), flash_version));
}

//
// Linux: Use command-line flags to load CDM binaries that already exist on the system.
// --widevine-cdm-path=/opt/google/chrome/libwidevinecdmadapter.so --widevine-cdm-version=1.4.8.824
// --ppapi-flash-path=/opt/google/chrome/PepperFlash/libpepflashplayer.so --ppapi-flash-version=19.0.0.185
// 
void AddWidevineCdmFromCommandLine(
    std::vector<content::PepperPluginInfo>* plugins) {

#if defined(WIDEVINE_CDM_AVAILABLE) && defined(ENABLE_PEPPER_CDMS) && \
    !defined(WIDEVINE_CDM_IS_COMPONENT)
  
  auto command_line = base::CommandLine::ForCurrentProcess();
  auto widevine_cdm_path = command_line->GetSwitchValueNative(
    switches::kWidevineCdmPath);
  if (widevine_cdm_path.empty())
    return;

  if (!base::PathExists(base::FilePath(widevine_cdm_path)))
    return;

  auto widevine_cdm_version = command_line->GetSwitchValueASCII(
    switches::kWidevineCdmVersion);
  if (widevine_cdm_version.empty())
    return;

  plugins->push_back(
      CreateWidevineCdmInfo(base::FilePath(widevine_cdm_path), 
                            widevine_cdm_version));
   
#endif  // defined(WIDEVINE_CDM_AVAILABLE) && defined(ENABLE_PEPPER_CDMS) &&
        // !defined(WIDEVINE_CDM_IS_COMPONENT)
}

}  // namespace

AtomContentClient::AtomContentClient() {
}

AtomContentClient::~AtomContentClient() {
}

std::string AtomContentClient::GetProduct() const {
  return "Chrome/" CHROME_VERSION_STRING;
}

void AtomContentClient::AddAdditionalSchemes(
    std::vector<std::string>* standard_schemes,
    std::vector<std::string>* savable_schemes) {
  auto command_line = base::CommandLine::ForCurrentProcess();
  auto custom_schemes = command_line->GetSwitchValueASCII(
      switches::kRegisterStandardSchemes);
  if (!custom_schemes.empty()) {
    std::vector<std::string> schemes;
    base::SplitString(custom_schemes, ',', &schemes);
    standard_schemes->insert(standard_schemes->end(),
                             schemes.begin(),
                             schemes.end());
  }
  standard_schemes->push_back("chrome-extension");
}

// #if defined(WIDEVINE_CDM_AVAILABLE) && defined(ENABLE_PEPPER_CDMS) && \
//     defined(WIDEVINE_CDM_IS_COMPONENT)

// component_updater::ComponentUpdateService* AtomContentClient::component_updater() {

//   if (!component_updater_.get()) {
   
//     scoped_refptr<update_client::Configurator> configurator =
//         component_updater::MakeChromeComponentUpdaterConfigurator(
//             base::CommandLine::ForCurrentProcess(),
//             CefContentBrowserClient::Get()->browser_context()->request_context().get());
//             web_contents()->GetBrowserContext()->GetRequestContext());
//             session->browser_context()->GetRequestContext());
                

//     // Creating the component updater does not do anything, components
//     // need to be registered and Start() needs to be called.
//     component_updater_.reset(component_updater::ComponentUpdateServiceFactory(
//                                  configurator).release());

//   }
//   return component_updater_.get();
// }
// void AtomContentClient::RegisterComponentsForUpdate() {
//   bool io_was_allowed = base::ThreadRestrictions::SetIOAllowed(true);

//   component_updater::ComponentUpdateService* cus =
//       component_updater();

//   // Registration can be before or after cus->Start() so it is ok to post
//   // a task to the UI thread to do registration once you done the necessary
//   // file IO to know you existing component version.
// #if !defined(OS_CHROMEOS) && !defined(OS_ANDROID)
//   if (base::CommandLine::ForCurrentProcess()->HasSwitch(
//       switches::kEnableWidevineCdm)) {
//     RegisterWidevineCdmComponent(cus);
//   }
// #endif  // !defined(OS_CHROMEOS) && !defined(OS_ANDROID)

//   base::ThreadRestrictions::SetIOAllowed(io_was_allowed);
// }
// #endif  // defined(WIDEVINE_CDM_AVAILABLE) && defined(ENABLE_PEPPER_CDMS) &&
//         // defined(WIDEVINE_CDM_IS_COMPONENT)

void AtomContentClient::AddPepperPlugins(
    std::vector<content::PepperPluginInfo>* plugins) {

 // not done yet 
 // RegisterComponentsForUpdate();

  AddPepperFlashFromCommandLine(plugins);
  AddWidevineCdmFromCommandLine(plugins);
  
}

}  // namespace atom
