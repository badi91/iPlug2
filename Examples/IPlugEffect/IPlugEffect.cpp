#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

IPlugEffect::IPlugEffect(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitDouble("Gain", 0.0, 0.0, 100.0, 0.01, "%");
  GetParam(kHighPre)->InitDouble("PRE", 50.0, 0.0, 100.0, 0.01, "%");
  GetParam(kHighPost)->InitDouble("POST", 50.0, 0.0, 100.0, 0.01, "%");
  GetParam(kHighBalance)->InitDouble("><", 0.0, -100.0, 100.0, 0.02, "");
  GetParam(kSlider)->InitEnum("SLIDER", 4, 4, "", 0, "", "ON", "COMP OFF", "MUTED", "OFF");

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
    pGraphics->EnableMouseOver(true);
    //auto gainKnob = new IVKnobControl(b.GetCentredInside(100).GetVShifted(-100), kGain);
    //pGraphics->AttachControl(gainKnob);
    auto props = new std::unordered_map<std::string, std::pair<bool, std::string>> {
      { Property_SnapToGrid, std::pair<bool, std::string>(true, "magnet") },
      { Property_Freeze, std::pair<bool, std::string>(false, "freeze") }
    };
    //auto chartBounds = b.GetFromTLHC(270, 270).GetPadded(5);
    IVChartEditorControl* lowChartEditor = new IVChartEditorControl(b, "IVChartEditorControl", DEFAULT_STYLE, COLOR_RED, kTab1, true, true, props);
    IVChartEditorControl* midChartEditor = new IVChartEditorControl(b, "IVChartEditorControl", DEFAULT_STYLE, COLOR_ORANGE, kTab2, true, true, props);
    IVChartEditorControl* highChartEditor = new IVChartEditorControl(b, "IVChartEditorControl", DEFAULT_STYLE, COLOR_YELLOW, kTab3, true, true, props);
    IVChartEditorControl* masterChartEditor = new IVChartEditorControl(b, "IVChartEditorControl", DEFAULT_STYLE, COLOR_GREEN, kTab4, true, true, props);

    auto highPreGain = new IVKnobControl(b.GetFromTLHC(70, 95).GetVShifted(305).GetHShifted(205), kHighPre);
    auto highPostGain = new IVKnobControl(b.GetFromTLHC(70, 95).GetVShifted(305).GetHShifted(290), kHighPost);
    auto highBalance = new IVKnobControl(b.GetFromTLHC(60, 85).GetVShifted(405).GetHShifted(140), kHighBalance, "", DEFAULT_STYLE, false, false, -135.0f, 135.0f, 0.0f);
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

    auto slider = new IVSliderControl(b.GetFromTLHC(100, 75).GetVShifted(315).GetHShifted(110), kSlider);
    slider->SetShowValueLabels(true);
    pGraphics->AttachControl(slider);

    auto gainLabel = new IVLabelControl(b.GetFromTLHC(40, 20).GetVShifted(380).GetHShifted(262).GetPadded(2), "GAIN");
    gainLabel->SetColor(kBG, IColor(96, 0, 0, 0));
    gainLabel->SetRoundness(0.5f);
    pGraphics->AttachControl(gainLabel);

    pGraphics->AttachControl(new IVNumberBoxControl(b.GetCentredInside(200)));

    pGraphics->AttachControl(new IVToggleControl(b.GetFromTLHC(90, 60).GetHShifted(5).GetVShifted(440), -1, "SOLO"));
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
