/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <RotarySliderWithLabels.h>
#include <Utilities.h>
#include <CustomButtons.h>

static juce::String getNameFromDSPOption(Project13AudioProcessor::DSP_Option option)
{
    switch (option)
    {
        case Project13AudioProcessor::DSP_Option::Phase:
            return "PHASE";
        case Project13AudioProcessor::DSP_Option::Chorus:
            return "CHORUS";
        case Project13AudioProcessor::DSP_Option::OverDrive:
            return "OVERDRIVE";
        case Project13AudioProcessor::DSP_Option::LadderFilter:
            return "LADDERFILTER";
        case Project13AudioProcessor::DSP_Option::GeneralFilter:
            return "GEN FILTER";
        case Project13AudioProcessor::DSP_Option::END_OF_LIST:
            jassertfalse;
    }
    
    return "NO SELECTION";
}

static Project13AudioProcessor::DSP_Option getDSPOptionFromName( juce::String name)
{
    if( name == "PHASE" )
        return Project13AudioProcessor::DSP_Option::Phase;
    if( name == "CHORUS" )
        return Project13AudioProcessor::DSP_Option::Chorus;
    if( name == "OVERDRIVE" )
        return Project13AudioProcessor::DSP_Option::OverDrive;
    if( name == "LADDERFILTER" )
        return Project13AudioProcessor::DSP_Option::LadderFilter;
    if( name == "GEN FILTER" )
        return Project13AudioProcessor::DSP_Option::GeneralFilter;
    
    return Project13AudioProcessor::DSP_Option::END_OF_LIST;
}
//==============================================================================
HorizontalConstrainer::HorizontalConstrainer(std::function<juce::Rectangle<int>()> confinerBoundsGetter, 
                                             std::function<juce::Rectangle<int>()> confineeBoundsGetter) :
boundsToConfineToGetter(std::move(confinerBoundsGetter)),
boundsOfConfineeGetter(std::move(confineeBoundsGetter))
{
    
}

void HorizontalConstrainer::checkBounds (juce::Rectangle<int>& bounds,
                                         const juce::Rectangle<int>& previousBounds,
                                         const juce::Rectangle<int>& limits,
                                         bool isStretchingTop,
                                         bool isStretchingLeft,
                                         bool isStretchingBottom,
                                         bool isStretchingRight)
{
    /*
     'bounds' is the bounding box that we are TRYING to set componentToConfine to.
     we only want to support horizontal dragging within the TabButtonBar.
     
     so, retain the existing Y position given to the TabBarButton by the TabbedButtonBar when the button was created.
     */
    bounds.setY( previousBounds.getY() );
    /*
     the X position needs to be limited to the left and right side of the owning TabbedButtonBar.
     however, to prevent the right side of the TabBarButton from being dragged outside the bounds of the TabbedButtonBar, we must subtract the width of this button from the right side of the TabbedButtonBar
     
     in order for this to work, we need to know the bounds of both the TabbedButtonBar and the TabBarButton.
     hence, loose coupling using lambda getter functions via the constructor parameters.
     Loose coupling is preferred vs tight coupling.
     */
    
    if( boundsToConfineToGetter != nullptr &&
       boundsOfConfineeGetter != nullptr )
    {
        auto boundsToConfineTo = boundsToConfineToGetter();
        auto boundsOfConfinee = boundsOfConfineeGetter();
        
        bounds.setX( juce::jlimit(boundsToConfineTo.getX(),
                                  boundsToConfineTo.getRight() - boundsOfConfinee.getWidth(),
                                  bounds.getX()));
    }
    else
    {
        bounds.setX(juce::jlimit(limits.getX(),
                                 limits.getY(),
                                 bounds.getX()));
    }
}
//==============================================================================
ExtendedTabBarButton::ExtendedTabBarButton(const juce::String& name,
                                           juce::TabbedButtonBar& owner,
                                           Project13AudioProcessor::DSP_Option dspOption) :
juce::TabBarButton(name, owner),
option(dspOption)
{
    constrainer = std::make_unique<HorizontalConstrainer>([&owner](){ return owner.getLocalBounds(); },
                                                          [this](){ return getBounds(); });
    constrainer->setMinimumOnscreenAmounts(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);
}

