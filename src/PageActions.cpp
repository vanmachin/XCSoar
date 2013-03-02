/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "PageActions.hpp"
#include "UIActions.hpp"
#include "UIState.hpp"
#include "Interface.hpp"
#include "ActionInterface.hpp"
#include "MainWindow.hpp"
#include "CrossSection/CrossSectionWidget.hpp"
#include "InfoBoxes/InfoBoxSettings.hpp"

namespace PageActions {
  /**
   * Loads the layout without updating current page information in
   * #UIState.
   */
  void LoadLayout(const PageLayout &layout);
};

static const PageLayout &
GetConfiguredLayout()
{
  const PageSettings &settings = CommonInterface::GetUISettings().pages;
  const UIState &state = CommonInterface::SetUIState();

  return settings.pages[state.page_index];
}

const PageLayout &
PageActions::GetCurrentLayout()
{
  const UIState &state = CommonInterface::SetUIState();

  return state.special_page.IsDefined()
    ? state.special_page
    : GetConfiguredLayout();
}

void
PageActions::Update()
{
  LoadLayout(GetCurrentLayout());
}


unsigned
PageActions::NextIndex()
{
  const PageSettings &settings = CommonInterface::GetUISettings().pages;
  const UIState &ui_state = CommonInterface::SetUIState();

  if (ui_state.special_page.IsDefined())
    /* if a "special" page is active, any page switch will return to
       the last configured page */
    return ui_state.page_index;

  return (ui_state.page_index + 1) % settings.n_pages;
}


void
PageActions::Next()
{
  UIState &ui_state = CommonInterface::SetUIState();

  ui_state.page_index = NextIndex();
  ui_state.special_page.SetUndefined();

  Update();
}

unsigned
PageActions::PrevIndex()
{
  const PageSettings &settings = CommonInterface::GetUISettings().pages;
  const UIState &ui_state = CommonInterface::SetUIState();

  if (ui_state.special_page.IsDefined())
    /* if a "special" page is active, any page switch will return to
       the last configured page */
    return ui_state.page_index;

  return (ui_state.page_index + PageSettings::MAX_PAGES - 1)
    % settings.n_pages;
}


void
PageActions::Prev()
{
  UIState &ui_state = CommonInterface::SetUIState();

  ui_state.page_index = PrevIndex();
  ui_state.special_page.SetUndefined();

  Update();
}

void
PageActions::LoadLayout(const PageLayout &layout)
{
  UIState &ui_state = CommonInterface::SetUIState();

  if (!layout.valid)
    return;

  if (!layout.infobox_config.enabled) {
    CommonInterface::main_window->SetFullScreen(true);
    ui_state.auxiliary_enabled = false;
  } else {
    if (!layout.infobox_config.auto_switch &&
        layout.infobox_config.panel < InfoBoxSettings::MAX_PANELS) {
      CommonInterface::main_window->SetFullScreen(false);
      ui_state.auxiliary_enabled = true;
      ui_state.auxiliary_index = layout.infobox_config.panel;
    }
    else {
      CommonInterface::main_window->SetFullScreen(false);
      ui_state.auxiliary_enabled = false;
    }
  }

  switch (layout.bottom) {
  case PageLayout::Bottom::NOTHING:
    CommonInterface::main_window->SetBottomWidget(nullptr);
    break;

  case PageLayout::Bottom::CROSS_SECTION:
    CommonInterface::main_window->SetBottomWidget(new CrossSectionWidget());
    break;

  case PageLayout::Bottom::MAX:
    gcc_unreachable();
  }

  switch (layout.main) {
  case PageLayout::Main::MAP:
    CommonInterface::main_window->ActivateMap();
    break;

  case PageLayout::Main::FLARM_RADAR:
    UIActions::ShowTrafficRadar();
    break;

  case PageLayout::Main::THERMAL_ASSISTANT:
    UIActions::ShowThermalAssistant();
    break;

  case PageLayout::Main::MAX:
    gcc_unreachable();
  }

  ActionInterface::UpdateDisplayMode();
  ActionInterface::SendUIState();
}

void
PageActions::OpenLayout(const PageLayout &layout)
{
  UIState &ui_state = CommonInterface::SetUIState();
  ui_state.special_page = layout;

  LoadLayout(layout);
}

void
PageActions::Restore()
{
  PageLayout &special_page = CommonInterface::SetUIState().special_page;
  if (!special_page.IsDefined())
    return;

  special_page.SetUndefined();

  LoadLayout(GetConfiguredLayout());
}

void
PageActions::DeferredRestore()
{
  CommonInterface::main_window->DeferredRestorePage();
}

GlueMapWindow *
PageActions::ShowMap()
{
  PageLayout layout = GetCurrentLayout();
  if (layout.main != PageLayout::Main::MAP) {
    /* not showing map currently: activate it */

    if (GetConfiguredLayout().main == PageLayout::Main::MAP)
      /* the configured page is a map page: restore it */
      Restore();
    else {
      /* generate a "special" map page based on the current page */
      layout.main = PageLayout::Main::MAP;
      OpenLayout(layout);
    }
  }

  return CommonInterface::main_window->ActivateMap();
}

GlueMapWindow *
PageActions::ShowOnlyMap()
{
  OpenLayout(PageLayout::FullScreen());
  return CommonInterface::main_window->ActivateMap();
}
