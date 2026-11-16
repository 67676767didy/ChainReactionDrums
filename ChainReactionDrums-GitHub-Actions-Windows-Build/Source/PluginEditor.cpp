#include "PluginEditor.h"

// CRD-inspired visual language: precise graph paper, thin patch wires, square labels.
// The reference's white canvas is translated into a restrained charcoal / warm-white theme.
static juce::Colour bg(0xff111315), panel(0xff171a1d), line(0xff454a4e), muted(0xff9da3a6), ink(0xfff1f0eb), accent(0xffe7e3d8), hot(0xffffb35c);
static const char* nodeNames[8] = {"kick", "snare", "ghost", "snare roll", "ghost flam", "hat", "sub", "knock"};

ChainReactionDrumsAudioProcessorEditor::ChainReactionDrumsAudioProcessorEditor(ChainReactionDrumsAudioProcessor& p)
 : AudioProcessorEditor(&p), processor(p)
{
    setSize(1180, 820); setResizable(true, true); setResizeLimits(900, 680, 1600, 1100);
    title=std::make_unique<juce::Label>(); title->setText("CRD   /   CHAIN REACTION DRUM MACHINE",juce::dontSendNotification); title->setFont(juce::Font(23.0f,juce::Font::plain)); title->setColour(juce::Label::textColourId,ink); addAndMakeVisible(*title);
    subtitle=std::make_unique<juce::Label>(); subtitle->setText("GENERATIVE PERCUSSION SYSTEM     •     PROBABILITY PATCHING     •     MIDI / AUDIO",juce::dontSendNotification); subtitle->setFont(juce::Font(10.0f)); subtitle->setColour(juce::Label::textColourId,muted); addAndMakeVisible(*subtitle);
    status=std::make_unique<juce::Label>(); status->setText("READY  /  DROP WAV OR AIFF ON A NODE TO LOAD A SAMPLE",juce::dontSendNotification); status->setFont(juce::Font(10.0f)); status->setColour(juce::Label::textColourId,hot); addAndMakeVisible(*status);
    auto makeSlider=[this](std::unique_ptr<juce::Slider>& s,const juce::String& name){s=std::make_unique<juce::Slider>();s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);s->setTextBoxStyle(juce::Slider::TextBoxBelow,false,58,15);s->setColour(juce::Slider::rotarySliderFillColourId,hot);s->setColour(juce::Slider::rotarySliderOutlineColourId,line);s->setColour(juce::Slider::thumbColourId,ink);s->setColour(juce::Slider::textBoxTextColourId,ink);s->setColour(juce::Slider::textBoxOutlineColourId,line);addAndMakeVisible(*s);auto l=std::make_unique<juce::Label>();l->setText(name,juce::dontSendNotification);l->setFont(juce::Font(9.5f,juce::Font::bold));l->setColour(juce::Label::textColourId,muted);l->setJustificationType(juce::Justification::centred);addAndMakeVisible(*l);return l;};
    auto attach=[&](std::unique_ptr<juce::Slider>& s,std::unique_ptr<juce::Label>& l,const char* id,const juce::String& n,double lo,double hi,double inc){l=makeSlider(s,n);s->setRange(lo,hi,inc);};
    attach(tempo,knobLabels[0],"tempo","TEMPO",40,240,0.1); attach(chaos,knobLabels[1],"chaos","CHAOS",0,1,0.01); attach(damage,knobLabels[2],"damage","DAMAGE",0,1,0.01); attach(dropout,knobLabels[3],"dropout","DROP OUT",0,1,0.01);
    attach(tape,knobLabels[4],"tape","TAPE / FILTER",0,1,0.01); attach(half,knobLabels[5],"half","HALF-TIME",0,1,0.01); attach(verb,knobLabels[6],"verb","REVERB",0,1,0.01); attach(master,knobLabels[7],"master","OUTPUT",0,1,0.01);
    tempoA=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,"tempo",*tempo);
    chaosA=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,"chaos",*chaos);
    damageA=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,"damage",*damage);
    dropoutA=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,"dropout",*dropout);
    tapeA=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,"tape",*tape);
    halfA=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,"halftime",*half);
    verbA=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,"reverb",*verb);
    masterA=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,"master",*master);
    for(int i=0;i<8;++i){sampleButtons[(size_t)i]=std::make_unique<juce::TextButton>("LOAD");sampleButtons[(size_t)i]->setColour(juce::TextButton::buttonColourId,panel);sampleButtons[(size_t)i]->setColour(juce::TextButton::textColourOffId,muted);sampleButtons[(size_t)i]->setColour(juce::TextButton::buttonOnColourId,hot);sampleButtons[(size_t)i]->onClick=[this,i]{chooseSample(i);};addAndMakeVisible(*sampleButtons[(size_t)i]);}
    auto makeButton=[this](const juce::String& text){auto b=std::make_unique<juce::TextButton>(text);b->setColour(juce::TextButton::buttonColourId,panel);b->setColour(juce::TextButton::textColourOffId,ink);b->setColour(juce::TextButton::buttonOnColourId,hot);addAndMakeVisible(*b);return b;};
    playLock=makeButton("LOCK PATTERN"); playLock->onClick=[this]{processor.setPatternLocked(!processor.isPatternLocked());playLock->setButtonText(processor.isPatternLocked()?"PATTERN LOCKED":"LOCK PATTERN");};
    randomize=makeButton("MUTATE GRAPH"); randomize->onClick=[this]{processor.randomizePattern();repaint();};
    save=makeButton("EXPORT PRESET"); save->onClick=[this]{choosePreset(true);}; load=makeButton("IMPORT PRESET"); load->onClick=[this]{choosePreset(false);};
    startTimerHz(20);
}
void ChainReactionDrumsAudioProcessorEditor::setupKnob(juce::Slider&,juce::Label&,const juce::String&,double,double,double){}
void ChainReactionDrumsAudioProcessorEditor::resized()
{
    auto r=getLocalBounds().reduced(22); title->setBounds(r.removeFromTop(34)); subtitle->setBounds(r.removeFromTop(20)); r.removeFromTop(12);
    auto content=r.removeFromTop((int)(r.getHeight()*0.77f)); auto controls=content.removeFromRight(285).reduced(12); auto graph=content;
    auto row=controls.removeFromTop(82); tempo->setBounds(row.removeFromLeft(125).reduced(4)); knobLabels[0]->setBounds(tempo->getX(),tempo->getY()-13,tempo->getWidth(),13); chaos->setBounds(row.removeFromLeft(125).reduced(4)); knobLabels[1]->setBounds(chaos->getX(),chaos->getY()-13,chaos->getWidth(),13);
    row=controls.removeFromTop(82); damage->setBounds(row.removeFromLeft(125).reduced(4)); knobLabels[2]->setBounds(damage->getX(),damage->getY()-13,damage->getWidth(),13); dropout->setBounds(row.removeFromLeft(125).reduced(4)); knobLabels[3]->setBounds(dropout->getX(),dropout->getY()-13,dropout->getWidth(),13);
    row=controls.removeFromTop(82); tape->setBounds(row.removeFromLeft(125).reduced(4)); knobLabels[4]->setBounds(tape->getX(),tape->getY()-13,125,13); half->setBounds(row.removeFromLeft(125).reduced(4)); knobLabels[5]->setBounds(half->getX(),half->getY()-13,125,13);
    row=controls.removeFromTop(82); verb->setBounds(row.removeFromLeft(125).reduced(4)); knobLabels[6]->setBounds(verb->getX(),verb->getY()-13,125,13); master->setBounds(row.removeFromLeft(125).reduced(4)); knobLabels[7]->setBounds(master->getX(),master->getY()-13,125,13);
    auto actions=controls.removeFromTop(38); playLock->setBounds(actions.removeFromLeft(actions.getWidth()/2).reduced(2)); randomize->setBounds(actions.reduced(2));
    actions=controls.removeFromTop(38); save->setBounds(actions.removeFromLeft(actions.getWidth()/2).reduced(2)); load->setBounds(actions.reduced(2));
    auto area=graphArea(); const float nx[8]={0.16f,0.50f,0.83f,0.91f,0.78f,0.49f,0.14f,0.77f}; const float ny[8]={0.34f,0.16f,0.16f,0.35f,0.57f,0.72f,0.83f,0.84f};
    for(int i=0;i<8;++i)nodePoints[(size_t)i]={area.getX()+area.getWidth()*nx[i],area.getY()+area.getHeight()*ny[i]};
    for(int i=0;i<8;++i)sampleButtons[(size_t)i]->setBounds((int)nodePoints[(size_t)i].x+29,(int)nodePoints[(size_t)i].y-10,42,19);
    auto bottom=r.removeFromTop(42); auto bw=bottom.getWidth()/4; playLock->setBounds(playLock->getBounds());
    status->setBounds(r.removeFromTop(20));
}
juce::Rectangle<float> ChainReactionDrumsAudioProcessorEditor::graphArea() const {auto r=getLocalBounds().reduced(22);r.removeFromTop(66);r.removeFromRight(285);r.removeFromBottom(105);return r.toFloat();}
int ChainReactionDrumsAudioProcessorEditor::hitNode(juce::Point<float> p) const {for(int i=0;i<8;++i)if(p.getDistanceFrom(nodePoints[(size_t)i])<48)return i;return -1;}
void ChainReactionDrumsAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg); auto b=getLocalBounds().toFloat(); g.setColour(panel);g.fillRect(b.reduced(8));g.setColour(line);g.drawRect(b.reduced(8),1.0f);
    auto ga=graphArea(); g.setColour(line.withAlpha(0.6f));g.drawRect(ga,1.0f);
    g.setColour(muted);g.setFont(9.0f);g.drawText("PATCH GRAPH   /   DRAG FROM A NODE TO CREATE OR REMOVE A PROBABILITY LINK",ga.removeFromTop(18).toNearestInt(),juce::Justification::centredLeft);
    for(int a=0;a<8;++a)for(int c=0;c<8;++c){float p=processor.getEdge(a,c);if(p>0.01f){auto s=nodePoints[(size_t)a],e=nodePoints[(size_t)c];g.setColour(accent.withAlpha(0.22f+0.65f*p));g.drawLine(s.x,s.y,e.x,e.y,1.0f+1.3f*p);auto mid=(s+e)*0.5f;g.setColour(ink);g.setFont(9.0f);g.drawText(juce::String((int)std::round(p*100))+"%",(int)mid.x-16,(int)mid.y-9,32,15,juce::Justification::centred);g.fillRect(mid.x-2.0f,mid.y-2.0f,4.0f,4.0f);}}
    for(int i=0;i<8;++i){auto p=nodePoints[(size_t)i];juce::Rectangle<float> box(p.x-35,p.y-13,70,26);g.setColour(i==selectedNode?hot:ink);g.drawRect(box,1.0f);g.setColour(ink);g.fillEllipse(p.x-28,p.y-6,12,12);g.setColour(bg);g.fillEllipse(p.x-25,p.y-3,6,6);g.setColour(ink);g.setFont(juce::Font(11.0f,juce::Font::plain));g.drawText(nodeNames[i],p.x-10,p.y-10,78,20,juce::Justification::centredLeft);}
    if(draggingEdge&&dragFrom>=0){g.setColour(hot);g.drawLine(juce::Line<float>(nodePoints[(size_t)dragFrom],getMouseXYRelative().toFloat()),1.5f);}
    // Recovery / take panel inspired by the supplied second reference video.
    auto side=getLocalBounds().removeFromRight(285).reduced(14);side.removeFromTop(74);g.setColour(line);g.drawLine((float)side.getX(),(float)side.getY(),(float)side.getX(),(float)side.getBottom(),1.0f);g.setColour(ink);g.setFont(11.0f);g.drawText("TAKE / RECOVERY",side.removeFromTop(22),juce::Justification::centredLeft);g.setColour(muted);g.setFont(9.0f);g.drawText("LAST TAKE     —     NO TAKE CAPTURE IN THIS BUILD",side.removeFromTop(18),juce::Justification::centredLeft);g.drawText("RECOVERY BUFFER     /     5 MINUTES",side.removeFromTop(18),juce::Justification::centredLeft);g.setColour(line);g.drawRect(side.removeFromTop(54).toFloat(),1.0f);g.setColour(muted);g.drawText("30 SEC     1 MIN     2 MIN     ALL",side.removeFromTop(22),juce::Justification::centredLeft);g.drawText("MIX + INDIVIDUAL STEMS",side.removeFromTop(18),juce::Justification::centredLeft);g.drawText("Aligned export to your DAW timeline",side.removeFromTop(18),juce::Justification::centredLeft);
    // Timeline / activity grid
    auto timeline=getLocalBounds().reduced(22);timeline.removeFromTop((int)(getHeight()*0.77f)+66);timeline.removeFromBottom(25);g.setColour(ink);g.setFont(10.0f);g.drawText("PATTERN ACTIVITY   /   16-STEP OVERVIEW",timeline.removeFromTop(16),juce::Justification::centredLeft);auto grid=timeline.toFloat();g.setColour(line);g.drawRect(grid,1.0f);for(int i=1;i<16;++i){float x=grid.getX()+grid.getWidth()*i/16.0f;g.drawVerticalLine((int)x,grid.getY(),grid.getBottom());}for(int i=1;i<4;++i){float y=grid.getY()+grid.getHeight()*i/4.0f;g.drawHorizontalLine((int)y,grid.getX(),grid.getRight());}
}
void ChainReactionDrumsAudioProcessorEditor::mouseDown(const juce::MouseEvent& e){selectedNode=hitNode(e.position);if(selectedNode>=0){dragFrom=selectedNode;draggingEdge=true;}repaint();}
void ChainReactionDrumsAudioProcessorEditor::mouseDrag(const juce::MouseEvent&){if(draggingEdge)repaint();}
void ChainReactionDrumsAudioProcessorEditor::mouseUp(const juce::MouseEvent& e){if(draggingEdge&&dragFrom>=0){int to=hitNode(e.position);if(to>=0&&to!=dragFrom){float v=processor.getEdge(dragFrom,to);processor.setEdge(dragFrom,to,v>0.05f?0.0f:0.65f);}}draggingEdge=false;dragFrom=-1;repaint();}
bool ChainReactionDrumsAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& f){for(auto& s:f)if(s.endsWithIgnoreCase(".wav")||s.endsWithIgnoreCase(".aif")||s.endsWithIgnoreCase(".aiff"))return true;return false;}
void ChainReactionDrumsAudioProcessorEditor::filesDropped(const juce::StringArray& f,int x,int y){int n=hitNode({(float)x,(float)y});if(n<0)n=selectedNode>=0?selectedNode:0;if(f.size()>0){processor.loadSample(n,juce::File(f[0]));status->setText("LOADED "+processor.getSampleName(n)+" INTO "+nodeNames[n],juce::dontSendNotification);repaint();}}
void ChainReactionDrumsAudioProcessorEditor::chooseSample(int n){auto chooser=std::make_shared<juce::FileChooser>("Load a drum sample",juce::File{},"*.wav;*.aif;*.aiff");chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[this,n,chooser](const juce::FileChooser& fc){auto f=fc.getResult();if(f.existsAsFile()){processor.loadSample(n,f);status->setText("LOADED "+f.getFileName(),juce::dontSendNotification);repaint();}});}
void ChainReactionDrumsAudioProcessorEditor::choosePreset(bool saving){auto chooser=std::make_shared<juce::FileChooser>(saving?"Export preset":"Import preset",juce::File{},"*.json");auto flags=saving?(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles):(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles);chooser->launchAsync(flags,[this,saving,chooser](const juce::FileChooser& fc){auto f=fc.getResult();if(f==juce::File{})return;if(saving)processor.savePreset(f);else processor.loadPreset(f);status->setText(saving?"PRESET EXPORTED":"PRESET IMPORTED",juce::dontSendNotification);repaint();});}
void ChainReactionDrumsAudioProcessorEditor::timerCallback(){if(processor.isPatternLocked())playLock->setButtonText("PATTERN LOCKED");}