int ExtendedTabBarButton::getBestTabLength(int depth)
{
    auto bestWidth = getLookAndFeel().getTabButtonBestWidth(*this, depth);
    
    auto& bar = getTabbedButtonBar();
    /*
     we want the tabs to occupy the entire TabBar width.
     so, after computing the best width for the button and depth,
     we choose whichever value is bigger, the bestWidth, or an equal division of the bar's width based on the number of tabs in the bar.
     */
    return juce::jmax(bestWidth,
                      bar.getWidth() / bar.getNumTabs());
}

void ExtendedTabBarButton::mouseDown (const juce::MouseEvent& e)
{
    toFront(true);
    dragger.startDraggingComponent (this, e);
    juce::TabBarButton::mouseDown(e);
}

void ExtendedTabBarButton::mouseDrag (const juce::MouseEvent& e)
{
    dragger.dragComponent (this, e, constrainer.get());
}
//==============================================================================
ExtendedTabbedButtonBar::ExtendedTabbedButtonBar() : juce::TabbedButtonBar(juce::TabbedButtonBar::Orientation::TabsAtTop) 
{
    auto img = juce::Image(juce::Image::PixelFormat::SingleChannel, 1, 1, true);
    auto gfx = juce::Graphics(img);
    gfx.fillAll(juce::Colours::transparentBlack);
    
    dragImage = juce::ScaledImage(img, 1.0);
}

bool ExtendedTabbedButtonBar::isInterestedInDragSource (const SourceDetails& dragSourceDetails) 
{
    if( dynamic_cast<ExtendedTabBarButton*>( dragSourceDetails.sourceComponent.get() ) )
        return true;
    
    return false;
}

void ExtendedTabbedButtonBar::itemDragEnter(const SourceDetails &dragSourceDetails)
{
    DBG("ExtendedTabbedButtonBar::itemDragEnter");
    juce::DragAndDropTarget::itemDragEnter(dragSourceDetails);
}

struct Comparator
{
    /*
     This will use a comparator object to sort the elements into order. The object
     passed must have a method of the form:
     @code
     int compareElements (ElementType first, ElementType second);
     @endcode

     ..and this method must return:
       - a value of < 0 if the first comes before the second
       - a value of 0 if the two objects are equivalent
       - a value of > 0 if the second comes before the first

     To improve performance, the compareElements() method can be declared as static or const.
     */
    static int compareElements( juce::TabBarButton* first, juce::TabBarButton* second )
    {
        if( first->getX() < second->getX() )
            return -1;
        if( first->getX() == second->getX() )
            return 0;
        
        return 1;
    }
};

juce::Array<juce::TabBarButton*> ExtendedTabbedButtonBar::getTabs()
{
    auto numTabs = getNumTabs();
    auto interalTabs = juce::Array<juce::TabBarButton*>();
    interalTabs.resize(numTabs);
    for( int i = 0; i < numTabs; ++i )
    {
        interalTabs.getReference(i) = getTabButton(i);
    }
    
//    auto unsorted = interalTabs;
//    Comparator comparator;
//    interalTabs.sort(comparator);
    
//    if( tabs != unsorted )
//        jassertfalse;
    
    return interalTabs;
}

int ExtendedTabbedButtonBar::findDraggedItemIndex(const SourceDetails &dragSourceDetails)
{
    if( auto tabButtonBeingDragged = dynamic_cast<ExtendedTabBarButton*>( dragSourceDetails.sourceComponent.get() ) )
    {
        auto tabs = getTabs();
        
        auto idx = tabs.indexOf(tabButtonBeingDragged);
        return idx;
    }
    
    return -1;
}

juce::TabBarButton* ExtendedTabbedButtonBar::findDraggedItem(const SourceDetails &dragSourceDetails)
{
    return getTabButton( findDraggedItemIndex(dragSourceDetails) );
}

