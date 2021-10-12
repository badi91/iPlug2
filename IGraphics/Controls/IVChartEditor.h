#pragma once

/**
 * @file
 * @ingroup IControls
 * @copydoc IVChartEditorControl
 */

#include "IControl.h"
#include "ISender.h"
#include <sstream>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

struct ChartEditorPoint
{
public:
  ChartEditorPoint(double aX, double aY) : x(aX), y(aY) {}
  double x;
  double y;
  //SmoothType smoothType;
  double smoothFactorL;
  double smoothFactorR;
};

class IVChartEditorControl : public IControl
                       , public IVectorBase
{
public:
  static constexpr int MAX_BUFFER_SIZE = 2048;
  
  IVChartEditorControl(const IRECT& bounds, const char* label = "", const IVStyle& style = DEFAULT_STYLE, IColor color = COLOR_WHITE, IGraphics* graphics = NULL, bool snapToGrid = true, bool isEditable = true)
  : IControl(bounds)
  , IVectorBase(style)
  , mColor(color)
  , mSnapToGrid(snapToGrid)
  , mIsEditable(isEditable)
  {
    float boundsX = bounds.L;
    float boundsY = bounds.T;
    float boundsW = bounds.W();
    float boundsH = bounds.H();

    mPoints = std::vector<ChartEditorPoint*>
    {
      new ChartEditorPoint(0.0, 0.0),
      new ChartEditorPoint(50.0, 50.0),
      new ChartEditorPoint(100.0, 50.0)
    };

    AttachIControl(this, label);
  }
  
  void OnMouseMove(float x, float y, float dx, float dy, const IMouseMod& mod) override
  {
      int orgFocusedPointIndex = mFocusedPointIndex;
      bool isAnyFocused = false;
      float percentSensitivity = 5;
      float boundsX = mPlotBounds.L;
      float boundsY = mPlotBounds.T;
      float boundsW = mPlotBounds.W();
      float boundsH = mPlotBounds.H();

      for (int i = 0; i < mPoints.size(); i++) {
        if (std::abs(((x - boundsX) / boundsW) * 100.0 - mPoints[i]->x) < percentSensitivity
          && std::abs((100.0 - ((y - boundsY) / boundsH) * 100.0) - mPoints[i]->y) < percentSensitivity) {
          mFocusedPointIndex = i;
          isAnyFocused = true;
          break;
        }
      }

      if (!isAnyFocused)
        mFocusedPointIndex = -1;

      if (mFocusedPointIndex != orgFocusedPointIndex)
        SetDirty(false);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    //TODO: Reset do org. pozycji na R CLICK
    if (mod.R)
      return;
    
    float percentSensitivity = 5;
    float boundsX = mPlotBounds.L;
    float boundsY = mPlotBounds.T;
    float boundsW = mPlotBounds.W();
    float boundsH = mPlotBounds.H();

    for (int i = 0; i < mPoints.size(); i++) {
      if (std::abs(((x - boundsX) / boundsW) * 100.0 - mPoints[i]->x) < percentSensitivity
          && std::abs((100.0 - ((y - boundsY) / boundsH) * 100.0) - mPoints[i]->y) < percentSensitivity) {
        mIsPointBeingDragged = true;
        mEditedPointIndex = i;
        SetDirty(false);
        break;
      }
    }
  }

  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    mIsPointBeingDragged = false;
    mEditedPointIndex = -1;
    SetDirty(false);
  }

  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
    if (mIsPointBeingDragged == false || mEditedPointIndex < 0)
      return;

    float boundsX = mPlotBounds.L;
    float boundsY = mPlotBounds.T;
    float boundsW = mPlotBounds.W();
    float boundsH = mPlotBounds.H();

    double newX = (x - boundsX) / boundsW * 100.0;
    double newY = 100 - (y - boundsY) / boundsH * 100.0;

    if (mEditedPointIndex == 0)
      newX = 0.0;
    if (mEditedPointIndex == mPoints.size() - 1)
      newX = 100.0;

    if (mSnapToGrid) {
      newX = std::roundf(newX / 5) * 5.0;
      newY = std::roundf(newY / 5) * 5.0;
    }

    mPoints[mEditedPointIndex]->x = std::clamp(newX, 0.0, 100.0);
    mPoints[mEditedPointIndex]->y = std::clamp(newY, 0.0, 100.0);

    SetDirty(false);
  }

  void OnResize() override
  {
    SetTargetRECT(MakeRects(mRECT));
    
    mPlotBounds = mWidgetBounds;// .GetPadded(2.0f, 2.0f, 2.0f, 2.0f);
    
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
    float x = mPlotBounds.L;
    float y = mPlotBounds.T;
    float w = mPlotBounds.W();
    float h = mPlotBounds.H();

    auto pathLinePoints = [&]() {
      for (int i = 1; i < mPoints.size(); i++) {
        double vx = x + mPoints[i]->x / 100.0 * w;
        double vy = y + h - mPoints[i]->y / 100.0 * h;
        g.PathLineTo(vx, vy);
      }
    };


    //Wykres liniowy
    double vx0 = x + mPoints[0]->x / 100.0 * w;
    double vy0 = y + (h - mPoints[0]->y / 100.0 * h);
    g.PathMoveTo(vx0, vy0);
    pathLinePoints();
    g.PathStroke(IPattern::IPattern(mColor), 2.0f);


    //Wypełnienie pod wykresem
    g.PathMoveTo(x + w, y + h);
    g.PathLineTo(x, y + h);
    g.PathLineTo(vx0, vy0);
    pathLinePoints();
    g.PathFill(IPattern(mColor.WithOpacity(0.15f)));
    

    //Rysowanie samych punktów
    if (mFocusedPointIndex > -1) {
      double vx = x + mPoints[mFocusedPointIndex]->x / 100.0 * w;
      double vy = y + h - mPoints[mFocusedPointIndex]->y / 100.0 * h;
      g.DrawCircle(mColor, vx, vy, 5.0f);
    }
    for (int i = 0; i < mPoints.size(); i++) {
      double vx = x + mPoints[i]->x / 100.0 * w;
      double vy = y + h - mPoints[i]->y / 100.0 * h;
      g.FillCircle(mColor, vx, vy, mEditedPointIndex == i ? 5.0f : 3.0f);
    }


    //Linie odniesienia w tle
    g.DrawLine(COLOR_WHITE, x, y + h, x + w, y, &BLEND_10);
    g.DrawLine(COLOR_WHITE, x, y + h / 2.0f, x + w, y + h / 2.0f, &BLEND_10);
    g.DrawLine(COLOR_WHITE, x + w / 2.0f, y, x + w / 2.0f, y + h, &BLEND_10);

    auto scale = 3 * 0.5 / 27 * w; 
    g.DrawLine(COLOR_WHITE, x + 2 * scale, y - 1, x + 2 * scale, y + 2, &BLEND_75);
    g.DrawLine(COLOR_WHITE, x + 4 * scale, y - 1, x + 4 * scale, y + 2, &BLEND_75);
    g.DrawLine(COLOR_WHITE, x + 5 * scale, y - 1, x + 5 * scale, y + 2, &BLEND_75);
    g.DrawLine(COLOR_WHITE, x + 6 * scale, y - 1, x + 6 * scale, y + 2, &BLEND_75);
    g.DrawLine(COLOR_WHITE, x + 7 * scale, y - 1, x + 7 * scale, y + 2, &BLEND_75);

    g.DrawLine(COLOR_WHITE, x + w / 2 + 2 * scale, y - 1, x + w / 2 + 2 * scale, y + 2, &BLEND_75);
    g.DrawLine(COLOR_WHITE, x + w / 2 + 4 * scale, y - 1, x + w / 2 + 4 * scale, y + 2, &BLEND_75);
    g.DrawLine(COLOR_WHITE, x + w / 2 + 6 * scale, y - 1, x + w / 2 + 6 * scale, y + 2, &BLEND_75);

    g.DrawText(mStyle.valueText.WithSize(12.0f).WithFGColor(COLOR_WHITE), "-18", IRECT(x + scale, y, x + 3 * scale, y + 15));
    g.DrawText(mStyle.valueText.WithSize(12.0f).WithFGColor(COLOR_WHITE), "-12", IRECT(x + 3 * scale, y, x + 5 * scale, y + 15));
    g.DrawText(mStyle.valueText.WithSize(12.0f).WithFGColor(COLOR_WHITE), "-9", IRECT(x + 4 * scale, y, x + 6 * scale, y + 15));
    g.DrawText(mStyle.valueText.WithSize(12.0f).WithFGColor(COLOR_WHITE), "-6", IRECT(x + 5 * scale, y, x + 7 * scale, y + 15));
    g.DrawText(mStyle.valueText.WithSize(12.0f).WithFGColor(COLOR_WHITE), "-3", IRECT(x + 6 * scale, y, x + 8 * scale, y + 15));
  }
  
  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    //auto Update = [&](float v) {
    //  mBuffer[mReadPos] = v;
    //  mReadPos = (mReadPos+1) % mBuffer.size();
    //  SetDirty(false);
    //};

    if (!IsDisabled() && msgTag == ISender<>::kUpdateMessage)
    {
      IByteStream stream(pData, dataSize);

      int pos = 0;
      ISenderData<1> d;
      pos = stream.Get(&d, pos);
     // Update(d.vals[0]);
    }
  }
  
private:
  IRECT mPlotBounds;
  IColor mColor;
  std::vector<ChartEditorPoint*> mPoints;
  bool mSnapToGrid;
  bool mIsEditable;
  int mEditedPointIndex = -1;
  bool mIsPointBeingDragged = false;
  int mFocusedPointIndex = -1;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
