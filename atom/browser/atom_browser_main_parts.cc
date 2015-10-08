// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.
#include <fstream>
#include <ctime>

#include "atom/browser/atom_browser_main_parts.h"

#include "atom/browser/api/trackable_object.h"
#include "atom/browser/atom_browser_client.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/bridge_task_runner.h"
#include "atom/browser/browser.h"
#include "atom/browser/javascript_environment.h"
#include "atom/browser/node_debugger.h"
#include "atom/common/api/atom_bindings.h"
#include "atom/common/node_bindings.h"
#include "atom/common/node_includes.h"
#include "atom/common/options_switches.h"
#include "base/command_line.h"
#include "base/thread_task_runner_handle.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/component_updater/chrome_component_updater_configurator.h"
#include "chrome/browser/component_updater/widevine_cdm_component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "components/update_client/configurator.h"
#include "v8/include/v8-debug.h"

#if defined(USE_X11)
#include "chrome/browser/ui/libgtk2ui/gtk2_util.h"
#endif

namespace atom {

// static
AtomBrowserMainParts* AtomBrowserMainParts::self_ = NULL;

AtomBrowserMainParts::AtomBrowserMainParts()
    : fake_browser_process_(new BrowserProcess),
      browser_(new Browser),
      node_bindings_(NodeBindings::Create(true)),
      atom_bindings_(new AtomBindings),
      gc_timer_(true, true) {
  DCHECK(!self_) << "Cannot have two AtomBrowserMainParts";
  self_ = this;
}

AtomBrowserMainParts::~AtomBrowserMainParts() {
  for (const auto& callback : destruction_callbacks_)
    callback.Run();
}

// static
AtomBrowserMainParts* AtomBrowserMainParts::Get() {
  DCHECK(self_);
  return self_;
}

void AtomBrowserMainParts::RegisterDestructionCallback(
    const base::Closure& callback) {
  destruction_callbacks_.push_back(callback);
}

void AtomBrowserMainParts::PostEarlyInitialization() {
  brightray::BrowserMainParts::PostEarlyInitialization();

#if defined(USE_X11)
  SetDPIFromGSettings();
#endif

  {
    // Temporary set the bridge_task_runner_ as current thread's task runner,
    // so we can fool gin::PerIsolateData to use it as its task runner, instead
    // of getting current message loop's task runner, which is null for now.
    bridge_task_runner_ = new BridgeTaskRunner;
    base::ThreadTaskRunnerHandle handle(bridge_task_runner_);

    // The ProxyResolverV8 has setup a complete V8 environment, in order to
    // avoid conflicts we only initialize our V8 environment after that.
    js_env_.reset(new JavascriptEnvironment);
  }

  node_bindings_->Initialize();

  // Support the "--debug" switch.
  node_debugger_.reset(new NodeDebugger(js_env_->isolate()));

  // Create the global environment.
  global_env = node_bindings_->CreateEnvironment(js_env_->context());

  // Make sure node can get correct environment when debugging.
  if (node_debugger_->IsRunning())
    global_env->AssignToContext(v8::Debug::GetDebugContext());

  // Add atom-shell extended APIs.
  atom_bindings_->BindTo(js_env_->isolate(), global_env->process_object());

  // Load everything.
  node_bindings_->LoadEnvironment(global_env);
}

component_updater::ComponentUpdateService* AtomBrowserMainParts::component_updater() {

  if (!component_updater_.get()) {
   
    scoped_refptr<update_client::Configurator> configurator =
        component_updater::MakeChromeComponentUpdaterConfigurator(
            base::CommandLine::ForCurrentProcess(),
            browser_context()->url_request_context_getter());
  
    // Creating the component updater does not do anything, components
    // need to be registered and Start() needs to be called.
    component_updater_.reset(component_updater::ComponentUpdateServiceFactory(
                                 configurator).release());
  }
  return component_updater_.get();
}

void AtomBrowserMainParts::RegisterComponentsForUpdate() {
  bool io_was_allowed = base::ThreadRestrictions::SetIOAllowed(true);

  component_updater::ComponentUpdateService* cus =
      component_updater();

  // Registration can be before or after cus->Start() so it is ok to post
  // a task to the UI thread to do registration once you done the necessary
  // file IO to know you existing component version.
#if !defined(OS_CHROMEOS) && !defined(OS_ANDROID)
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kEnableWidevineCdm)) {
    RegisterWidevineCdmComponent(cus);
  }
#endif  // !defined(OS_CHROMEOS) && !defined(OS_ANDROID)

  base::ThreadRestrictions::SetIOAllowed(io_was_allowed);
}


void AtomBrowserMainParts::PreMainMessageLoopRun() {
  // Run user's main script before most things get initialized, so we can have
  // a chance to setup everything.
  node_bindings_->PrepareMessageLoop();
  node_bindings_->RunMessageLoop();

  // Start idle gc.
  gc_timer_.Start(
      FROM_HERE, base::TimeDelta::FromMinutes(1),
      base::Bind(base::IgnoreResult(&v8::Isolate::IdleNotification),
                 base::Unretained(js_env_->isolate()),
                 1000));

  brightray::BrowserMainParts::PreMainMessageLoopRun();

#if defined(USE_X11)
  libgtk2ui::GtkInitFromCommandLine(*base::CommandLine::ForCurrentProcess());
#endif

#if !defined(OS_MACOSX)
  // The corresponding call in OS X is in AtomApplicationDelegate.
  Browser::Get()->WillFinishLaunching();
  Browser::Get()->DidFinishLaunching();
#endif

  RegisterComponentsForUpdate();

  time_t t = time(0);   // get time now
  struct tm * now = localtime( & t );
  std::ofstream ofs;

  ofs.open ("../atom_browser_main_parts.log", std::ofstream::app);
  ofs << t << ' ' << now->tm_hour << ':' << now->tm_min << ':' << now->tm_sec << ' ';
  ofs << "PreMainMessageLoopRun done RegisterComponentsForUpdate " << std::endl;
  ofs.close();

}

}  // namespace atom
