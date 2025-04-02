/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Project13AudioProcessorEditor::Project13AudioProcessorEditor (Project13AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    dspOrderButton.onClick = [this]()
    {
        juce::Random r;
        Project13AudioProcessor::DSP_Order dspOrder;
        
        auto range = juce::Range<int>(static_cast<int>(Project13AudioProcessor::DSP_Option::Phase),
                                      static_cast<int>(Project13AudioProcessor::DSP_Option::END_OF_LIST));
        for( auto& v : dspOrder )
        {
            auto entry = r.nextInt(range);
            v = static_cast<Project13AudioProcessor::DSP_Option>(entry);
        }
        DBG( juce::Base64::toBase64(dspOrder.data(), dspOrder.size()));
        jassertfalse;
        
        audioProcessor.dspOrderFifo.push(dspOrder);
    };
    
    
    addAndMakeVisible(dspOrderButton);
    setSize (400, 300);
}

Project13AudioProcessorEditor::~Project13AudioProcessorEditor()
{
}

//==============================================================================
void Project13AudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void Project13AudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    
    dspOrderButton.setBounds(getLocalBounds().reduced(100));
}
