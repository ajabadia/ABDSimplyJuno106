#pragma once
#include <JuceHeader.h>

namespace JunoUI
{
    // Authentic Juno-106 Palette
    const juce::Colour kJunoRed = juce::Colour::fromFloatRGBA(0.756f, 0.153f, 0.176f, 1.0f);    // #C1272D
    const juce::Colour kJunoDarkGrey = juce::Colour(0xff232323);
    const juce::Colour kPanelBackground = juce::Colour(0xff333333);
    const juce::Colour kAccentBlue = juce::Colour(0xff3399FF);
    const juce::Colour kAccentRed = juce::Colour(0xffCC0000);
    const juce::Colour kTextWhite = juce::Colours::white;
    const juce::Colour kTextGrey = juce::Colour(0xffcccccc);

    /**
     * MidiLearnMouseListener - Handles right-click MIDI Learn on components
     */
    class MidiLearnMouseListener : public juce::MouseListener
    {
    public:
        MidiLearnMouseListener(MidiLearnHandler& h, const juce::String& pId)
            : handler(h), paramID(pId) {}

        void mouseDown(const juce::MouseEvent& event) override
        {
            if (event.mods.isRightButtonDown())
            {
                juce::PopupMenu menu;
                
                int currentCC = handler.getCCForParam(paramID);
                juce::String ccText = (currentCC >= 0) ? " (Mapped to CC " + juce::String(currentCC) + ")" : " (Unmapped)";
                
                menu.addItem(1, "MIDI Learn" + ccText);
                menu.addItem(2, "Clear MIDI Mapping", currentCC >= 0);

                menu.showMenuAsync(juce::PopupMenu::Options(), [this](int result)
                {
                    if (result == 1) handler.startLearning(paramID);
                    else if (result == 2) handler.bind(-1, paramID); // Unbind
                });
            }
        }

    private:
        MidiLearnHandler& handler;
        juce::String paramID;
    };

    /**
     * JunoLookAndFeel - Custom skin for authentic hardware look
     */
    class JunoLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        JunoLookAndFeel()
        {
            setColour(juce::Slider::thumbColourId, juce::Colours::silver);
            setColour(juce::Slider::trackColourId, juce::Colours::black);
            setColour(juce::TextButton::buttonColourId, juce::Colour(0xff444444));
            setColour(juce::ToggleButton::tickColourId, juce::Colours::orange);
        }

