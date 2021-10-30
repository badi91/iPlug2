#pragma once

/**
 * @file
 * @ingroup IControls
 * @copydoc IVChartEditorControl
 */

#include "IControl.h"
#include "ISender.h"
#include <sstream>
#include <set>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

struct ChartEditorPoint
{
public:
  ChartEditorPoint(double aX, double aY, bool isXRestricted = false) : x(aX), y(aY), isXRestricted(isXRestricted) {}
  double x;
  double y;
  //SmoothType smoothType;
  double smoothFactorL;
  double smoothFactorR;
  bool isXRestricted;/*

  bool operator==(const ChartEditorPoint& other) const {
    return this->x == other.x && this->y == other.y && this->isXRestricted == other.isXRestricted;
  }*/
};

struct ChartEditorPointComparer {
  bool operator()(const ChartEditorPoint* li, const ChartEditorPoint* ri) const {
    return li->x < ri->x;
  }
};

class IVChartEditorControl : public IControl
                       , public IVectorBase
{
public:
  static constexpr int MAX_BUFFER_SIZE = 2048;
  
  IVChartEditorControl(const IRECT& bounds, const char* label = "", const IVStyle& style = DEFAULT_STYLE, IColor color = COLOR_WHITE, int paramIndex = kNoParameter, bool snapToGrid = true, bool isEditable = true)
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

    //mPoints = std::set<ChartEditorPoint*, ChartEditorPointComparer>();
    mPoints.emplace(new ChartEditorPoint(0.0, 0.0, true));
    mPoints.emplace(new ChartEditorPoint(50.0, 50.0));
    mPoints.emplace(new ChartEditorPoint(100.0, 50.0, true));

    SetParamIdx(paramIndex);

    AttachIControl(this, label);
  }
  
  void OnMouseMove(float x, float y, float dx, float dy, const IMouseMod& mod) override
  {
      ChartEditorPoint* orgFocusedPoint = mFocusedPoint;
      bool isAnyFocused = false;
      float percentSensitivity = 5;
      float boundsX = mWidgetBounds.L;
      float boundsY = mWidgetBounds.T;
      float boundsW = mWidgetBounds.W();
      float boundsH = mWidgetBounds.H();

      for (auto it : mPoints) {
        if (std::abs(((x - boundsX) / boundsW) * 100.0 - it->x) < percentSensitivity
          && std::abs((100.0 - ((y - boundsY) / boundsH) * 100.0) - it->y) < percentSensitivity) {
          mFocusedPoint = it;
          isAnyFocused = true;
          break;
        }
      }

      if (!isAnyFocused)
        mFocusedPoint = nullptr;

      if (mFocusedPoint != orgFocusedPoint)
        SetDirty(false);
  }

  bool rightClickedDown = false;

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if (mod.R)
      rightClickedDown = true;

    float percentSensitivity = 5;
    float boundsX = mWidgetBounds.L;
    float boundsY = mWidgetBounds.T;
    float boundsW = mWidgetBounds.W();
    float boundsH = mWidgetBounds.H();

    for (auto it : mPoints) {
      if (std::abs(((x - boundsX) / boundsW) * 100.0 - it->x) < percentSensitivity
          && std::abs((100.0 - ((y - boundsY) / boundsH) * 100.0) - it->y) < percentSensitivity) {
        mIsPointBeingDragged = true;
        mEditedPoint = it;
        SetDirty(false);
        break;
      }
    }
  }

  void HandleRightClick(float x, float y) {
    ChartEditorPoint* pointToDelete = nullptr; //TODO: TEMP UNTIL NO TOOLTIP
    float percentSensitivity = 5;
    float boundsX = mWidgetBounds.L;
    float boundsY = mWidgetBounds.T;
    float boundsW = mWidgetBounds.W();
    float boundsH = mWidgetBounds.H();

    for (auto it : mPoints) {
      if (std::abs(((x - boundsX) / boundsW) * 100.0 - it->x) < percentSensitivity
        && std::abs((100.0 - ((y - boundsY) / boundsH) * 100.0) - it->y) < percentSensitivity) {
        pointToDelete = it;
        break;
      }
    }
    if (pointToDelete != nullptr && !pointToDelete->isXRestricted) {
      mPoints.erase(pointToDelete);
      SetDirty(false);
    }
  }

  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override {
    bool overAnyPoint = false;
    float percentSensitivity = 5;
    float boundsX = mWidgetBounds.L;
    float boundsY = mWidgetBounds.T;
    float boundsW = mWidgetBounds.W();
    float boundsH = mWidgetBounds.H();

    for (auto it : mPoints) {
      if (std::abs(((x - boundsX) / boundsW) * 100.0 - it->x) < percentSensitivity
        && std::abs((100.0 - ((y - boundsY) / boundsH) * 100.0) - it->y) < percentSensitivity) {
        overAnyPoint = true;
        break;
      }
    }

    if (overAnyPoint)
      return;


    double newX = (x - boundsX) / boundsW * 100.0;
    double newY = 100 - (y - boundsY) / boundsH * 100.0;

    if (mSnapToGrid) {
      newX = std::roundf(newX / 5) * 5.0;
      newY = std::roundf(newY / 5) * 5.0;
    }

    mPoints.emplace(new ChartEditorPoint(newX, newY));
    SetDirty(false);
  }

  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    //TODO: Tooltip dla punktu
    if (rightClickedDown) {
      HandleRightClick(x, y);
      rightClickedDown = false;
      return;
    }

    mIsPointBeingDragged = false;
    mEditedPoint = nullptr;
    SetDirty(false);
  }

  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
    if (mIsPointBeingDragged == false || mEditedPoint == nullptr || rightClickedDown)
      return;

    float boundsX = mWidgetBounds.L;
    float boundsY = mWidgetBounds.T;
    float boundsW = mWidgetBounds.W();
    float boundsH = mWidgetBounds.H();

    double newX = (x - boundsX) / boundsW * 100.0;
    double newY = 100 - (y - boundsY) / boundsH * 100.0;

    if (mSnapToGrid) {
      newX = std::roundf(newX / 5) * 5.0;
      newY = std::roundf(newY / 5) * 5.0;
    }

    auto prevPoint = std::prev(mPoints.find(mEditedPoint));
    auto nextPoint = std::next(mPoints.find(mEditedPoint));
    if (!mEditedPoint->isXRestricted)
      mEditedPoint->x = std::clamp(newX, (prevPoint != mPoints.end() ? (*prevPoint)->x + 0.01 : 0.0), (nextPoint != mPoints.end() ? (*nextPoint)->x - 0.01 : 100.0));
    mEditedPoint->y = std::clamp(newY, 0.0, 100.0);

    SetDirty(false);
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

    auto pathLinePoints = [&]() {
      for (auto it : mPoints) {
        if (it == (*mPoints.begin()))
          continue;
        double vx = x + it->x / 100.0 * w;
        double vy = y + h - it->y / 100.0 * h;
        g.PathLineTo(vx, vy);
      }
    };


    //Wykres liniowy
    double vx0 = x + (*mPoints.begin())->x / 100.0 * w;
    double vy0 = y + (h - (*mPoints.begin())->y / 100.0 * h);
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
    if (mFocusedPoint != nullptr) {
      double vx = x + mFocusedPoint->x / 100.0 * w;
      double vy = y + h - mFocusedPoint->y / 100.0 * h;
      g.DrawCircle(mColor, vx, vy, 5.0f);
    }
    for (auto it : mPoints) {
      double vx = x + it->x / 100.0 * w;
      double vy = y + h - it->y / 100.0 * h;
      g.DrawCircle(COLOR_BLACK, vx, vy, 2.5f);
      g.FillCircle(IColor::LinearInterpolateBetween(mColor, COLOR_WHITE, 0.5f), vx, vy, mEditedPoint == it ? 4.0f : 2.5f);
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


    //TODO: DELETE
    //Pisanie indeksów punktów
    for (auto it : mPoints) {
      double vx = x + it->x / 100.0 * w;
      double vy = y + h - it->y / 100.0 * h;
      int distance = std::distance(mPoints.begin(), mPoints.find(it));
      g.DrawText(mStyle.valueText.WithSize(16.0f).WithFGColor(COLOR_WHITE), std::to_string(distance).c_str(), IRECT(vx - 10, vy, vx + 10, vy - 20));
    }
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

  void SetPlotBound(IRECT bounds) {
    mWidgetBounds = bounds;
    SetDirty(false);
  }

  IColor GetColor() {
    return mColor;
  }

  const IColor& GetColor(EVColor color) const
  {
    return mStyle.colorSpec.GetColor(color);
  }

  //TODO: jakie info wystawiać? dane na podstawie których tworzone są punkty, czy to po prostu sama tablica punktów czy raczej jakieś współczynniki?
  
private:
  IColor mColor;
  std::set<ChartEditorPoint*, ChartEditorPointComparer> mPoints;
  bool mSnapToGrid;
  bool mIsEditable;
  ChartEditorPoint* mEditedPoint = nullptr;
  ChartEditorPoint* mFocusedPoint = nullptr;
  bool mIsPointBeingDragged = false;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
