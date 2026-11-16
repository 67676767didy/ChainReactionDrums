#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace IDs {
static constexpr auto tempo = "tempo", chaos = "chaos", damage = "damage", dropout = "dropout";
static constexpr auto tape = "tape", halftime = "halftime", reverb = "reverb", master = "master";
}

ChainReactionDrumsAudioProcessor::ChainReactionDrumsAudioProcessor()
 : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
   parameters(*this, nullptr, "PARAMETERS", {
    std::make_unique<juce::AudioParameterFloat>(IDs::tempo, "Tempo", juce::NormalisableRange<float>(40, 240, 0.1f), 140),
    std::make_unique<juce::AudioParameterFloat>(IDs::chaos, "Chaos", 0.0f, 1.0f, 0.28f),
    std::make_unique<juce::AudioParameterFloat>(IDs::damage, "Damage", 0.0f, 1.0f, 0.18f),
    std::make_unique<juce::AudioParameterFloat>(IDs::dropout, "Dropout", 0.0f, 1.0f, 0.08f),
    std::make_unique<juce::AudioParameterFloat>(IDs::tape, "Tape Stop", 0.0f, 1.0f, 0.05f),
    std::make_unique<juce::AudioParameterFloat>(IDs::halftime, "Half-time", 0.0f, 1.0f, 0.08f),
    std::make_unique<juce::AudioParameterFloat>(IDs::reverb, "Reverb", 0.0f, 1.0f, 0.22f),
    std::make_unique<juce::AudioParameterFloat>(IDs::master, "Master", 0.0f, 1.0f, 0.8f)
   })
{
    for (int i=0;i<nodeCount;++i) {
        nodeChance[(size_t)i].store(0.85f);
        for (int j=0;j<nodeCount;++j) edges[(size_t)i][(size_t)j].store(0.0f);
    }
    // A useful initial directed loop with occasional cross-talk.
    for (int i=0;i<nodeCount;++i) {
        edges[(size_t)i][(size_t)((i+1)%nodeCount)].store(0.72f);
        if (i%2==0) edges[(size_t)i][(size_t)((i+3)%nodeCount)].store(0.22f);
    }
    randomizePattern();
}

float ChainReactionDrumsAudioProcessor::param(const char* id) const { return parameters.getRawParameterValue(id)->load(); }
void ChainReactionDrumsAudioProcessor::prepareToPlay(double sampleRate, int)
{
    sr=sampleRate; sampleClock=0; nextTick=0; step=0;
    lowpass.prepare({sampleRate, 512, 2}); lowpass.setType(juce::dsp::StateVariableTPTFilterType::lowpass); lowpass.setCutoffFrequency(18000.0f);
    reverb.reset(); juce::dsp::Reverb::Parameters rp; rp.roomSize=0.55f; rp.damping=0.45f; rp.wetLevel=0.25f; rp.dryLevel=0.75f; reverb.setParameters(rp);
    delayBuffer.setSize(2, (int)(sampleRate*1.5)); delayBuffer.clear(); delayWrite=0;
}
void ChainReactionDrumsAudioProcessor::releaseResources() {}
bool ChainReactionDrumsAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const { return l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo() || l.getMainOutputChannelSet()==juce::AudioChannelSet::mono(); }

void ChainReactionDrumsAudioProcessor::triggerNode(int node, juce::MidiBuffer& midi, int offset, int depth)
{
    if (node<0 || node>=nodeCount || depth>4) return;
    if (random.nextFloat() > nodeChance[(size_t)node].load()) return;
    voices[(size_t)node].position=0;
    const int notes[8]={36,38,42,46,39,45,51,49};
    midi.addEvent(juce::MidiMessage::noteOn(10, notes[node], (juce::uint8)110), offset);
    midi.addEvent(juce::MidiMessage::noteOff(10, notes[node]), offset+2);
    for (int target=0;target<nodeCount;++target) {
        const float p=edges[(size_t)node][(size_t)target].load();
        if (p>0.001f && random.nextFloat()<p) {
            // The next generation is scheduled a small musical subdivision later by the caller's block clock.
            // A tiny offset avoids same-sample recursive bursts while keeping propagation tight.
            triggerNode(target, midi, offset + std::min(24, (int)(sr*0.008)), depth + 1);
        }
    }
}

