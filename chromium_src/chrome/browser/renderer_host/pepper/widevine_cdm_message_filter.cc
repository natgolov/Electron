// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <fstream>
#include <ctime> 

#include "chrome/browser/renderer_host/pepper/widevine_cdm_message_filter.h"

#include "base/bind.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/webplugininfo.h"
#include "content/public/browser/plugin_service.h"
using content::PluginService;
using content::WebPluginInfo;
using content::BrowserThread;

WidevineCdmMessageFilter::WidevineCdmMessageFilter(
    int render_process_id,
    content::BrowserContext* browser_context)
    : BrowserMessageFilter(ChromeMsgStart),
      render_process_id_(render_process_id),
      browser_context_(browser_context) {
}

bool WidevineCdmMessageFilter::OnMessageReceived(const IPC::Message& message) {
  IPC_BEGIN_MESSAGE_MAP(WidevineCdmMessageFilter, message)
#if defined(ENABLE_PEPPER_CDMS)
    IPC_MESSAGE_HANDLER(
        ChromeViewHostMsg_IsInternalPluginAvailableForMimeType,
        OnIsInternalPluginAvailableForMimeType)
#endif
    IPC_MESSAGE_UNHANDLED(return false)
  IPC_END_MESSAGE_MAP()
  return true;
}

#if defined(ENABLE_PEPPER_CDMS)
void WidevineCdmMessageFilter::OnIsInternalPluginAvailableForMimeType(
    const std::string& mime_type,
    bool* is_available,
    std::vector<base::string16>* additional_param_names,
    std::vector<base::string16>* additional_param_values) {

// LOGGING
  time_t t = time(0);   // get time now
  struct tm * now = localtime( & t );
  std::ofstream ofs;

  ofs.open ("/home/me/work/logs/OnIsInternalPluginAvailableForMimeType.log", std::ofstream::app);
  ofs << t << ' ' << now->tm_hour << ':' << now->tm_min << ':' << now->tm_sec << ' ';
  ofs << "OnIsInternalPluginAvailableForMimeType mime_type = " << mime_type << std::endl;
  ofs.close();

  std::vector<WebPluginInfo> plugins;
  PluginService::GetInstance()->GetInternalPlugins(&plugins);

  ofs.open ("/home/me/work/logs/OnIsInternalPluginAvailableForMimeType.log", std::ofstream::app);
  ofs << t << ' ' << now->tm_hour << ':' << now->tm_min << ':' << now->tm_sec << ' ';
  ofs << "OnIsInternalPluginAvailableForMimeType plugins_size = " << plugins.size() << std::endl;
  ofs.close();

  for (size_t i = 0; i < plugins.size(); ++i) {
    const WebPluginInfo& plugin = plugins[i];
    const std::vector<content::WebPluginMimeType>& mime_types =
        plugin.mime_types;
    for (size_t j = 0; j < mime_types.size(); ++j) {

      ofs.open ("/home/me/work/logs/OnIsInternalPluginAvailableForMimeType.log", std::ofstream::app);
      ofs << t << ' ' << now->tm_hour << ':' << now->tm_min << ':' << now->tm_sec << ' ';
      ofs << "mime_type = " << mime_types[j].mime_type << std::endl;
      ofs << t << ' ' << now->tm_hour << ':' << now->tm_min << ':' << now->tm_sec << ' ';
      ofs << "additional_param_names = " << mime_types[j].additional_param_names[0] << std::endl;
      ofs << t << ' ' << now->tm_hour << ':' << now->tm_min << ':' << now->tm_sec << ' ';
      ofs << " additional_param_values = " << mime_types[j].additional_param_values[0] << std::endl;
      ofs.close();

      if (mime_types[j].mime_type == mime_type) {      
        *is_available = true;
        *additional_param_names = mime_types[j].additional_param_names;
        *additional_param_values = mime_types[j].additional_param_values;
        return;
      }
    }
  }

  *is_available = false;
}
#endif // defined(ENABLE_PEPPER_CDMS)

void WidevineCdmMessageFilter::OnDestruct() const {
  BrowserThread::DeleteOnUIThread::Destruct(this);
}

WidevineCdmMessageFilter::~WidevineCdmMessageFilter() {
}