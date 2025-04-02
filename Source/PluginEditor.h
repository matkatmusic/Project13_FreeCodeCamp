/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

/*
 https://forum.juce.com/t/draggabletabbedcomponent/13265/5?u=matkatmusic
 "You can create a subclass of TabbedButtonBar which is also a DragAndDropTarget. And you can create custom tab buttons which allow themselves to be dragged."
 */
struct ExtendedTabbedButtonBar : juce::TabbedButtonBar, juce::DragAndDropTarget
{
    ExtendedTabbedButtonBar() : juce::TabbedButtonBar(juce::TabbedButtonBar::Orientation::TabsAtTop) { }
    
    bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override { return false; }
    void itemDropped (const SourceDetails& dragSourceDetails) override { }
    
    juce::TabBarButton* createTabButton (const juce::String& tabName, int tabIndex) override;
};

struct HorizontalConstrainer : juce::ComponentBoundsConstrainer
{
    HorizontalConstrainer(std::function<juce::Rectangle<int>()> confinerBoundsGetter,
                          std::function<juce::Rectangle<int>()> confineeBoundsGetter);
    
    void checkBounds (juce::Rectangle<int>& bounds,
                      const juce::Rectangle<int>& previousBounds,
                      const juce::Rectangle<int>& limits,
                      bool isStretchingTop,
                      bool isStretchingLeft,
                      bool isStretchingBottom,
                      bool isStretchingRight) override;
    
private:
    std::function<juce::Rectangle<int>()> boundsToConfineToGetter;
    std::function<juce::Rectangle<int>()> boundsOfConfineeGetter;
};

struct ExtendedTabBarButton : juce::TabBarButton
{
    ExtendedTabBarButton(const juce::String& name, juce::TabbedButtonBar& owner);
    juce::ComponentDragger dragger;
    std::unique_ptr<HorizontalConstrainer> constrainer;
    
    void mouseDown (const juce::MouseEvent& e)
    {
        toFront(true);
        dragger.startDraggingComponent (this, e);
        juce::TabBarButton::mouseDown(e);
    }

    void mouseDrag (const juce::MouseEvent& e)
    {
        dragger.dragComponent (this, e, constrainer.get());
    }
    
};
//==============================================================================
/**
*/
class Project13AudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    Project13AudioProcessorEditor (Project13AudioProcessor&);
    ~Project13AudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    Project13AudioProcessor& audioProcessor;

    juce::TextButton dspOrderButton { "dsp order" };
    
    ExtendedTabbedButtonBar tabbedComponent;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Project13AudioProcessorEditor)
};
