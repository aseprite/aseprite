# App Options

---
## General Usage
``` aseprite [options] [file1.aseprite file2.aseprite ...] ```

Each command line option is prefixed with a double dash (--). Some options require an additional value to be specified after the option name.

## Options
### Mode Control
- --shell: Start an interactive console for executing scripts. (Requires scripting support)
- --batch: Run in batch mode without starting the GUI.
- --preview: Show what would be done without actually executing the actions.
### File Operations
- --save-as <filename>: Save the last given sprite in another format.
- --palette <filename>: Change the palette of the last opened sprite.
- --scale <factor>: Scale all previously opened sprites by the given factor.
### Sprite Sheet Generation
- --sheet <filename.png>: Export the sprite(s) to a sprite sheet image file.
- --sheet-type <type>: Specify the layout of the sprite sheet (horizontal, vertical, rows, columns, packed).
- --sheet-width <pixels>: Define the width of the sprite sheet.
- --sheet-height <pixels>: Define the height of the sprite sheet.
### Layer and Frame Control
- --split-layers: Save each visible layer of sprites as separated images in the sprite sheet.
- --split-tags: Export each tag as a separated file.
- --ignore-layer <name>: Exclude the specified layer from the sprite sheet or save operation.
- --tag <name>: Include only the frames associated with the specified tag in the operations.
### Image Processing
- --trim: Trim empty space around the sprite or frames.
- --crop x,y,width,height: Crop all images to the specified rectangle.
- --extrude: Extrude all images by duplicating all edges by one pixel.
### Debugging and Help
- --verbose: Output detailed information about the operations being performed.
- --debug: Enable detailed debug mode and log output to the desktop.
- --help: Display help information about command-line options and exit.
- --version: Output version information of Aseprite and exit.

### Examples
#### 1. Convert a Sprite to a Different Format:
``` aseprite --batch --save-as output.png input.aseprite ```
#### 2. Generate a Sprite Sheet:
``` aseprite --batch --sheet output.png --sheet-type packed input.aseprite ```
#### 3. Scale and Export a Sprite:
``` aseprite --batch --scale 2 --save-as scaled_output.aseprite input.aseprite ```

## Notes
Ensure that all file paths and names are specified after the options.