void ChainReactionDrumsAudioProcessor::renderVoice(Voice& v, juce::AudioBuffer<float>& out, int start, int count)
{
    if (v.position<0) return;
    if (v.data.getNumSamples()>0) {
        for (int n=0;n<count && v.position<v.data.getNumSamples();++n,++v.position) {
            const int chs=juce::jmin(out.getNumChannels(), v.data.getNumChannels());
            for(int ch=0;ch<chs;++ch) out.addSample(ch,start+n,v.data.getSample(ch,v.position)*v.gain);
            if (chs==1 && out.getNumChannels()>1) out.addSample(1,start+n,v.data.getSample(0,v.position)*v.gain);
        }
        if(v.position>=v.data.getNumSamples()) v.position=-1;
    } else {
        // Built-in synthesized fallback sounds; each node has a distinct simple voice.
        const int length=juce::jmin(count, (int)(sr*(0.08 + (v.position==0?0.0:0.0))));
        for(int n=0;n<length;++n,++v.position) {
            const float t=(float)v.position/(float)sr;
            float s=0.0f;
            switch (&v-&voices[0]) {
                case 0: { float f=48.0f+120.0f*std::exp(-t*35.0f); s=std::sin(juce::MathConstants<float>::twoPi*f*t)*std::exp(-t*24.0f); break; }
                case 1: s=(random.nextFloat()*2.0f-1.0f)*std::exp(-t*32.0f); break;
                case 2: case 3: s=(random.nextFloat()*2.0f-1.0f)*std::exp(-t*95.0f)*0.35f; break;
                case 4: s=std::sin(juce::MathConstants<float>::twoPi*180.0f*t)*std::exp(-t*18.0f)*0.5f; break;
                default: s=std::sin(juce::MathConstants<float>::twoPi*(220.0f+30.0f*(&v-&voices[0]))*t)*std::exp(-t*12.0f)*0.45f; break;
            }
            for(int ch=0;ch<out.getNumChannels();++ch) out.addSample(ch,start+n,s*v.gain);
        }
        if(v.position>=(int)(sr*0.08)) v.position=-1;
    }
}

void ChainReactionDrumsAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int samples=buffer.getNumSamples(); buffer.clear();
    auto playHead=getPlayHead();
    double bpm=param(IDs::tempo); bool hostPlaying=false;
    if(playHead) if(auto pos=playHead->getPosition()) { if(auto hostBpm=pos->getBpm()) bpm=*hostBpm; if(auto ppq=pos->getPpqPosition()) { sampleClock=(int64_t)(*ppq*60.0/bpm*sr); } if(auto playing=pos->getIsPlaying()) hostPlaying=*playing; }
    bpm=juce::jlimit(40.0,300.0,bpm);
    const double sixteenth=sr*60.0/bpm/4.0;
    const float chaos=param(IDs::chaos), damage=param(IDs::damage);
    const bool transport=hostPlaying || !playHead;
    for(int i=0;i<samples;++i) {
        if(transport && sampleClock>=nextTick) {
            const int base=(step%4==0)?0:((step%4==2)?1:2);
            if(random.nextFloat()>param(IDs::dropout)) triggerNode(base,midi,i);
            if(chaos>0.01f && random.nextFloat()<chaos*0.24f) triggerNode(random.nextInt(nodeCount),midi,i);
            if(param(IDs::halftime)>0.01f && random.nextFloat()<param(IDs::halftime)*0.025f) nextTick+=(int64_t)sixteenth;
            ++step; nextTick+=(int64_t)sixteenth; if(step%16==0 && !patternLocked.load() && random.nextFloat()<chaos*0.16f) {
                const int a=random.nextInt(nodeCount), b=random.nextInt(nodeCount); if(a!=b) edges[(size_t)a][(size_t)b].store(random.nextFloat()*0.85f);
            }
        }
        ++sampleClock;
        for(auto& v:voices) renderVoice(v,buffer,i,1);
    }
    const float drive=1.0f+damage*18.0f;
    for(int ch=0;ch<buffer.getNumChannels();++ch) {
        auto* d=buffer.getWritePointer(ch);
        for(int i=0;i<samples;++i) {
            float x=d[i]*drive; d[i]=std::tanh(x)*(1.0f/(1.0f+damage*0.7f));
        }
    }
    if(buffer.getNumChannels()==2) {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        lowpass.setCutoffFrequency(18000.0f-12000.0f*param(IDs::tape));
        lowpass.process(context);
        juce::dsp::Reverb::Parameters rp; rp.roomSize=0.65f; rp.damping=0.45f; rp.wetLevel=param(IDs::reverb)*0.65f; rp.dryLevel=1.0f-rp.wetLevel*0.5f; reverb.setParameters(rp);
        reverb.processStereo(buffer.getWritePointer(0),buffer.getWritePointer(1),samples);
    }
    buffer.applyGain(param(IDs::master));
}

