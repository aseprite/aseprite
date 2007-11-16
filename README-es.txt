
 ASE - Allegro Sprite Editor
 Copyright (C) 2001-2005, 2007 por David A. Capello
 ------------------------------------------------------------------------
 Mire el fichero "AUTHORS.txt" para la lista completa de colaboradores


===================================
COPYRIGHT
===================================

  Este programa es software libre. Puede redistribuirlo y/o modificarlo bajo
  los términos de la Licencia Pública General GNU según es publicada por la
  Free Software Foundation, bien de la versión 2 de dicha Licencia o bien
  (según su elección) de cualquier versión posterior.

  Este programa es distribuido con la esperanza de que sea útil, pero SIN
  GARANTIA ALGUNA, incluso sin la garantía implícita de COMERCIALIZACIÓN o
  IDONEIDAD PARA UN PROPÓSITO PARTICULAR. Véase la Licencia Pública General
  de GNU para más detalles.

  Debería haber recibido una copia de la Licencia Pública General junto con
  este programa. Si no ha sido así, escriba a la Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


===================================
INTRODUCCIÓN
===================================

  ASE es un programa especialmente diseñado para crear sprites
  animados con mucha facilidad y que luego puedan ser utilizados en un
  video juego.  Este programa le permitirá crear desde imágenes
  estáticas, a personajes con movimiento, texturas, patrones, fondos,
  logos, paletas de colores, y cualquier otra cosa que se le ocurra.


===================================
CARACTERÍSTICAS
===================================

  ASE le ofrece la posibilidad de:

  - Editar sprites con capas y cuadros de animación.

  - Editar imágenes RGB (con Alpha), escala de grises (también con
    Alpha), e imágenes con paleta de 256 colores o "indexadas".

  - Controlar paletas de 256 colores completamente.

  - Aplicar filtros para diferentes efectos (matriz de convolución,
    curva de color, etc.).

  - Cargar y guardar sprites en los formatos .BMP, .PCX, .TGA, .JPG,
    .GIF, .FLC, .FLI, .ASE (el formato especial de ASE).

  - Utilizar secuencia de bitmaps (ani00.pcx, ani01.pcx, etc.) para
    guardar las animaciones.

  - Herramientas de dibujo (puntos, pincel, brocha real, relleno, línea,
    rectángulo, elipse), modos de dibujo (opaco, vidrio), y tipos de
    brochas (círculo, cuadrado, línea).

  - Soporte de máscaras (selecciones).

  - Soporte para deshacer/rehacer cada operación.

  - Soporte para editores multiples.

  - Dibujar con una rejilla personalizable.

  - Único modo de dibujo alicatado para dibujar patrones y texturas en
    segundos.

  - Guardar y cargar sesiones completas de trabajo (en ficheros `.ses').

  - Capacidad de hacer `scripts' (guiones) con el lenguaje Lua
    (http://www.lua.org).


===================================
CONFIGURACIÓN
===================================

  En plataformas Windows y DOS:

    ase.cfg			- Configuración
    data/matrices		- Matrices de convolución
    data/menus			- Menús
    data/scripts/*		- Scripts o "guiones"

  En plataformas Unix, el archivo de configuratión es ~/.aserc, y los
  archivos de datos (en data/) son buscados en estos lugares (por
  orden de preferencia):

    $HOME/.ase/
    /usr/local/share/ase/
    data/

  Mire "src/core/dirs.c" para más información.


===================================
MODO VERBOSO
===================================

  Cuando ejecuta "ase" con el parámetro "-v", en las plataformas
  Windows y DOS los errores son escritos en STDERR y un archivo
  "logXXXX.txt" en el directorio "ase/" es creado con el mismo
  contenido.

  En otras plataformas (como Unix), ese archivo de registro no es
  creado, ya que la utilización de STDERR es mucho más común.

  Mire "src/core/core.c" para más información.


===================================
ACTUALIZACIONES
===================================

  Los últimos paquetes tanto binarios como el de código fuente, los
  puede encontrar desde:

    http://sourceforge.net/project/showfiles.php?group_id=20848

  También, si desea obtener la última versión en desarrollo de ASE desde el
  repositorio CVS, la cual por sierto es la más propensa a tener errores pero
  es la que más actualizada está con respecto a las herramientas, la puede
  explorar archivo por archivo en esta dirección:

    http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/ase

  O la puede bajar completamente a su disco con un programa que controle CVS,
  de la siguiente forma:

    1) Debemos ingresar al repositorio de forma anónima (cuando le pregunte
       por la contraseña, solamente presione <enter>):

         cvs -d :pserver:anonymous@cvs.ase.sourceforge.net:/cvsroot/ase login

    2) Realizamos el primer checkout, lo que significa que sacaremos una
       copia "fresca" de ASE:

         cvs -z3 -d :pserver:anonymous@cvs.ase.sourceforge.net:/cvsroot/ase checkout gnuase

    3) Una vez esto, cada vez que quiera actualizar la copia local con la
       del repositorio, deberá ejecutar en el directorio gnuase/:

         cvs update -Pd

  AVISO: Cuando obtenga la versión CVS, no borre los directorios CVS
  ni el contenido dentro de ellos, ya que es para uso interno del
  programa cvs.


===================================
CRÉDITOS
===================================

  Mire el archivo "AUTHORS.txt".


===================================
INFORMACIÓN DE CONTACTO
===================================

  Para pedir ayuda, reportar bugs, mandar parches, etc., utilice la
  lista de correo ase-help:

    ase-help@lists.sourceforge.net
    http://lists.sourceforge.net/lists/listinfo/ase-help/

  Para más información visite la página oficial del proyecto:

    http://ase.sourceforge.net


 ----------------------------------------------------------------------------
 Copyright (C) 2001-2005 por David A. Capello