        void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                              float sliderPos, float minSliderPos, float maxSliderPos,
                              const juce::Slider::SliderStyle style, juce::Slider& slider) override
        {
            if (slider.getProperties().contains("isSwitch"))
            {
                drawJunoSwitch(g, x, y, width, height, sliderPos, slider);
                return;
            }

            juce::ignoreUnused(minSliderPos, maxSliderPos, style);
            auto isVertical = slider.isVertical();
            auto trackWidth = 4.0f;
            auto thumbWidth = 20.0f;
            auto thumbHeight = 35.0f;

            // Draw Ticks and Numbers (Authentic Juno Style)
            if (isVertical)
            {
                // Authentic Light Grey color (derived from background but brighter)
                g.setColour(juce::Colour(0xffa0a0a0)); // Light grey
                g.setFont(juce::FontOptions(10.0f, juce::Font::bold));

                // Determine Number Sides: "Left", "Right", "Both", "None"
                // Default to "Both" if not set? Or "Right"?
                // Let's default to "None" if unset to verify? No, let's default to "Right" as fail-safe or "Both".
                // Based on standard JUCE, properties are empty by default.
                juce::String side = slider.getProperties().getWithDefault("numberSide", "Both");

                int numTicks = 11; // 0 to 10
                
                // Exact Thumb Travel Calculation
                // The slider value maps to the center of the thumb.
                // Min Value (usually bottom) -> y + height - margin
                // Max Value (usually top) -> y + margin
                // We want ticks to align perfectly with these centers.
                
                // thumbHeight is 35. Margin is half of that.
                float margin = thumbHeight * 0.5f;
                float trackTop = (float)y + margin;
                float trackBottom = (float)y + (float)height - margin;
                float trackLen = trackBottom - trackTop;
                
                for (int i = 0; i < numTicks; ++i)
                {
                    // i=0 is Bottom (Value 0), i=10 is Top (Value 10)
                    float yPos = trackBottom - (i * (trackLen / 10.0f));
                    
                    bool isWide = (i == 0 || i == 5 || i == 10);
                    float tickW = isWide ? 26.0f : 12.0f; 
                    
                    // Draw Tick Line (Centered horizontally)
                    float centerX = (float)x + (float)width * 0.5f;
                    g.fillRect(centerX - tickW * 0.5f, yPos, tickW, 1.5f);
                    
                    // Draw Numbers
                    if (isWide)
                    {
                        juce::String numStr = juce::String(i);
                        
                        // Left
                        if (side == "Left" || side == "Both")
                            g.drawText(numStr, (int)(centerX - 25), (int)(yPos - 5), 15, 10, juce::Justification::centredRight);
                        
                        // Right
                        if (side == "Right" || side == "Both")
                            g.drawText(numStr, (int)(centerX + 10), (int)(yPos - 5), 15, 10, juce::Justification::centredLeft);
                    }
                }
            }

            // Draw Track
            g.setColour(juce::Colours::black);
            if (isVertical)
                g.fillRect(juce::Rectangle<float>((float)x + ((float)width - trackWidth) * 0.5f, (float)y, trackWidth, (float)height));
            else
                g.fillRect(juce::Rectangle<float>((float)x, (float)y + ((float)height - trackWidth) * 0.5f, (float)width, trackWidth));

            // Draw Thumb (Cap)
            auto thumbRect = isVertical ? 
                juce::Rectangle<float>((float)x + ((float)width - thumbWidth) * 0.5f, sliderPos - thumbHeight * 0.5f, thumbWidth, thumbHeight) :
                juce::Rectangle<float>(sliderPos - thumbHeight * 0.5f, (float)y + ((float)height - thumbWidth) * 0.5f, thumbHeight, thumbWidth);

            // Silver body
            g.setColour(juce::Colours::silver);
            g.fillRect(thumbRect);
            
            // Accent line on cap
            g.setColour(juce::Colours::black);
            if (isVertical)
                g.fillRect(thumbRect.getX(), thumbRect.getCentreY() - 1.0f, thumbRect.getWidth(), 2.0f);
            else
                g.fillRect(thumbRect.getCentreX() - 1.0f, thumbRect.getY(), 2.0f, thumbRect.getHeight());

            // Outline
            g.setColour(juce::Colours::black.withAlpha(0.6f));
            g.drawRect(thumbRect, 1.0f);
        }

        void drawJunoSwitch(juce::Graphics& g, int x, int y, int width, int height, float sliderPos, juce::Slider& slider)
        {
            // Background slot
            g.setColour(juce::Colours::black);
            auto area = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height);
            auto slot = area.withSizeKeepingCentre(10.0f, area.getHeight() - 10.0f);
            g.fillRoundedRectangle(slot, 5.0f);

            // Switch Handle
            float handleHeight = 20.0f;
            float handleWidth = 24.0f; // Wider than slot
            // Authentic switch is black plastic
            
            auto handleRect = juce::Rectangle<float>(0, 0, handleWidth, handleHeight)
                                .withCentre(juce::Point<float>(area.getCentreX(), sliderPos));
            
            g.setColour(juce::Colour(0xff333333)); // Dark Grey plastic
            g.fillRect(handleRect);
            
            // Highlight
            g.setColour(juce::Colours::white.withAlpha(0.2f));
            g.fillRect(handleRect.removeFromTop(2.0f));

            g.setColour(juce::Colours::black);
            g.drawRect(handleRect, 1.0f);
        }

        void drawToggleButton(juce::Graphics& g, juce::ToggleButton& b,
                              bool /*isMouseOver*/, bool isButtonDown) override
        {
            auto area = b.getLocalBounds().toFloat();
            auto isToggled = b.getToggleState();
            
            // Layout: LED at top (20%), Button at bottom (80%)
            // If button is small, adjust logic?
            // Assuming authentic layout space provided by parent (e.g. 50x60)
            
            float ledSize = 10.0f;
            auto ledArea = area.removeFromTop(area.getHeight() * 0.35f).withSizeKeepingCentre(ledSize, ledSize);
            auto buttonArea = area.reduced(2.0f); // Remaining is button

            // Draw LED
            juce::Colour ledOnColor = b.getProperties().contains("ledColor") 
                                    ? juce::Colour::fromString(b.getProperties()["ledColor"].toString())
                                    : kJunoRed; // Default Red
                                    
            juce::Colour ledColor = isToggled ? ledOnColor : ledOnColor.darker(0.8f);
            
            // Glow
            if (isToggled) {
                g.setColour(ledColor.withAlpha(0.5f));
                g.fillEllipse(ledArea.expanded(4.0f));
                g.setColour(ledColor.withAlpha(0.8f));
                g.fillEllipse(ledArea.expanded(1.0f));
            } else {
                 g.setColour(juce::Colours::black);
                 g.fillEllipse(ledArea);
            }
            
            // Core
            g.setColour(ledColor);
            g.fillEllipse(ledArea);
            
            // Reflection
            g.setColour(juce::Colours::white.withAlpha(0.4f));
            g.fillEllipse(ledArea.removeFromTop(ledSize * 0.4f).removeFromLeft(ledSize * 0.4f).translated(2.0f, 2.0f));

            // Draw Button Cap (Authentic White/Beige)
            juce::Colour capColor = juce::Colour(0xffe0e0d0); // Juno Button White
            if (isButtonDown) capColor = capColor.darker(0.1f);
            
            g.setColour(capColor);
            g.fillRect(buttonArea);
            
            // 3D Bevel
            g.setColour(juce::Colours::white.withAlpha(0.6f));
            g.fillRect(buttonArea.removeFromTop(2.0f));
            g.setColour(juce::Colours::black.withAlpha(0.2f));
            g.fillRect(buttonArea.removeFromBottom(4.0f));

            // Outline
            g.setColour(juce::Colours::black.withAlpha(0.4f));
            g.drawRect(buttonArea, 1.0f);
        }

        void drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour& backgroundColour,
                                  bool isMouseOverButton, bool isButtonDown) override
        {
            auto area = b.getLocalBounds().toFloat();
            auto baseColour = backgroundColour;

            // Authentic Juno Patch Button Style
            if (b.getProperties().contains("isPatchButton"))
            {
                // Juno Blue/Aqua: #66AACC
                auto junoBlue = juce::Colour(0xff66aacc);
                
                if (b.getToggleState()) 
                {
                   baseColour = junoBlue.brighter(0.4f); 
                }
                else
                {
                   baseColour = junoBlue;
                }
                
                if (isButtonDown) baseColour = baseColour.darker(0.1f);
                else if (isMouseOverButton) baseColour = baseColour.brighter(0.1f);
                
                g.setColour(baseColour);
                g.fillRect(area);
                
                // Bevel
                g.setColour(juce::Colours::white.withAlpha(0.3f));
                g.fillRect(area.removeFromTop(3.0f));
                g.setColour(juce::Colours::black.withAlpha(0.2f));
                g.fillRect(area.removeFromBottom(3.0f));
                
                g.setColour(juce::Colours::black.withAlpha(0.6f));
                g.drawRect(area, 1.0f);
            }
            else
            {
                // Default Button (Dark Grey / Black)
                if (isButtonDown) baseColour = baseColour.darker(0.2f);
                else if (isMouseOverButton) baseColour = baseColour.brighter(0.1f);

                g.setColour(baseColour);
                g.fillRect(area);

                g.setColour(juce::Colours::black.withAlpha(0.5f));
                g.drawRect(area, 1.0f);
            }
        }
    };

    // Helper functions
    inline void setupVerticalSlider(juce::Slider& slider)
    {
        slider.setSliderStyle(juce::Slider::LinearVertical);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0); // Headers have labels
    }

    inline void setupRotarySlider(juce::Slider& slider)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    }

    inline void setupLabel(juce::Label& label, const juce::String& text, juce::Component& parent)
    {
        parent.addAndMakeVisible(label);
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        label.setColour(juce::Label::textColourId, kTextGrey);
    }

    /** Attaches MIDI Learn capability to a component */
    inline void setupMidiLearn(juce::Component& comp, MidiLearnHandler& handler, const juce::String& paramID, juce::OwnedArray<MidiLearnMouseListener>& listeners)
    {
        auto* listener = new MidiLearnMouseListener(handler, paramID);
        listeners.add(listener);
        comp.addMouseListener(listener, false);
    }

    inline void drawSectionBackground(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title)
    {
        auto fBounds = bounds.toFloat();
        
        // 1. Main Background with subtle gradient
        juce::ColourGradient grad(kJunoDarkGrey.brighter(0.1f), 0, (float)bounds.getY(),
                                  kJunoDarkGrey, 0, (float)bounds.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRect(fBounds);
        
        // 2. Red Header
        auto headerRect = fBounds.removeFromTop(20.0f);
        g.setColour(kJunoRed);
        g.fillRect(headerRect);
        
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText(title, headerRect, juce::Justification::centred);
        
        // 3. Bottom Accent Bar
        auto footerRect = fBounds.removeFromBottom(3.0f);
        // Alternate blue/red pattern based on title or position?
        // For simplicity, let's use a common logic.
        g.setColour(title.contains("DCO") || title.contains("LFO") ? kAccentBlue : kAccentRed);
        g.fillRect(footerRect);

        // 4. Border
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawRect(bounds.toFloat(), 1.0f);
    }

    /**
     * JunoLCD - Simulates the red 2-digit LED display
     */
    class JunoLCD : public juce::Component
    {
    public:
        void paint(juce::Graphics& g) override
        {
            auto area = getLocalBounds().toFloat();
            
            // Outer frame
            g.setColour(juce::Colours::black);
            g.fillRoundedRectangle(area, 2.0f);
            
            // Inner glow (very dark red)
            auto inner = area.reduced(2.0f);
            g.setColour(juce::Colour(0xff220000));
            g.fillRect(inner);
            
            // Text (Bright Red/Orange)
            g.setColour(juce::Colours::orangered);
            g.setFont(juce::FontOptions(20.0f, juce::Font::bold)); // Use a "segmented" looking font if possible, else bold sans
            g.drawText(text, inner, juce::Justification::centred);
        }

        void setText(const juce::String& newText) { text = newText; repaint(); }

    private:
        juce::String text = "--";
    };
}
