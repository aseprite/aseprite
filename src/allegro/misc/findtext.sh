#! /bin/sh
#
#  Shell script to scan the source and find any text strings that need
#  to be translated. It creates an entext.cfg file containing the
#  English version, and if given a commandline argument, will then
#  check the named file (which should be a translation into some other
#  language) to see if it is missing any of the strings.


echo "Scanning for translation strings..."

echo "language_name = English" > entext.cfg
echo >> entext.cfg
echo "[language]" >> entext.cfg

grep get_config_text src/*.c src/*/*.c | \
   sed -n -e "s/.*get_config_text *(\"\(.*\)\").*/\1/p" \
	  -e "/ascii_name/d" -e "/gfx_mode_data/d" \
	  -e "/depth/d" -e "/char \*/d" \
	  -e "s/\(.*\):.*get_config_text *(\([^\"].*\)).*/\1 generates \2/p" | \
      \
   sed -e "p" -e "s/[ =#]/_/g" | \
   sed -e "N" -e "s/\(.*\)\n\(.*\)/\2 = \1/" | \
   sed -e "s/.* = \([a-zA-Z0-9_\/]*\.c\) generates \(.*\)/warning: \1 uses generated string \2/" | \
   sort -d | \
   uniq >> entext.cfg

if [ $# -gt 0 ]; then
   echo "Comparing entext.cfg with $1..."
   en=`mktemp findtext.XXXXXX`
   nat=`mktemp findtext.XXXXXX`
   sed -e "s/=.*//" -e "s/ //" entext.cfg | sort > $en
   sed -e "s/=.*//" -e "s/ //" $1 | sort > $nat
   echo "-------- Missing Translations --------"
   comm -2 -3 $en $nat
   echo "-------- Possibly Unused Translations --------"
   comm -1 -3 $en $nat
   rm $en $nat
else
   echo "Translation strings written into entext.cfg"
fi
