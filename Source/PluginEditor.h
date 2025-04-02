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
struct ExtendedTabbedButtonBar : juce::TabbedButtonBar, juce::DragAndDropTarget, juce::DragAndDropContainer
{
    ExtendedTabbedButtonBar();
    bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override;
    /*
     Drag-to-reorder is accomplished by the TabbedButtonBar being a dragAndDrop Target and a DragAndDropContainer, and by listening to mouse events on each TabBarButton.
     
     the sequence flow is:
     mouseDown on a tab.  this starts the DragAndDropContainer responding to mouseDrag events.
     first, itemDragEnter is called when the first mouseEvent happens.
     as the mouse is moved, itemDragMove() is called.  the tabBarButtons are constrained to the bounds of the ExtendedTabbedButtonBar, so they'll never be dragged outside of it.
     
     itemDragMove() checks the x coordinate of the item being dragged and compares it with its neighbors.
     tab indexes are swapped if a tab crosses over the middle of another tab.
     */
    void itemDragEnter (const SourceDetails& dragSourceDetails) override;
    void itemDragMove (const SourceDetails& dragSourceDetails) override;
    void itemDragExit (const SourceDetails& dragSourceDetails) override;
    void itemDropped (const SourceDetails& dragSourceDetails) override;
    
    void mouseDown(const juce::MouseEvent& e) override;
    
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
    
    void mouseDown (const juce::MouseEvent& e) override;

    void mouseDrag (const juce::MouseEvent& e) override;
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
