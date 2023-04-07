#pragma once

#include "IControl.h"
#include "ISender.h"
#include <iostream>
#include <vector>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

class ITab
{
public:
  ITab(IVChartEditorControl* control, std::string name) : control(control), name(name)
  {
    subControls = *new std::vector<IControl*>();
  };
  IVChartEditorControl* control;
  std::vector<IControl*> subControls;
  std::string name;
  IRECT tabSelectorRect;
  //bool isActive = false;
  void AttachIControl(IControl* control) {
    subControls.push_back(control);
  }
};


class ITabbedPanel : public IControl, public IVectorBase
{
public:  
  ITabbedPanel(const IRECT& bounds, const char* label, const IVStyle& style, std::vector<ITab*> tabs)
    : IControl(bounds)
    , IVectorBase(style)
    , mTabs(tabs)
  {
    //for (auto i : tabs) {
    //  AttachIControl(i->control, "DUPA");
    //}
    AttachIControl(this, label);
  }

  void OnAttached() override
  {
    for (auto i : mTabs) {
      for (auto j : i->subControls) {
        GetUI()->AttachControl(j);
      }
    }
  }

  void OnResize() override
  {
    SetTargetRECT(MakeRects(mRECT));

    for (int i = 0; i < mTabs.size(); i++) {
      auto tab = mTabs[i];
      tab->control->SetPlotBound(mWidgetBounds.GetFromTLHC(270, 270).GetPadded(-5.0f));
      auto wholeSelectorRect = mWidgetBounds.GetFromTLHC(105, 150).GetVShifted(300);
      double targetH = wholeSelectorRect.H() / mTabs.size();
      tab->tabSelectorRect = wholeSelectorRect.GetFromTop(targetH).GetVShifted(targetH * i);
    }

    //freezeButtonRect = mWidgetBounds.GetFromBRHC(10.0f, 10.0f).GetHShifted(-10.0).GetVShifted(-10.0f);
    //magnetButtonRect = mWidgetBounds.GetFromBRHC(10.0f, 10.0f).GetHShifted(-30.0).GetVShifted(-10.0f);
    
    SetDirty(false);
  }
  
  void Draw(IGraphics& g) override
  {
    DrawBackground(g, mRECT);
    DrawWidget(g);
    DrawLabel(g);
    
    if(mStyle.drawFrame)
      g.DrawRect(GetColor(kFR), mWidgetBounds, &mBlend, mStyle.frameThickness);

    for (int i = 0; i < mTabs.size(); i++) {
      auto tab = mTabs[i];
      auto textStyle = mStyle.valueText.WithSize(16.0f).WithFGColor((i == activeTabIndex ? COLOR_LIGHT_GRAY : COLOR_DARK_GRAY)).WithAlign(EAlign::Far).WithVAlign(EVAlign::Middle);
      g.FillRect((i == activeTabIndex ? COLOR_GRAY : COLOR_MID_GRAY), tab->tabSelectorRect);
      g.DrawText(textStyle, tab->name.c_str(), tab->tabSelectorRect.GetFromRight(tab->tabSelectorRect.W() - 5.0f).GetPadded(-2.0f));
      g.FillRect(tab->control->GetColor(), tab->tabSelectorRect.GetFromLeft(5.0f));
    }

    ////Przycisk freeze
    //g.DrawCircle(COLOR_DARK_GRAY, freezeButtonRect.MW(), freezeButtonRect.MH(), 5.0f);
    ////if (!getActiveControl()->getIsEditable())
    //  g.FillCircle(COLOR_BLUE, freezeButtonRect.MW(), freezeButtonRect.MH(), 4.0f);

    ////Przycisk magnet
    //g.DrawCircle(COLOR_DARK_GRAY, magnetButtonRect.MW(), magnetButtonRect.MH(), 5.0f);
    ////if (getActiveControl()->getSnapToGrid())
    //  g.FillCircle(COLOR_RED, magnetButtonRect.MW(), magnetButtonRect.MH(), 4.0f);
  }

  ITab* getActiveTab() { return mTabs[activeTabIndex]; }

  void DrawWidget(IGraphics& g) override
  {
    getActiveTab()->control->Draw(g);
    //for (auto i : mTabs[activeTabIndex]->subControls)
    //  i->Draw(g);
  }

  void OnMouseMove(float x, float y, float dx, float dy, const IMouseMod& mod) override
  {
    if (getActiveTab()->control->getPlotBound().Contains(x, y))
      getActiveTab()->control->OnMouseMove(x, y, dx, dy, mod);

    //for (auto i : getActiveTab()->subControls) {
    //  if (i->GetRECT().Contains(x, y))
    //    i->OnMouseMove(x, y, dx, dy, mod);
    //}
    SetDirty(false);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    auto activeControl = getActiveTab()->control;
    //if (freezeButtonRect.Contains(x, y))
    //  activeControl->setIsEditable(!activeControl->getIsEditable());
    //if (magnetButtonRect.Contains(x, y))
    //  activeControl->setSnapToGrid(!activeControl->getSnapToGrid());

    auto wholeSelectorRect = mWidgetBounds.GetFromTLHC(105, 150).GetVShifted(300);

    int i = 0;
    for (auto tab : mTabs) {
      if (tab->tabSelectorRect.Contains(x, y)) {
        activeTabIndex = i;
        for (auto control : tab->subControls) {
          control->Hide(false);
        }
      }
      else if (wholeSelectorRect.Contains(x, y)) {
        for (auto control : tab->subControls) {
          control->Hide(true);
        }
      }
      ++i;
    }
    if (activeControl->getPlotBound().Contains(x, y))
      activeControl->OnMouseDown(x, y, mod);

    //for (auto i : getActiveTab()->subControls) {
    //  if (i->GetRECT().Contains(x, y))
    //    i->OnMouseDown(x, y, mod);
    //}

    SetDirty(false);
  }

  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    if (getActiveTab()->control->getPlotBound().Contains(x, y))
      getActiveTab()->control->OnMouseUp(x, y, mod);

    //for (auto i : getActiveTab()->subControls) {
    //  if (i->GetRECT().Contains(x, y))
    //    i->OnMouseUp(x, y, mod);
    //}

    SetDirty(false);
  }

  void OnMouseDrag(float x, float y, float dx, float dy, const IMouseMod& mod) override
  {
    if (getActiveTab()->control->getPlotBound().Contains(x, y))
      getActiveTab()->control->OnMouseDrag(x, y, dx, dy, mod);

    //for (auto i : getActiveTab()->subControls) {
    //  if (i->GetRECT().Contains(x, y))
    //    i->OnMouseDrag(x, y, dx, dy, mod);
    //}

    SetDirty(false);
  }

  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override
  {
    if (getActiveTab()->control->getPlotBound().Contains(x, y))
      getActiveTab()->control->OnMouseDblClick(x, y, mod);

    //for (auto i : getActiveTab()->subControls) {
    //  if (i->GetRECT().Contains(x, y))
    //    i->OnMouseDblClick(x, y, mod);
    //}

    SetDirty(false);
  }
  
private:
  std::vector<ITab*> mTabs;
  int activeTabIndex = 2;
  //IRECT freezeButtonRect;
  //IRECT magnetButtonRect;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