void ChainReactionDrumsAudioProcessor::loadSample(int node,const juce::File& file)
{
    if(node<0||node>=nodeCount||!file.existsAsFile()) return;
    juce::AudioFormatManager fm; fm.registerBasicFormats(); std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(file));
    if(!reader) return;
    auto& v=voices[(size_t)node]; v.data.setSize((int)reader->numChannels,(int)reader->lengthInSamples);
    reader->read(&v.data,0,(int)reader->lengthInSamples,0,true,true); v.sourceRate=reader->sampleRate; v.position=-1; sampleNames[(size_t)node]=file.getFileName();
}
juce::String ChainReactionDrumsAudioProcessor::getSampleName(int n) const { return n>=0&&n<nodeCount ? (sampleNames[(size_t)n].isEmpty()?"BUILT-IN":sampleNames[(size_t)n]) : ""; }
void ChainReactionDrumsAudioProcessor::setEdge(int a,int b,float p){if(a>=0&&a<nodeCount&&b>=0&&b<nodeCount&&a!=b) edges[(size_t)a][(size_t)b].store(juce::jlimit(0.0f,1.0f,p));}
float ChainReactionDrumsAudioProcessor::getEdge(int a,int b) const{return a>=0&&a<nodeCount&&b>=0&&b<nodeCount?edges[(size_t)a][(size_t)b].load():0.0f;}
void ChainReactionDrumsAudioProcessor::randomizePattern(){ if(patternLocked.load()) return; for(int a=0;a<nodeCount;++a) for(int b=0;b<nodeCount;++b) if(a!=b) edges[(size_t)a][(size_t)b].store(random.nextFloat()<0.22f?random.nextFloat()*0.8f:0.0f); }
void ChainReactionDrumsAudioProcessor::savePreset(const juce::File& f){juce::DynamicObject::Ptr o=new juce::DynamicObject(); for(int a=0;a<nodeCount;++a){o->setProperty("chance"+juce::String(a),nodeChance[(size_t)a].load()); for(int b=0;b<nodeCount;++b)o->setProperty("e"+juce::String(a)+"_"+juce::String(b),getEdge(a,b));} for(auto* id:{IDs::tempo,IDs::chaos,IDs::damage,IDs::dropout,IDs::tape,IDs::halftime,IDs::reverb,IDs::master})o->setProperty(id,param(id)); f.replaceWithText(juce::JSON::toString(juce::var(o),true));}
void ChainReactionDrumsAudioProcessor::loadPreset(const juce::File& f){auto v=juce::JSON::parse(f); if(auto* o=v.getDynamicObject()){for(int a=0;a<nodeCount;++a){if(o->hasProperty("chance"+juce::String(a)))nodeChance[(size_t)a].store((float)o->getProperty("chance"+juce::String(a)));for(int b=0;b<nodeCount;++b){auto k="e"+juce::String(a)+"_"+juce::String(b);if(o->hasProperty(k))setEdge(a,b,(float)o->getProperty(k));}} for(auto* id:{IDs::tempo,IDs::chaos,IDs::damage,IDs::dropout,IDs::tape,IDs::halftime,IDs::reverb,IDs::master})if(o->hasProperty(id))parameters.getParameter(id)->setValueNotifyingHost(parameters.getParameter(id)->convertTo0to1((float)o->getProperty(id)));}}
void ChainReactionDrumsAudioProcessor::setCurrentProgram(int i){currentProgram=juce::jlimit(0,9,i); if(i==0)randomizePattern(); else {random.setSeed((juce::int64)(i*918273+42));randomizePattern();}}
juce::String ChainReactionDrumsAudioProcessor::getProgramName(int i){const char* n[]={"01 / FRACTURE","02 / GLASS TEETH","03 / ROTOR","04 / DUST CIRCUIT","05 / IMPACT ZONE","06 / HYPERSTATIC","07 / TAPE GHOST","08 / BROKEN GRID","09 / FEEDBACK","10 / NULL RITUAL"};return n[juce::jlimit(0,9,i)];}
void ChainReactionDrumsAudioProcessor::getStateInformation(juce::MemoryBlock& d){auto state=parameters.copyState(); for(int a=0;a<nodeCount;++a)for(int b=0;b<nodeCount;++b)state.setProperty("edge"+juce::String(a)+"_"+juce::String(b),getEdge(a,b),nullptr); juce::MemoryOutputStream s(d,true);state.writeToStream(s);}
void ChainReactionDrumsAudioProcessor::setStateInformation(const void* data,int size){auto state=juce::ValueTree::readFromData(data,(size_t)size);if(state.isValid()){parameters.replaceState(state);for(int a=0;a<nodeCount;++a)for(int b=0;b<nodeCount;++b){auto k="edge"+juce::String(a)+"_"+juce::String(b);if(state.hasProperty(k))setEdge(a,b,(float)state.getProperty(k));}}}
juce::AudioProcessorEditor* ChainReactionDrumsAudioProcessor::createEditor(){return new ChainReactionDrumsAudioProcessorEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new ChainReactionDrumsAudioProcessor();}