void ExtendedTabbedButtonBar::itemDragMove(const SourceDetails &dragSourceDetails)
{
    if( auto tabButtonBeingDragged = dynamic_cast<ExtendedTabBarButton*>( dragSourceDetails.sourceComponent.get() ) )
    {
        /*
         The previous system for switching the tab order didn't work completely.
         This is a revision that works correctly.
         It depends on a separate array existing that takes a snapshot of the tab order prior to the drag events starting.
         It also uses new rules to govern how tabs should be reordered, which is based on transitioning over the edge of an adjacent tab.
         here's how it works:
         the mouseDown function takes a snapshot of the tab order, updating the ExtendedTabbsButtonBar::tabs array.
         itemDragMove re-orders this array and reposition the tabs manually.
         the itemDropped() function is where the calls to moveTab() actually occur, which updates the TabbedButtonBar internal list of tabs.
         
         Here are the new rules for how tab movement should be handled
         if the center of the dragged tab transitions from < nextTab.X to >= nextTab.x (from left to right)
            swap(idx, nextTabIndex)
            This handles dragging the tab from left to right.
         if the center of the dragged tab transitions from > previousTab.Right to <= previousTab.right (from right to left)
            swap(idx, previousTabIndex)
            this handles dragging the tab from right to left.
         */
        
        auto idx = tabs.indexOf(tabButtonBeingDragged);
        if( idx == -1 )
        {
            DBG("failed to find tab being dragged in list of tabs");
            jassertfalse;
            return;
        }
        
        auto previousTabIndex = idx - 1;
        auto nextTabIndex = idx + 1;
        auto previousTab = juce::isPositiveAndBelow(previousTabIndex, tabs.size()) ?
                            tabs[previousTabIndex] :
                            nullptr;
        auto nextTab = juce::isPositiveAndBelow(nextTabIndex, tabs.size()) ? tabs[nextTabIndex] : nullptr;
        
        auto centerX = tabButtonBeingDragged->getBounds().getCentreX();
        
        if( centerX > previousDraggedTabCenterPosition.x )
        {
            //transitioning right.
            if( nextTab != nullptr )
            {
                if( previousDraggedTabCenterPosition.x < nextTab->getX() && nextTab->getX() <= centerX )
                {
                    DBG( "swapping [" << idx << "] " << tabButtonBeingDragged->getName() << " with [" << nextTabIndex << "] " << nextTab->getName() );
                    nextTab->setBounds(nextTab->getBounds().withX(previousTab != nullptr ?
                                                                  previousTab->getRight() + 1 :
                                                                  0));
                    tabs.swap(idx, nextTabIndex);
                }
            }
        }
        else if( centerX < previousDraggedTabCenterPosition.x )
        {
            //transitioning left
            if( previousTab != nullptr )
            {
                if( previousDraggedTabCenterPosition.x > previousTab->getRight() && centerX <= previousTab->getRight() )
                {
                    DBG( "swapping [" << idx << "] " << tabButtonBeingDragged->getName() << " with [" << previousTabIndex << "] " << previousTab->getName() );
                    
                    previousTab->setBounds(previousTab->getBounds().withX(nextTab != nullptr ?
                                                                          nextTab->getX() - previousTab->getWidth() - 1 :
                                                                          getWidth() - previousTab->getWidth() - 1));
                    tabs.swap(idx, previousTabIndex);
                }
            }
        }

        tabButtonBeingDragged->toFront(true);
        
        previousDraggedTabCenterPosition = tabButtonBeingDragged->getBounds().getCentre();
    }
}

void ExtendedTabbedButtonBar::itemDragExit(const SourceDetails &dragSourceDetails)
{
    DBG("ExtendedTabbedButtonBar::itemDragExit");
    juce::DragAndDropTarget::itemDragExit(dragSourceDetails);
}

bool ExtendedTabbedButtonBar::reorderTabsAfterDrop()
{
    bool tabOrderChanged = false;
#define DEBUG_TAB_ORDER true
#if DEBUG_TAB_ORDER
    DBG( "starting tab order: ");
    for( auto t : tabs )
    {
        DBG( "  " << t->getName() );
    }
#endif
    while( true )
    {
        auto internalTabs = getTabs();
        if( internalTabs == tabs )
            break;
        
        for( int i = 0; i < tabs.size(); ++i )
        {
            auto t = tabs[i];
            auto location = internalTabs.indexOf(t);
            if( i != location )
            {
#if DEBUG_TAB_ORDER
                DBG("");
                
                DBG(i << ":   internal order: " );
                for( auto t : internalTabs )
                {
                    DBG( "      " << t->getName() );
                }
#endif
                moveTab(location, i);
                tabOrderChanged = true;
                break;
            }
        }
    } //end while(true) loop
    
    return tabOrderChanged;
}

