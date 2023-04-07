#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

IPlugEffect::IPlugEffect(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");
  GetParam(kHighPre)->InitDouble("PRE", 0., 0.0, 100.0, 0.01, "%");
  GetParam(kHighPost)->InitDouble("POST", 0., 0., 100.0, 0.01, "%");
  GetParam(kHighBalance)->InitDouble("><", 0., -100.0, 100.0, 0.02, "");

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
    //pGraphics->AttachControl(new IVKnobControl(b.GetCentredInside(100).GetVShifted(-250), kGain));
    auto props = new std::unordered_map<std::string, std::pair<bool, std::string>> {
      { Property_SnapToGrid, std::pair<bool, std::string>(true, "magnet") },
      { Property_Freeze, std::pair<bool, std::string>(false, "freeze") }
    };
    //auto chartBounds = b.GetFromTLHC(270, 270).GetPadded(5);
    IVChartEditorControl* lowChartEditor = new IVChartEditorControl(b, "IVChartEditorControl", DEFAULT_STYLE, COLOR_RED, kTab1, true, true, props);
    IVChartEditorControl* midChartEditor = new IVChartEditorControl(b, "IVChartEditorControl", DEFAULT_STYLE, COLOR_ORANGE, kTab2, true, true, props);
    IVChartEditorControl* highChartEditor = new IVChartEditorControl(b, "IVChartEditorControl", DEFAULT_STYLE, COLOR_YELLOW, kTab3, true, true, props);
    IVChartEditorControl* masterChartEditor = new IVChartEditorControl(b, "IVChartEditorControl", DEFAULT_STYLE, COLOR_GREEN, kTab4, true, true, props);

    auto highPreGain = new IVKnobControl(b.GetFromTLHC(70, 95).GetVShifted(325).GetHShifted(215), kHighPre);
    auto highPostGain = new IVKnobControl(b.GetFromTLHC(70, 95).GetVShifted(325).GetHShifted(290), kHighPost);
    auto highBalance = new IVKnobControl(b.GetFromTLHC(70, 95).GetVShifted(425).GetHShifted(150), kHighBalance, "", DEFAULT_STYLE, false, false, -135.0f, 135.0f, 0.0f);
    highPreGain->SetTrackSize(8.0f)->SetShowValue(false);
    highPostGain->SetTrackSize(8.0f)->SetShowValue(false);
    highBalance->SetTrackSize(8.0f)->SetShowValue(false);
    highPreGain->SetColor(kX1, IColor(255, 140, 83, 215));
    highPostGain->SetColor(kX1, IColor(255, 0, 170, 200));
    highBalance->SetColor(kX1, COLOR_WHITE);

    auto highTab = new ITab(highChartEditor, "HIGH");
    highTab->AttachIControl(highPreGain);
    highTab->AttachIControl(highPostGain);
    highTab->AttachIControl(highBalance);

    pGraphics->AttachControl(new ITabbedPanel(  //TODO: RECT & COLOR from TabbedPanel?
      b, "", DEFAULT_STYLE, std::vector<ITab*> {
        new ITab(lowChartEditor, "LOW"),
        new ITab(midChartEditor, "MID"),
        highTab,
        new ITab(masterChartEditor, "MASTER")
    }));
    pGraphics->AttachControl(new IControlPropsEditor(b.GetFromTLHC(250, 30).GetVShifted(265).GetHShifted(5), props));
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
