# Jumble Tool Documentation

---
## Overview
The Jumble Tool in Aseprite is designed to creatively alter the pixels within a selected area by mixing them based on cursor movement and a randomized algorithm. This tool can be used to add a subtle disordered effect to the image details, enhancing textures or creating a sense of movement.

## How It Works
The Jumble Tool affects pixels within the radius of the brush tool. It modifies the image by applying two main factors to how pixels are chosen and displaced:

- Random Displacement: For each pixel under the tool's brush area, the tool generates a small random offset between -1 and +1 for both the X and Y directions. This random factor slightly alters where pixels will be picked from.
- Movement Influence: The speed and direction of the mouse cursor influence how far pixels are moved from their original positions. Faster movements lead to greater displacement of pixels, enhancing the jumbling effect.

## Technical Details
The process for each pixel affected by the Jumble Tool is as follows:

Calculate a new position (pt) for the pixel by adding a small random offset to the current cursor position. This offset is adjusted by the speed of the cursor (m_speed.x and m_speed.y), which accounts for the direction and velocity of the mouse movement.
Ensure that the new position remains within the boundaries of the image. If the mode is set to 'tiled,' the coordinates will wrap around the image edges, otherwise, they will be clamped to stay within the image frame.
Retrieve the color from the newly calculated position in the source image, and apply this color to the corresponding position in the destination area where the brush is applied.

## Usage Tips
- Texture Enhancement: Use the Jumble Tool to add complexity or a randomized texture to flat-colored areas.
- Creating Motion Effects: By quickly dragging the tool across areas with detailed textures, you can create a blurred or smeared effect that simulates motion.
- Subtle Changes: For subtle effects, use slower movements and a smaller brush size to finely mix colors and details.
## Conclusion
The Jumble Tool is a powerful feature for artists who want to introduce randomness and a dynamic feel to their artwork in a controlled manner. By understanding and manipulating the factors of randomness and cursor speed, users can achieve a wide range of artistic effects.