void ExtendedTabbedButtonBar::itemDropped (const SourceDetails& dragSourceDetails) 
{
    DBG( "item dropped" );
    //find the dropped item.  lock the position in.
    if( reorderTabsAfterDrop() == false )
    {
        /*
         if you didn't reorder the tabs but just dragged the tab, you still need to lock the tab position of the tab you dropped.
         resized() will do that for us.
         */
        resized();
    }
//    resized();
    
    //notify of the new tab order
    auto tabs = getTabs();
    Project13AudioProcessor::DSP_Order newOrder;
    
    jassert(tabs.size() == newOrder.size());
    for( size_t i = 0; i < tabs.size(); ++i )
    {
        auto tab = tabs[ static_cast<int>(i) ];
        if( auto* etbb = dynamic_cast<ExtendedTabBarButton*>(tab) )
        {
            newOrder[i] = etbb->getOption();
        }
    }
    
    listeners.call([newOrder](Listener& l)
    {
        l.tabOrderChanged(newOrder);
    });
    
}

void ExtendedTabbedButtonBar::mouseDown(const juce::MouseEvent& e)
{
    DBG( "ExtendedTabbedButtonBar::mouseDown");
    if( auto tabButtonBeingDragged = dynamic_cast<ExtendedTabBarButton*>( e.originalComponent ) )
    {
        tabs = getTabs();
        auto idx = tabs.indexOf(tabButtonBeingDragged);
        if( idx != -1 )
        {
            setCurrentTabIndex(idx);
            setTabColours();
        }
        
        startDragging(tabButtonBeingDragged->TabBarButton::getTitle(),
                      tabButtonBeingDragged,
                      dragImage);
    }
}

/*
 In order to change the color of the selected tab, a few things needed to be investigated:
 1) how is the button painted?
 2) where is the color set?
 
 1) was answered by looking in juce_TabbedButtonBar to look for a `TabbedBarButton::paint` function.
    one does not exist.
    instead a paintButton() function exists.
    Internally this paintButton() function used a LookAndFeel function to draw the button.
 Now we can investigate the LookAndFeel function to see how the button is painted and filled with whatever color it is filled with.
 that leads to this line in the LNF implementation:
 
 const Colour bkg (button.getTabBackgroundColour());
 
 This lets us begin to answer #2:
 2) Where is the color set?
 Here are the steps to answer that and see how we can leverage this value:
 
 - figure out where the color being used to paint the button was being set. answer: TabbedButtonBar::addTab() parameter
 - figure out where that color is being stored: answer: on the TabInfo itself, which owns the button
 - figure out how to change that color: Answer: yes, via  TabbedButtonBar::setTabBackgroundColour()
 - figure out if changing that color during the mousedown event would suffice: answer: yes
 - figure out why that color isn't shown when the plugin first launches: answer: color is only changed when mouse-down occurs.  solution is to refactor code that changes color into a function and call it during mousedown AND when tabs are created/added

 */

void ExtendedTabbedButtonBar::setTabColours()
{
    auto tabs = getTabs();
    for( int i = 0; i < tabs.size(); ++i )
    {
        auto color = tabs[i]->isFrontTab() ? juce::Colours::skyblue : juce::Colours::darkgrey;
        setTabBackgroundColour(i, color);
        tabs[i]->repaint();
    }
}

struct PowerButtonWithParam : PowerButton
{
    PowerButtonWithParam(juce::RangedAudioParameter* p);
    void changeAttachment(juce::RangedAudioParameter* p);
private:
    std::unique_ptr<juce::ButtonParameterAttachment> attachment;
};

PowerButtonWithParam::PowerButtonWithParam(juce::RangedAudioParameter* p)
{
    jassert(p != nullptr);
    changeAttachment(p);
}

