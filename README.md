# Custom SDL3 Window

This is a proof of concept for an SDL3 window with a self-drawn non-client area and drop shadow with alpha blending.

The window can be moved and resized using hit tests, and it reacts dynamically to changes in the system theme (light or dark mode) and fractional display scaling for high-pixel-density screens.

## Restrictions

The window doesn't know when it's next to the screen edges yet, so it doesn't hide the shadow there.
