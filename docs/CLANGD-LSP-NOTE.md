# NeoVim & clangd LSP Users Note

After generating a build with CMakeLists -> `EXPORT_COMPILE_COMMANDS ON`, you
will need to symlink the JSON file at the aseprite top-level dir:

```bash
ln -s build/aseprite-release/compile_commands.json compile_commands.json
```

> _Note_: After symlinking, only the _first_ time you edit a src file clangd
> will take quite a while to cache.