void PowerButtonWithParam::changeAttachment(juce::RangedAudioParameter* p)
{
    attachment.reset();
    if( p != nullptr )
    {
        attachment = std::make_unique<juce::ButtonParameterAttachment>(*p, *this);
        attachment->sendInitialUpdate();
    }
}

juce::TabBarButton* ExtendedTabbedButtonBar::createTabButton (const juce::String& tabName, int tabIndex)
{
    auto dspOption = getDSPOptionFromName(tabName);
    auto etbb = std::make_unique<ExtendedTabBarButton>(tabName, *this, dspOption);
    etbb->addMouseListener(this, false);
    
    return etbb.release();
}

void ExtendedTabbedButtonBar::addListener(Listener *l)
{
    listeners.add(l);
}

void ExtendedTabbedButtonBar::removeListener(Listener *l)
{
    listeners.remove(l);
}

void ExtendedTabbedButtonBar::currentTabChanged (int newCurrentTabIndex,
                                                 const juce::String& newCurrentTabName)
{
    juce::ignoreUnused(newCurrentTabName);
    listeners.call([newCurrentTabIndex](Listener& l)
                   {
                       l.selectedTabChanged(newCurrentTabIndex);
                   });
}

//==============================================================================
DSP_Gui::DSP_Gui(Project13AudioProcessor& proc) : processor(proc)
{
    
}

void DSP_Gui::resized()
{
    auto bounds = getLocalBounds();
    if( buttons.empty() == false )
    {
        auto buttonArea = bounds.removeFromTop(30);
        auto w = buttonArea.getWidth() / buttons.size();
        for( auto& button : buttons )
        {
            button->setBounds(buttonArea.removeFromLeft( static_cast<int>(w) ));
        }
    }
    
    if( comboBoxes.empty() == false )
    {
        auto comboBoxArea = bounds.removeFromLeft(bounds.getWidth() / 4);
        auto h = juce::jmin(comboBoxArea.getHeight() / static_cast<int>(comboBoxes.size()), 30);
        for( auto& cb : comboBoxes )
        {
            cb->setBounds(comboBoxArea.removeFromTop( static_cast<int>(h) ));
        }
    }
    
    if( sliders.empty() == false )
    {
        auto w = bounds.getWidth() / sliders.size();
        for( auto& slider : sliders )
        {
            slider->setBounds(bounds.removeFromLeft( static_cast<int>(w) ));
        }
    }
}

void DSP_Gui::paint( juce::Graphics& g )
{
    g.fillAll(juce::Colours::black);
}

void DSP_Gui::rebuildInterface( std::vector< juce::RangedAudioParameter* > params )
{
    if( params == currentParams )
    {
        DBG( "interface didn't change");
        return;
    }
    
    DBG( "interface changed" );
    currentParams = params;
    
    sliderAttachments.clear();
    comboBoxAttachments.clear();
    buttonAttachments.clear();
    
    sliders.clear();
    comboBoxes.clear();
    buttons.clear();
    
    for( size_t i = 0; i < params.size(); ++i )
    {
        auto p = params[i];
        
        if( dynamic_cast<juce::AudioParameterBool*>(p) )
        {
            DBG( "skipping button attachments" );
        }
        else
        {
            //sliders are used for float and choice params
            sliders.push_back(std::make_unique<RotarySliderWithLabels>(p, p->label, p->getName(100)));
            auto& slider = *sliders.back();
            SimpleMBComp::addLabelPairs(slider.labels, *p, p->label);
            slider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
            sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, p->getName(100), slider));
        }
        
#if false
        if( auto* choice = dynamic_cast<juce::AudioParameterChoice*>(p) )
        {
            //make a combobox
            comboBoxes.push_back( std::make_unique<juce::ComboBox>());
            auto& cb = *comboBoxes.back();
            cb.addItemList(choice->choices, 1);
            comboBoxAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, p->getName(100), cb));
        }
        else if( auto* toggle = dynamic_cast<juce::AudioParameterBool*>(p) )
        {
            //make a toggle button
//            buttons.push_back(std::make_unique<juce::ToggleButton>("Bypass"));
//            auto& btn = *buttons.back();
//            buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, p->getName(100), btn));
            DBG( "DSP_Gui::rebuild interface skipping APVTS::ButtonAttachment for AudioParameterBool: " << p->getName(100));
        }
        else
        {
            //it's a float or int param make a slider
            sliders.push_back(std::make_unique<RotarySliderWithLabels>(p, p->label, p->getName(100)));
            auto& slider = *sliders.back();
            SimpleMBComp::addLabelPairs(slider.labels, *p, p->label);
            slider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
            sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, p->getName(100), slider));
        }
