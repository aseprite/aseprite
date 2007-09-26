# allegro-sprite-editor tips
# Copyright (C) 2001, 2002, 2003, 2004, 2005, 2007  David A. Capello

 **********************************************************************
\palette aselogo.pcx
\image aselogo.pcx
Allegro Sprite Editor

Bienvenido a ASE

Puede encontrar más información sobre los atajos del teclado en el
archivo "docs/QuickHelp.html".

Reporte bugs y errores a:

ase-help@lists.sourceforge.net
 **********************************************************************
\split
\image nodither.pcx
Este es el viejo método para graficar imágenes RGB.  Sin fusionado.
\next
\image dither.pcx
Ahora, puede configurar para usar este nuevo método de fusionado de
colores.
\done

Vea el menú "Tool/Opciones" o presione <Ctrl+Shift+O>.
 **********************************************************************
Utilice las teclas <1>, <2>, <3>, <4>, <5> y <6> para cambiar el zoom
del editor activo.

\image zoom.pcx
 **********************************************************************
Puede obtener un color desde el sprite usando las teclas <9> y <0>
para cambiar los colores izquierdo o derecho respectivamente.
 **********************************************************************
\image sprprop.pcx

Puede modificar la cantidad de cuadros de animación del sprite usando
el menú "Sprite/Propiedades" o presionando <Ctrl+P>.
 **********************************************************************
Puede agregar cuadros de animación presionando <Ctrl+Shift+N> o
utilizando el menú "Frame/Nuevo".
 **********************************************************************
¿Quiere editar los cuadros y las capas?  Use el "Editor de Película":

\image filmedit.pcx

Vaya al menú "Ver/Editor de Película" o presione TAB cuando esté
editando un sprite.
 **********************************************************************
Puede usar el papel cebolla (onionskin) para editar animaciones
(presione <C> para ver la ventana de "Tools Setup"):

\image onion.pcx

Recuerde que solo la capa activa es utilizada como papel cebolla,
además, es útil solo cuando la capa usa el color máscara de fondo.
 **********************************************************************
Cuando aplica algún efecto (como la Matriz de Convolución, o la
Curva de Color), puede elegir los canales objetivos:
\split
\image tar_rgb.pcx
* R: Canal Rojo
* G: Canal Verde
* B: Canal Azul
* A: Canal Alpha
\next
\image tar_gray.pcx
* K: Canal Negro
* A: Canal Alpha
\next
\image tar_indx.pcx
* R: Canal Rojo
* G: Canal Verde
* B: Canal Azul
* Index: Utilizar directamente el índice
\done

También puede especificar a que imágenes le desea aplicar el efecto:

\split
\image tar_1.pcx
Cuadro actual de la capa actual
\next
\image tar_2.pcx
Todos los cuadros de la capa actual
\next
\image tar_3.pcx
Cuadros actuales de todas las capas
\next
\image tar_4.pcx
Todos los cuadros de todas las capas
\done
 **********************************************************************
\image colcurv.pcx

En la ventana de "Curva de Color", puede usar las teclas <Ins>
(insertar) y <Del> (suprimir) para insertar o borrar vértices
respectivamentes.  También, si presiona en uno de estos puntos con el
botón derecho del mouse, la venta "Propiedades del Punto" aparecera
(para cambiar las coordenadas manualmente).
 **********************************************************************
\image minipal.pcx

Seleccionando un color en la barra-de-colores usando la tecla <Ctrl>
le mostrará esta mini-paleta.
 **********************************************************************
\image lua.pcx

¿Desea escribir scripts (guiones) en Lua para generar imágenes o crear
tareas repetitivas?  Puede buscar la documentación en
"docs/scripts/README.txt".  También puede ver el directorio
"data/scripts/gens/" para algunos ejemplos.
 **********************************************************************
\palette sprite.pcx
\image sprite.pcx
Y recuerde buscar actualizaciones en:

http://ase.sourceforge.net/

Copyright (C) 2001, 2002, 2003, 2004, 2005, 2007 por David A. Capello
