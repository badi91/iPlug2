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
  ChartEditorPoint(double aX, double aY, bool isXRestricted = false, double interpolationFactor = 0.5) :
    x(aX), y(aY), isXRestricted(isXRestricted), interpolationFactor(interpolationFactor) {}
  double x;
  double y;
  //SmoothType smoothType;
  //double smoothFactorL;
  //double smoothFactorR;
  double interpolationFactor; // -1 -> left to min (this point) +1 -> right to max (next point)
  bool isXRestricted;
  IVec2 midPoint;
};

struct ChartEditorPointComparer {
  bool operator()(const ChartEditorPoint* li, const ChartEditorPoint* ri) const {
    return li->x < ri->x;
  }
};

static const std::string Property_SnapToGrid = "Snap to Grid";
static const std::string Property_Freeze = "Freeze";

class IVChartEditorControl : public IControl, public IVectorBase, public IHasProperties
{
public:
  static constexpr int MAX_BUFFER_SIZE = 2048;
  
  IVChartEditorControl(const IRECT& bounds, const char* label = "", const IVStyle& style = DEFAULT_STYLE, IColor color = COLOR_WHITE, int paramIndex = kNoParameter, bool snapToGrid = true,
    bool isEditable = true, std::unordered_map<std::string, std::pair<bool, std::string>>* properties = nullptr)
  : IControl(bounds)
  , IVectorBase(style)
  , IHasProperties(properties != nullptr ? properties : new std::unordered_map<std::string, std::pair<bool, std::string>> {
    { Property_Freeze, std::pair<bool, std::string>(false, "freeze") },
    { Property_SnapToGrid, std::pair<bool, std::string>(true, "magnet") }
  })
  , mColor(color)
  //, mSnapToGrid(snapToGrid)
  //, mIsEditable(isEditable)
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
    if (GetPropertyValue(Property_Freeze).first)
      return;
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
    mIsPointBeingDragged = mIsMidPointBeingDragged = false;
    mEditedPoint = mEditedMidPoint = nullptr;

    if (GetPropertyValue(Property_Freeze).first)
      return;
    if (mod.R)
      rightClickedDown = true;

    float percentSensitivity = 5;
    float boundsX = mWidgetBounds.L;
    float boundsY = mWidgetBounds.T;
    float boundsW = mWidgetBounds.W();
    float boundsH = mWidgetBounds.H();