#endif
    }
    
    for( auto& slider : sliders )
        addAndMakeVisible(slider.get());
    for( auto& cb : comboBoxes)
        addAndMakeVisible(cb.get());
    for( auto& btn : buttons )
        addAndMakeVisible(btn.get());
    
    resized();
}
//==============================================================================
Project13AudioProcessorEditor::Project13AudioProcessorEditor (Project13AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setLookAndFeel(&lookAndFeel);
    addAndMakeVisible(tabbedComponent);
    addAndMakeVisible(dspGUI);
    
    inGainControl = std::make_unique<RotarySliderWithLabels>(audioProcessor.inputGain, "dB", "IN");
    outGainControl = std::make_unique<RotarySliderWithLabels>(audioProcessor.outputGain, "dB", "OUT");
    
    addAndMakeVisible(inGainControl.get());
    addAndMakeVisible(outGainControl.get());
    
    SimpleMBComp::addLabelPairs(inGainControl->labels, *audioProcessor.inputGain, "dB");
    SimpleMBComp::addLabelPairs(outGainControl->labels, *audioProcessor.outputGain, "dB");
    
    inGainAttachment = std::make_unique<juce::SliderParameterAttachment>(*audioProcessor.inputGain, *inGainControl);
        
    outGainAttachment =  std::make_unique<juce::SliderParameterAttachment>(*audioProcessor.outputGain, *outGainControl);
    
    audioProcessor.guiNeedsLatestDspOrder.set(true);
    
    tabbedComponent.addListener(this);
    startTimerHz(30);
    setSize (768, 450 + ioControlSize);
}

Project13AudioProcessorEditor::~Project13AudioProcessorEditor()
{
    setLookAndFeel(nullptr);
    tabbedComponent.removeListener(this);
}

