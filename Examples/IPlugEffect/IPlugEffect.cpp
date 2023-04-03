#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

IPlugEffect::IPlugEffect(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    const IRECT b = pGraphics->GetBounds();
    //pGraphics->AttachControl(new ITextControl(b.GetMidVPadded(50), "Hello iPlug 2!", IText(50)));
    pGraphics->AttachControl(new IVKnobControl(b.GetCentredInside(100).GetVShifted(-250), kGain));
    //TODO: propertiesy są bardziej "globalne" - nie dla konkretnego IVChartEditorControl tylko współdzielone
    auto props = new std::unordered_map<std::string, std::pair<bool, std::string>> {
      { Property_SnapToGrid, std::pair<bool, std::string>(true, "magnet") },
      { Property_Freeze, std::pair<bool, std::string>(false, "freeze") }
    };
    IVChartEditorControl* lowChartEditor = new IVChartEditorControl(b, "IVChartEditorControl", DEFAULT_STYLE, COLOR_RED, kTab1, true, true, props);
    IVChartEditorControl* midChartEditor = new IVChartEditorControl(b, "IVChartEditorControl", DEFAULT_STYLE, COLOR_ORANGE, kTab2, true, true, props);
    IVChartEditorControl* highChartEditor = new IVChartEditorControl(b, "IVChartEditorControl", DEFAULT_STYLE, COLOR_YELLOW, kTab3, true, true, props);
    IVChartEditorControl* masterChartEditor = new IVChartEditorControl(b, "IVChartEditorControl", DEFAULT_STYLE, COLOR_GREEN, kTab4, true, true, props);
    pGraphics->AttachControl(new ITabbedPanel(  //TODO: RECT & COLOR from TabbedPanel?
      b.GetCentredInside(200, 300), "ITabbedPanel", DEFAULT_STYLE, std::vector<ITab*> {
        new ITab(lowChartEditor, "LOW"),
        new ITab(midChartEditor, "MID"),
        new ITab(highChartEditor, "HIGH"),
        new ITab(masterChartEditor, "MASTER")
    }));
    pGraphics->AttachControl(new IControlPropsEditor(b.GetCentredInside(200, 50).GetVShifted(100).GetFromRight(50), props));
  };
#endif
}

#if IPLUG_DSP
void IPlugEffect::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  const int nChans = NOutChansConnected();
  
  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * gain;
    }
  }
}
#endif
