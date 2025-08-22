# New Layer Type

This is a little hacking guide about how to add a new kind of layer to Aseprite:

1. The first step is to add a new [`doc::ObjectType` value](../src/doc/object_type.h)
   for the new kind of layer
1. Create a new `LayerNAME` class derived from `Layer` or `LayerImage` (e.g. [`LayerAudio`](../src/doc/layer_audio.h))
1. Add this new `LayerNAME` [in `Layer::Layer()`'s `ASSERT()`](../src/doc/layer.cpp)
1. Expand [the `type` parameter in `NewLayerParams` for
   `NewLayerCommand`](../src/app/commands/cmd_new_layer.cpp) and
   `NewLayerCommand::Type` for `LayerNAME`
1. Add the necessary code in `NewLayerCommand::onExecute()` to create
   this new kind of layer with a transaction
1. Complete `NewLayerCommand::layerPrefix()` function (which is used
   in `NewLayerCommand::onGetFriendlyName()`) to get a proper text for
   this command when it creates this new kind of layer, you will need
   some [new `NewLayer_*Layer` string in
   `en.ini`](../data/strings/en.ini)
1. Add a [new menu option in `gui.xml`](../data/gui.xml) in `<menu
   text="@.layer" id="layer_menu">` for a new *Layer > New* menu,
   you will need some [new `layer_new_*_layer` string in
   `en.ini`](../data/strings/en.ini) in the `[main_menu]` section
1. Add the serialization of the layer specifics in
   [`write_layer`/`read_layer` functions](../src/doc/layer_io.cpp),
   this code is used for [undo/redo](../src/app/cmd/add_layer.cpp)
1. Add the serialization for [data recovery](https://www.aseprite.org/docs/data-recovery/)
   purposes of the layer specifics in [Writer::writeLayerStructure()](../src/app/crash/write_document.cpp)
   and [Reader::readLayer()](../src/app/crash/read_document.cpp) functions
1. To make `DuplicateLayerCommand` work, add the new `doc::ObjectType`
   in `app::DocApi::copyLayerWithSprite()` and `app::Doc::copyLayerContent()`