//==============================================================================
void Project13AudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    auto fillMeter = [&](auto rect, const auto& rmsSource)
    {
        g.setColour(juce::Colours::black);
        g.fillRect(rect);
        
        auto rms = rmsSource.get();
        if( rms > 1.f )
        {
            g.setColour(juce::Colours::red);
            auto lowerLeft = juce::Point<float>(rect.getX(),
                                                juce::jmap<float>(juce::Decibels::gainToDecibels(1.f),
                                                                  NEGATIVE_INFINITY,
                                                                  MAX_DECIBELS,
                                                                  rect.getBottom(),
                                                                  rect.getY()));
            auto upperRight = juce::Point<float>(rect.getRight(),
                                                 juce::jmap<float>(juce::Decibels::gainToDecibels(rms),
                                                                   NEGATIVE_INFINITY,
                                                                   MAX_DECIBELS,
                                                                   rect.getBottom(),
                                                                   rect.getY()));
            
            auto overThreshRect = juce::Rectangle<float>(lowerLeft, upperRight);
            g.fillRect(overThreshRect);
        }
        
        rms = juce::jmin<float>(rms, 1.f);
//        DBG( "rms: " << rms );
        g.setColour(juce::Colours::green);
        g.fillRect(rect
                   .withY(juce::jmap<float>(juce::Decibels::gainToDecibels(rms),
                                            NEGATIVE_INFINITY,
                                            MAX_DECIBELS,
                                            rect.getBottom(),
                                            rect.getY()))
                   .withBottom(rect.getBottom()));
    };
    
    auto drawTicks = [&](auto rect, auto leftMeterRightEdge, auto rightMeterLeftEdge)
    {
        for( int i = MAX_DECIBELS; i >= NEGATIVE_INFINITY; i -= 12 )
        {
            auto y = juce::jmap<int>(i, NEGATIVE_INFINITY, MAX_DECIBELS, rect.getBottom(), rect.getY());
            auto r = juce::Rectangle<int>(rect.getWidth(), fontHeight);
            r.setCentre(rect.getCentreX(), y);
            
            g.setColour(i == 0 ? juce::Colours::white :
                        i > 0 ? juce::Colours::red :
                        juce::Colours::lightsteelblue );
            g.drawFittedText(juce::String(i), r, juce::Justification::centred, 1);
            
            if( i != MAX_DECIBELS && i != NEGATIVE_INFINITY )
            {
                g.drawLine(rect.getX() + tickIndent, y, leftMeterRightEdge - tickIndent, y);
                g.drawLine(rightMeterLeftEdge + tickIndent, y, rect.getRight() - tickIndent, y);
            }
        }
    };
    
    /*
     this lambda draws the label then computes the rectangles that form each individual channel
     then it draws the meters for each computed rectangle
     then it draws the ticks
     */
    auto drawMeter = [&fillMeter, &drawTicks](juce::Rectangle<int> rect,
                                              juce::Graphics& g,
                                              const juce::Atomic<float>& leftSource,
                                              const juce::Atomic<float>& rightSource,
                                              const juce::String& label)
    {
        g.setColour(juce::Colours::green);
        g.drawRect(rect);
        rect.reduce(2,2);
        
        g.setColour(juce::Colours::white);
        g.drawText(label, rect.removeFromBottom(fontHeight), juce::Justification::centred);
        
        rect.removeFromTop(fontHeight / 2);
        
        const auto meterArea = rect;
        const auto leftChan = rect.removeFromLeft(meterChanWidth);
        const auto rightChan = rect.removeFromRight(meterChanWidth);
        
        fillMeter(leftChan, leftSource);
        fillMeter(rightChan, rightSource);
        drawTicks(meterArea, leftChan.getRight(), rightChan.getX());
    };
    
    auto bounds = getLocalBounds();
    bounds.removeFromBottom(ioControlSize);
    
    auto preMeterArea = bounds.removeFromLeft(meterWidth);
    auto postMeterArea = bounds.removeFromRight(meterWidth);
    
    drawMeter(preMeterArea,
              g,
              audioProcessor.leftPreRMS,
              audioProcessor.rightPreRMS,
              "In");
    drawMeter(postMeterArea,
              g,
              audioProcessor.leftPostRMS,
              audioProcessor.rightPostRMS,
              "Out");
}

void Project13AudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    
    auto bounds = getLocalBounds();
    auto gainArea = bounds.removeFromBottom(ioControlSize);
    inGainControl->setBounds(gainArea.removeFromLeft(ioControlSize));
    outGainControl->setBounds(gainArea.removeFromRight(ioControlSize));
    
    auto leftMeterArea = bounds.removeFromLeft(meterWidth);
    auto rightMeterArea = bounds.removeFromRight(meterWidth);
    juce::ignoreUnused(leftMeterArea, rightMeterArea);
    tabbedComponent.setBounds(bounds.removeFromTop(30));
    dspGUI.setBounds( bounds );
}

void Project13AudioProcessorEditor::tabOrderChanged(Project13AudioProcessor::DSP_Order newOrder)
{
    rebuildInterface();
    audioProcessor.dspOrderFifo.push(newOrder);
}

void Project13AudioProcessorEditor::timerCallback()
{
    repaint();
    
    if( audioProcessor.restoreDspOrderFifo.getNumAvailableForReading() == 0 )
        return;
    
    using T = Project13AudioProcessor::DSP_Order;
    T newOrder;
    newOrder.fill(Project13AudioProcessor::DSP_Option::END_OF_LIST);
    auto empty = newOrder;
    while( audioProcessor.restoreDspOrderFifo.pull(newOrder) )
    {
        ; //do nothing   you'll do something with the most recently pulled order next.
    }
    
    if( newOrder != empty ) //if you pulled nothing, neworder will be filled with END_OF_LIST
    {
        //don't create tabs if neworder is filled with END_OF_LIST
        addTabsFromDSPOrder(newOrder);
    }
    
    if( selectedTabAttachment == nullptr )
    {
        selectedTabAttachment = std::make_unique<juce::ParameterAttachment>(*audioProcessor.selectedTab, 
                                                                            [this](float tabNum)
        {
            auto newTabNum = static_cast<int>(tabNum);
            if( juce::isPositiveAndBelow(newTabNum, tabbedComponent.getNumTabs()))
            {
                tabbedComponent.setCurrentTabIndex(newTabNum);
            }
            else
            {
                jassertfalse;
            }
        });
        
        selectedTabAttachment->sendInitialUpdate();
    }
    
}

