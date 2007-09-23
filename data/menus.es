# allegro-sprite-editor menus				-*- Shell-script -*-
# Copyright (C) 2001-2005, 2007 by David A. Capello
#
# Spanish menus (see menus.en for more information)

- {
  "&Tips" = always "dialogs_tips(true)" <F1> ;
  "&Acerca de" = always "dialogs_about()" ;
  "&Refrescar" = always "app_refresh_screen()" <F5> ;
  ----
  "&Protector de Pantalla" = always "dialogs_screen_saver()" ;
  "Cambiar Modo &Gráfico" {
    "&Seleccionar" = always "set_gfx(\"interactive\", 0, 0, 0)" ;
    ----
    "320x200 8" = always "set_gfx(\"autodetect\", 320, 200, 8)" ;
    "320x240 8" = always "set_gfx(\"autodetect\", 320, 240, 8)" ;
    "640x480 8" = always "set_gfx(\"autodetect\", 640, 480, 8)" ;
    "800x600 8" = always "set_gfx(\"autodetect\", 800, 600, 8)" ;
    ----
    "320x200 16" = always "set_gfx(\"autodetect\", 320, 200, 16)" ;
    "320x240 16" = always "set_gfx(\"autodetect\", 320, 240, 16)" ;
    "640x480 16" = always "set_gfx(\"autodetect\", 640, 480, 16)" ;
    "800x600 16" = always "set_gfx(\"autodetect\", 800, 600, 16)" ;
  }
  ----
  "Screen Sh&ot" SCREENSHOT_ACCEL = always "screen_shot()" <F12> ;
  "G&rabar Pantalla" = is_rec "switch_recording_screen()" ;
  ----
  "Recargar los &Menus" = always "rebuild_root_menu_with_alert()" ;
  ----
  "Cargar Sesión" = always "GUI_LoadSession()" ;
  "Guardar Sesión" = always "GUI_SaveSession()" ;
}

&Archivo {
  "&Nuevo" = always "GUI_NewSprite()" <Ctrl+N> ;
  "&Abrir" = always "GUI_OpenSprite()" <Ctrl+O> <F3> ;
  "Abrir &Reciente" RECENT_LIST ;
  ----
  "&Guardar" = has_sprite "GUI_SaveSprite()" <Ctrl+S> <F2> ;
  "&Cerrar" = has_sprite "GUI_CloseSprite()" <Ctrl+W> ;
  ----
  "&Salir" = always "GUI_Exit()" <Ctrl+Q> <Esc> ;
}

&Edit {
  "&Deshacer" UNDO_ACCEL = can_undo "GUI_Undo()" <Ctrl+Z> <Ctrl+U> ;
  "&Rehacer" REDO_ACCEL = can_redo "GUI_Redo()" <Ctrl+R> ;
  ----
  "Cortar" = has_imagemask "cut_to_clipboard()" <Ctrl+X> <Shift+Del> ;
  "Copiar" = has_imagemask "copy_to_clipboard()" <Ctrl+C> <Ctrl+Ins> ;
  "Pegar" = has_clipboard "paste_from_clipboard(true, 0, 0, 0, 0)" <Ctrl+V> <Shift+Ins> ;
  "Borrar" = has_image "GUI_EditClear()" <Ctrl+B> <Ctrl+Del> ;
  ----
  "Movimiento Rápido" = has_imagemask "quick_move()" <Shift+M> ;
  "Copia Rápida" = has_imagemask "quick_copy()" <Shift+C> ;
#  "Intercambio Rápido" = has_imagemask "quick_swap()" <Shift+S> ;
  ----
  "Flip &Horizontal" = has_image "GUI_FlipHorizontal()" <Shift+H> ;
  "Flip &Vertical" = has_image "GUI_FlipVertical()" <Shift+V> ;
  ----
  "Reemplazar Color" = has_image "dialogs_replace_color()" </> ;
  "Invertir" = has_image "dialogs_invert_color()" <Ctrl+I> ;
}

&Sprite {
  "&Propiedades" = has_sprite "GUI_SpriteProperties()" <Ctrl+P> ;
  ----
  "&Duplicar" = has_sprite "GUI_DuplicateSprite()" ;
  "&Tipo de Imagen" = has_sprite "GUI_ImageType()" ;
  ----
  #"Tamaño de &Imagen" = has_sprite "print(\"Unavailable\")" ;
  #"&Canvas Resize" = has_sprite "canvas_resize()" ;
  "Rec&ortar" = has_mask "crop_sprite()" ;
  "Recortar &Automáticamente" = has_sprite "autocrop_sprite()" ;
}

&Layer LAYER_POPUP {
  "&Propiedades" = has_layer "GUI_LayerProperties()" <Shift+P> ;
  ----
  "&Nueva Capa" = has_sprite "GUI_NewLayer()" <Shift+N> ;
  "Nuevo Con&junto" = has_sprite "GUI_NewLayerSet()" ;
  "&Remover" = has_layer "GUI_RemoveLayer()" ;
  ----
  "&Duplicar" = has_layer "GUI_DuplicateLayer()" ;
  "&Mezclar Abajo" = has_layer "GUI_MergeDown()" ;
  "&Aplanar" = has_layer "FlattenLayers()" ;
  "Rec&ortar" = has_mask "crop_layer()" ;
}