    //TODO: sprawdzić czy nie można wykorzystać tutaj jakoś IRECT
    for (auto it : mPoints) {
      if (std::abs(((x - boundsX) / boundsW) * 100.0 - it->x) < percentSensitivity
          && std::abs((100.0 - ((y - boundsY) / boundsH) * 100.0) - it->y) < percentSensitivity) {
        mIsPointBeingDragged = true;
        mEditedPoint = it;
        SetDirty(false);
        break;
      }
      if (std::abs(x - it->midPoint.x) < percentSensitivity
        && std::abs(y - it->midPoint.y) < percentSensitivity) {
        mIsMidPointBeingDragged = true;
        mEditedMidPoint = it;
        SetDirty(false);
        break;
      }
    }
  }

  void HandleRightClick(float x, float y)
  {
    if (GetPropertyValue(Property_Freeze).first)
      return;
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
      if (std::abs(x - it->midPoint.x) < percentSensitivity
        && std::abs(y - it->midPoint.y) < percentSensitivity) {
        it->interpolationFactor = 0.5;
        break;
      }
    }
    if (pointToDelete != nullptr && !pointToDelete->isXRestricted) {
      mPoints.erase(pointToDelete);
      SetDirty(false);
    }
  }

  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override
  {
    if (GetPropertyValue(Property_Freeze).first)
      return;
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

    if (GetPropertyValue(Property_SnapToGrid).first) {
      newX = std::roundf(newX / 5) * 5.0;
      newY = std::roundf(newY / 5) * 5.0;
    }

    mPoints.emplace(new ChartEditorPoint(newX, newY));
    SetDirty(false);
  }

  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    if (GetPropertyValue(Property_Freeze).first)
      return;
    //TODO: Tooltip dla punktu
    if (rightClickedDown) {
      HandleRightClick(x, y);
      rightClickedDown = false;
      return;
    }

    mIsPointBeingDragged = mIsMidPointBeingDragged = false;
    mEditedPoint = mEditedMidPoint = nullptr;
    SetDirty(false);
  }

  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
    //TODO: przerobić logikę mEditedPoint bo teraz można dragować też midpointy...


    if ((mIsPointBeingDragged == false && mIsMidPointBeingDragged == false)
      || (mEditedPoint == nullptr && mEditedMidPoint == nullptr)
      || rightClickedDown
      || GetPropertyValue(Property_Freeze).first)
      return;

    float boundsX = mWidgetBounds.L;
    float boundsY = mWidgetBounds.T;
    float boundsW = mWidgetBounds.W();
    float boundsH = mWidgetBounds.H();

    if (mIsMidPointBeingDragged && mEditedMidPoint != nullptr) {
      double newX = (x - boundsX) / boundsW * 100.0;
      double newY = 100 - (y - boundsY) / boundsH * 100.0;
      auto nextPoint = std::next(mPoints.find(mEditedMidPoint));
      double h = (*nextPoint)->y - mEditedMidPoint->y;
      double newH = newY - mEditedMidPoint->y;

      mEditedMidPoint->interpolationFactor = std::clamp(newH / h, 0.0, 1.0);

      return;
    }

    double newX = (x - boundsX) / boundsW * 100.0;
    double newY = 100 - (y - boundsY) / boundsH * 100.0;

    if (GetPropertyValue(Property_SnapToGrid).first) {
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

  //TODO: odpalać przy dodawaniu/usuwaniu punktów?
  void CalculateMidPoints()
  {
  float x = mWidgetBounds.L;
  float y = mWidgetBounds.T;
  float w = mWidgetBounds.W();
  float h = mWidgetBounds.H();

  IVec2 midPoint;
  for (auto it = mPoints.begin(); it != mPoints.end(); ++it)
  {
    if (std::next(it) == mPoints.end())
      break;
    IVec2 thisPoint = IVec2(x + (*it)->x / 100.0 * w, y + h - (*it)->y / 100.0 * h);
    IVec2 nextPoint = IVec2(x + (*std::next(it))->x / 100.0 * w, y + h - (*std::next(it))->y / 100.0 * h);
    midPoint = thisPoint.GetMidPoint(nextPoint);
    //TODO: only if non linear interpolation!
    auto fy1 = [](double x) { return -2 * x + 2; };
    auto fy2 = [](double x) { return 2 * x; };
    midPoint.y = (((fy1((*it)->interpolationFactor)) * thisPoint.y) + ((fy2((*it)->interpolationFactor)) * nextPoint.y)) / 2.0;
    (*it)->midPoint = midPoint;
  }
  }

  void OnResize() override
  {
    SetTargetRECT(MakeRects(mRECT));

    SetDirty(false);
  }

  void Draw(IGraphics& g) override
  {
    CalculateMidPoints();

    DrawBackground(g, mRECT);
    DrawWidget(g);
    DrawLabel(g);

    if (mStyle.drawFrame)
      g.DrawRect(GetColor(kFR), mWidgetBounds, &mBlend, mStyle.frameThickness);
  }

  float LagrangeInterpolation(IGraphics& g, float xp, IVec2 point1, IVec2 point2, IVec2 point3)
  {
    float p1 = ((xp - point2.x) / (point1.x - point2.x))
      * ((xp - point3.x) / (point1.x - point3.x));
    float p2 = ((xp - point1.x) / (point2.x - point1.x))
      * ((xp - point3.x) / (point2.x - point3.x));
    float p3 = ((xp - point1.x) / (point3.x - point1.x))
      * ((xp - point2.x) / (point3.x - point2.x));
    return p1 * point1.y + p2 * point2.y + p3 * point3.y;
  }

  void DrawWidget(IGraphics& g) override
  {
    float x = mWidgetBounds.L;
    float y = mWidgetBounds.T;
    float w = mWidgetBounds.W();
    float h = mWidgetBounds.H();

    auto pathLinePoints = [&]() {
      for (auto it = mPoints.begin(); it != mPoints.end(); ++it) {
        if (std::next(it) == mPoints.end())
          continue;

        IVec2 thisPoint = IVec2(x + (*it)->x / 100.0 * w, y + h - (*it)->y / 100.0 * h);
        IVec2 nextPoint = IVec2(x + (*std::next(it))->x / 100.0 * w, y + h - (*std::next(it))->y / 100.0 * h);
        //g.DrawLine(COLOR_RED, (*it)->x, (*it)->y, (*it)->midPoint.x, (*it)->midPoint.y);
        //g.DrawLine(COLOR_GREEN, (*it)->midPoint.x, (*it)->midPoint.y, (*std::next(it))->x, (*std::next(it))->y);
        //g.PathQuadraticBezierTo((*it)->midPoint.x, (*it)->midPoint.y, nextPoint.x, nextPoint.y);
        float lastDrawnX = 0;
        for (float xp = thisPoint.x + 5; xp <= nextPoint.x; xp += 5) {
          float yp = LagrangeInterpolation(g, xp, thisPoint, (*it)->midPoint, nextPoint);
          float minValue = std::min(thisPoint.y, nextPoint.y);
          float maxValue = std::max(thisPoint.y, nextPoint.y);
          g.PathLineTo(xp, std::clamp(yp, minValue, maxValue));
          lastDrawnX = xp;
        }
        if (lastDrawnX < nextPoint.x)
          g.PathLineTo(nextPoint.x, nextPoint.y);
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
    if (!GetPropertyValue(Property_Freeze).first)
    {
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
    }

    //Linie odniesienia w tle
    g.DrawLine(COLOR_WHITE, x, y + h, x + w, y, &BLEND_10);
    g.DrawLine(COLOR_WHITE, x, y + h / 2.0f, x + w, y + h / 2.0f, &BLEND_10);
    g.DrawLine(COLOR_WHITE, x + w / 2.0f, y, x + w / 2.0f, y + h, &BLEND_10);

    //Skala na osi X
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
    g.DrawText(mStyle.valueText.WithSize(12.0f).WithFGColor(COLOR_WHITE), "3", IRECT(x + w / 2 + scale, y, x + w / 2 + 3 * scale, y + 15));
    g.DrawText(mStyle.valueText.WithSize(12.0f).WithFGColor(COLOR_WHITE), "6", IRECT(x + w / 2 + 3 * scale, y, x + w / 2 + 5 * scale, y + 15));
    g.DrawText(mStyle.valueText.WithSize(12.0f).WithFGColor(COLOR_WHITE), "9", IRECT(x + w / 2 + 5 * scale, y, x + w / 2 + 7 * scale, y + 15));

    //Skala na osi Y
    g.DrawLine(COLOR_WHITE, x + w - 2, y + h - 2 * scale, x + w + 1, y + h - 2 * scale, &BLEND_75);
    g.DrawLine(COLOR_WHITE, x + w - 2, y + h - 4 * scale, x + w + 1, y + h - 4 * scale, &BLEND_75);
    g.DrawLine(COLOR_WHITE, x + w - 2, y + h - 5 * scale, x + w + 1, y + h - 5 * scale, &BLEND_75);
    g.DrawLine(COLOR_WHITE, x + w - 2, y + h - 6 * scale, x + w + 1, y + h - 6 * scale, &BLEND_75);
    g.DrawLine(COLOR_WHITE, x + w - 2, y + h - 7 * scale, x + w + 1, y + h - 7 * scale, &BLEND_75);
    g.DrawLine(COLOR_WHITE, x + w - 2, y + 2 * scale, x + w + 1, y + 2 * scale, &BLEND_75);
    g.DrawLine(COLOR_WHITE, x + w - 2, y + 4 * scale, x + w + 1, y + 4 * scale, &BLEND_75);
    g.DrawLine(COLOR_WHITE, x + w - 2, y + 6 * scale, x + w + 1, y + 6 * scale, &BLEND_75);

    g.DrawText(mStyle.valueText.WithSize(12.0f).WithFGColor(COLOR_WHITE), "-18", IRECT(x + w - 18, y + h - 3 * scale - 5, x + w - 3, y + h - scale - 5));
    g.DrawText(mStyle.valueText.WithSize(12.0f).WithFGColor(COLOR_WHITE), "-12", IRECT(x + w - 18, y + h - 5 * scale - 5, x + w - 3, y + h - 3 * scale - 5));
    g.DrawText(mStyle.valueText.WithSize(12.0f).WithFGColor(COLOR_WHITE), "-9", IRECT(x + w - 15, y + h - 3 * scale - 5, x + w, y + h - 4 * scale - 5));
    g.DrawText(mStyle.valueText.WithSize(12.0f).WithFGColor(COLOR_WHITE), "-6", IRECT(x + w - 15, y + h - 7 * scale - 5, x + w, y + h - 5 * scale - 5));
    g.DrawText(mStyle.valueText.WithSize(12.0f).WithFGColor(COLOR_WHITE), "-3", IRECT(x + w - 15, y + h - 8 * scale - 5, x + w, y + h - 6 * scale - 5));
    g.DrawText(mStyle.valueText.WithSize(12.0f).WithFGColor(COLOR_WHITE), "3", IRECT(x + w - 15, y + 5 * scale - 5, x + w, y + 7 * scale - 5));
    g.DrawText(mStyle.valueText.WithSize(12.0f).WithFGColor(COLOR_WHITE), "6", IRECT(x + w - 15, y + 3 * scale - 5, x + w, y + 5 * scale - 5));
    g.DrawText(mStyle.valueText.WithSize(12.0f).WithFGColor(COLOR_WHITE), "9", IRECT(x + w - 15, y + scale - 5, x + w, y + 3 * scale - 5));


    //Rysowanie kontrolek pomiędzy punktami
    if (!GetPropertyValue(Property_Freeze).first)
    {
      for (auto it = mPoints.begin(); it != mPoints.end(); ++it) {
        if (std::next(it) == mPoints.end()) {
          continue;
        }
        g.DrawCircle(mColor, (*it)->midPoint.x, (*it)->midPoint.y, 4.0f);
      }
    }

    //TODO:zmiana Property w innej kontrolce musi wywołać redraw tutaj
    
    ////TODO: DELETE
    ////Pisanie indeksów punktów
    //for (auto it : mPoints) {
    //  double vx = x + it->x / 100.0 * w;
    //  double vy = y + h - it->y / 100.0 * h;
    //  int distance = std::distance(mPoints.begin(), mPoints.find(it));
    //  g.DrawText(mStyle.valueText.WithSize(16.0f).WithFGColor(COLOR_WHITE), std::to_string(distance).c_str(), IRECT(vx - 10, vy, vx + 10, vy - 20));
    //}
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

  IRECT getPlotBound() {
    return mWidgetBounds;
  }

  IColor GetColor() {
    return mColor;
  }

  const IColor& GetColor(EVColor color) const
  {
    return mStyle.colorSpec.GetColor(color);
  }

  //void setIsEditable(bool isEditable)
  //{
  //  mIsEditable = isEditable;
  //}

  //void setSnapToGrid(bool snapToGrid)
  //{
  //  mSnapToGrid = snapToGrid;
  //}

  //bool getIsEditable()
  //{
  //  return mIsEditable;
  //}

  //bool getSnapToGrid()
  //{
  //  return mSnapToGrid;
  //}

  //TODO: jakie info wystawiać? dane na podstawie których tworzone są punkty, czy to po prostu sama tablica punktów czy raczej jakieś współczynniki?
  
private:
  IColor mColor;
  std::set<ChartEditorPoint*, ChartEditorPointComparer> mPoints;
  //bool mSnapToGrid;
  //bool mIsEditable;
  ChartEditorPoint* mEditedPoint = nullptr;
  ChartEditorPoint* mFocusedPoint = nullptr;
  bool mIsPointBeingDragged = false;
  bool mIsMidPointBeingDragged = false;
  ChartEditorPoint* mEditedMidPoint = nullptr;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