void Project13AudioProcessorEditor::addTabsFromDSPOrder(Project13AudioProcessor::DSP_Order newOrder)
{
    tabbedComponent.clearTabs();
    for( auto v : newOrder )
    {
        tabbedComponent.addTab(getNameFromDSPOption(v), juce::Colours::white, -1);
    }
    
    /*
     Bypass buttons are added to the tabs AFTER they have been created and added to the tabbed component.
     The mechanism used is kind of ugly
     the DSP order is used to retrieve the params for a DSP_Option.
     then the params are searched. We are looking for an AudioParameterBool instance.
     then the found AudioParameterBool is checked to see if the param name contains "bypass".
     If those two conditions are met, the bypass param has been found.
     now the button can be created, configured, and added to the tab as an extra component.
     */
    
    auto numTabs = tabbedComponent.getNumTabs();
    auto size = tabbedComponent.getHeight();
    for( int i = 0; i < numTabs; ++i )
    {
        if( auto tab = tabbedComponent.getTabButton(i) )
        {
            auto order = newOrder[i];
            auto params = audioProcessor.getParamsForOption(order);
            for( auto p : params )
            {
                if( auto bypass = dynamic_cast<juce::AudioParameterBool*>(p))
                {
                    if( bypass->name.containsIgnoreCase("bypass") )
                    {
                        auto pbwp = std::make_unique<PowerButtonWithParam>(bypass);
                        pbwp->setSize(size, size);
                        tab->setExtraComponent(pbwp.release(), juce::TabBarButton::ExtraComponentPlacement::beforeText);
                    }
                }
            }
        }
    }
    
    tabbedComponent.setTabColours();
    rebuildInterface();
    //if the order is identical to the current order used by the audio side, this push will do nothing.
    audioProcessor.dspOrderFifo.push(newOrder);
}

void Project13AudioProcessorEditor::rebuildInterface()
{
    auto currentTabIndex = tabbedComponent.getCurrentTabIndex();
    auto currentTab = tabbedComponent.getTabButton(currentTabIndex);
    if( auto etbb = dynamic_cast<ExtendedTabBarButton*>(currentTab))
    {
        auto option = etbb->getOption();
        auto params = audioProcessor.getParamsForOption(option);
        jassert(params.empty() == false);
        dspGUI.rebuildInterface(params);
    }
}

void Project13AudioProcessorEditor::selectedTabChanged(int newCurrentTabIndex)
{
    /*
     Selected Tab restoration requires a ParamAttachment to exist.
     the attachment is also in charge of setting the Selected Tab parameter when the user clicks on a tab.
     when the tab is changed, the interface is also rebuilt.
     when the audio parameter settings are loaded from disk, the callback for the parameter attachment is called.
     this callback changes the selected tab and rebuilds the interface.
     the creation of the attachment can't happen until after tabs have been created.
     tabs are created in TimerCallback whenever the restoreDspOrderFifo has a DSP_Order instance to pull.
     This is why the attachment creation is not in the constructor, but is instead in timerCallback(), after it is determined that the restoreDspOrderFifo has items to pull.
     */
    
    if( selectedTabAttachment )
    {
        rebuildInterface();
        /*
         When the synth first launches, this callback is triggered from the `selectedTabAttachment->sendInitialUpdate()` call in `timerCallback()`
         every time the currentTabIndex changes, we want to refresh the tab colors.
         Previously, we were changing tab colours from addTabsFromDSPOrder() and when the tab is clicked.
         For some reason, addTabsFromDSPOrder() wasn't changing the tab color.
         calling it here ensures that the tab color changes.
         */
        tabbedComponent.setTabColours();
        selectedTabAttachment->setValueAsCompleteGesture(static_cast<float>(newCurrentTabIndex));
    }
}