F&rame FRAME_POPUP {
  "&Propiedades" = has_layer "GUI_FrameProperties()" <Ctrl+Shift+P> ;
  "&Remover" = has_image "GUI_RemoveFrame()" ;
  ----
  "&Nuevo" = has_layerimage "new_frame()" <Ctrl+Shift+N> ;
  "&Mover" = is_movingframe "move_frame()" ;
  "&Copiar" = is_movingframe "copy_frame()" ;
  "En&lazar" = is_movingframe "link_frame()" ;
  ----
  "Rec&ortar" = has_imagemask "crop_frame()" ;
}

&Mask {
  "&Todo" = has_sprite "MaskAll()" <Ctrl+A> ;
  "&Deseleccionar" = has_mask "DeselectMask()" <Ctrl+D> ;
  "&Reseleccionar" = has_sprite "ReselectMask()" <Shift+Ctrl+D> ;
  "&Invertir" = has_sprite "InvertMask()" <Shift+Ctrl+I> ;
  ----
  "&Color" = has_image "dialogs_mask_color()" <?> ;
  "&Especial" {
    "Estirar &Fondo" = has_mask "StretchMaskBottom()" ;
  }
  ----
  "Repositorio" = has_sprite "dialogs_mask_repository()" <Shift+Ctrl+M> ;
  "Cargar" = has_sprite "GUI_LoadMask()" ;
  "Guardar" = has_mask "GUI_SaveMask()" ;
}

# XXX
# &Path {
#   "Repositorio" = has_sprite "print(\"Unavailable\")" <Shift+Ctrl+P> ;
#   "Cargar" = has_sprite "print(\"Unavailable\")" ;
#   "Guardar" = has_path "print(\"Unavailable\")" ;
# }

&Tool {
  "&Herramientas de Dibujo" {
    "&Configurar" = always "dialogs_tools_configuration()" <C> ;
    ----
    "&Marcador" = tool_marker "select_tool(\"Marker\")" <M> ;
    "Puntos" = tool_dots "select_tool(\"Dots\")" <D> ;
    "Pincel" = tool_pencil "select_tool(\"Pencil\")" <P> ;
    "&Brocha (pincel real)" = tool_brush "select_tool(\"Brush\")" <B> ;
    "Relleno" = tool_floodfill "select_tool(\"Floodfill\")" <F> ;
    "&Spray" = tool_spray "select_tool(\"Spray\")" <S> ;
    "&Línea" = tool_line "select_tool(\"Line\")" <L> ;
    "&Rectángulo" = tool_rectangle "select_tool(\"Rectangle\")" <R> ;
    "&Elipse" = tool_ellipse "select_tool(\"Ellipse\")" <E> ;
  }
  &Filtros FILTERS_POPUP {
    "&Matriz de Convolución" = has_image "dialogs_convolution_matrix()" <F9> ;
    "&Curva de Color" = has_image "dialogs_color_curve()" <Ctrl+M> <F10> ;
#     "Mapa de &Vectores" = has_image "dialogs_vector_map()" <F11> ;
    ----
    "&Reducción de ruido (filtrado por mediana)" = has_image "dialogs_median_filter()" ;
  }
  &Scripts {
    "Cargar script desde &archivo" = always "GUI_LoadScriptFile()" <Ctrl+0> ;
    ----
    "aselogo.lua" = always "include(\"examples/aselogo.lua\")" ;
    "cosy.lua" = always "include(\"examples/cosy.lua\")" ;
    "dacap.lua" = always "include(\"examples/dacap.lua\")" ;
    "shell.lua" = always "include(\"examples/shell.lua\")" ;
    "spiral.lua" = always "include(\"examples/spiral.lua\")" ;
    "terrain.lua" = always "include(\"examples/terrain.lua\")" ;
  }
  ----
  "Dibujar &Texto" = has_sprite "dialogs_draw_text()" ;
  "&MapGen" = always "dialogs_mapgen()" ;
  "&Reproducir Archivo FLI/FLC" = always "GUI_PlayFLI()" ;
  ----
  "&Opciones" = always "dialogs_options()" <Shift+Ctrl+O> ;
}

&Ver {
  "&Alicatado" = has_sprite "view_tiled()" <F8> ;
  "&Normal" = has_sprite "view_normal()" <F7> ;
  "Pantalla c&ompleta" = has_sprite "view_fullscreen()" ;
  ----
  "Barra de &Menu" = menu_bar "app_switch(app_get_menu_bar())" <Ctrl+F1> ;
  "Barra de E&stado" = status_bar "app_switch(app_get_status_bar())" <Ctrl+F2> ;
  "Barra de &Colores" = color_bar "app_switch(app_get_color_bar())" <Ctrl+F3> ;
  "Barra de &Herramientas" = tool_bar "app_switch(app_get_tool_bar())" <Ctrl+F4> ;
  ----
  &Editor {
    "Hacer &Único" = always "make_unique_editor(current_editor)" <Ctrl+1> ;
    ----
    "Dividir &Verticalmente" = always "split_editor(current_editor, JI_VERTICAL)" <Ctrl+2> ;
    "Dividir &Horizontalmente" = always "split_editor(current_editor, JI_HORIZONTAL)" <Ctrl+3> ;
    ----
    "&Cerrar" = always "close_editor(current_editor)" <Ctrl+4> ;
  }
  "E&ditor de Película" FILMEDITOR_ACCEL = has_sprite "switch_between_film_and_sprite_editor()" <Tab> ;
  "&Paleta de Colores" = always "dialogs_palette_editor()" <F4> ;
  ----
  "Popup Menu de &Filtros" = always "show_fx_popup_menu()" <X> ;
}

L&ista SPRITE_LIST ;
