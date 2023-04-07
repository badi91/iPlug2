#pragma once

#include "IControl.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

class IControlPropsEditor : public IControl, public IVectorBase
{
public:
  static constexpr int MAX_BUFFER_SIZE = 2048;
  
  IControlPropsEditor(const IRECT& bounds, std::unordered_map<std::string, std::pair<bool, std::string>>*& controlProperties, const char* label = "", const IVStyle& style = DEFAULT_STYLE)
  : IControl(bounds)
  , IVectorBase(style)
  , controlProperties(controlProperties)
  {
    float boundsX = bounds.L;
    float boundsY = bounds.T;
    float boundsW = bounds.W();
    float boundsH = bounds.H();

    AttachIControl(this, label);
    SetDrawFrame(false);
  }

  void OnMouseMove(float x, float y, float dx, float dy, const IMouseMod& mod) override
  {
    float check_x = mWidgetBounds.L;
    float check_y = mWidgetBounds.T + mWidgetBounds.H() / 2.0f;
    mFocusedProp = "";
    for (auto prop : *controlProperties) {
      check_x += 25.0f; //TODO: zaawansowane obliczenia xD
      if (x >= check_x - 10.0f && x <= check_x + 10.0f && y >= check_y - 10.0f && y <= check_y + 10.0f) {
        mFocusedProp = prop.first;
        break;
      } 
    }
    SetDirty(false);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
  }

  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override
  {
  }

  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    //if (!mod.L || mod.R)
    //  return;
    DBGMSG("GetPropsMouseUp: %p", *controlProperties);

    float check_x = mWidgetBounds.L;
    float check_y = mWidgetBounds.T + mWidgetBounds.H() / 2.0f;
    for (auto &prop : *controlProperties) {
      check_x += 25.0f; //TODO: zaawansowane obliczenia xD
      if (x >= check_x - 10.0f && x <= check_x + 10.0f && y >= check_y - 10.0f && y <= check_y + 10.0f) {
        auto result = controlProperties->find(prop.first);
        if (result != controlProperties->end())
          (*result).second = std::pair(!prop.second.first, prop.second.second);
        break;
      }
    }
    SetDirty(true);
    GetUI()->SetAllControlsDirty();
  }

  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
  }

  void OnResize() override
  {
    SetTargetRECT(MakeRects(mRECT));
    
    SetDirty(false);
  }
  
  void Draw(IGraphics& g) override
  {
    DrawBackground(g, mRECT);
    DrawWidget(g);
    DrawLabel(g);
    
    if(mStyle.drawFrame)
      g.DrawRect(GetColor(kFR), mWidgetBounds, &mBlend, mStyle.frameThickness);
  }

  void DrawWidget(IGraphics& g) override
  {
    float x = mWidgetBounds.L;
    float y = mWidgetBounds.T;
    float w = mWidgetBounds.W();
    float h = mWidgetBounds.H();

    for (auto &prop : (*controlProperties)) {
      x += 25.0f; //TODO: zaawansowane obliczenia xD
      IBitmap magnetBitmap = prop.first == mFocusedProp
        ? g.LoadBitmap(std::string(prop.second.second + "_focused.png").c_str(), 1)
        : g.LoadBitmap(std::string(prop.second.second + ".png").c_str(), 1);
      if (prop.second.first)
        magnetBitmap = g.LoadBitmap(std::string(prop.second.second + "_selected.png").c_str(), 1);

      g.DrawFittedBitmap(magnetBitmap, IRECT(x - 10.0f, y + 5.0f, x + 10.0f, y + 25.0f));
      //g.DrawCircle((prop.first == mFocusedProp ? COLOR_LIGHT_GRAY : COLOR_DARK_GRAY), x, y + h / 2.0f, 5.0f);
      //if (prop.second)
      //  g.FillCircle(COLOR_GREEN, x, y + h / 2.0f, 4.0f);
    }
  }
  
private:
  std::unordered_map<std::string, std::pair<bool, std::string>>* controlProperties;
  std::string mFocusedProp;
  //TODO: Layout (horizontal/vertical) -> auto assume from bounds ratio?
  //TODO: kolory?
};


END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
